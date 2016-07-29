// slots.c

#include <stdio.h>
#include <stdlib.h>

#include "slots.h"
#include "vm.h"
#include "alloc.h"
#include "debug.h"

const uint16_t TAG_DOUBLE_MAX    = 0xFFF8;
const uint16_t TAG_DOUBLE_MIN    = 0x0007;
const uint16_t TAG_POINTER_LO    = 0x0000;
const uint16_t TAG_POINTER_HI    = 0xFFFF;
const uint16_t TAG_SMALL_INTEGER = 0xFFFE;
const uint16_t TAG_BOOL          = 0xFFFD;
const uint16_t TAG_TYPE          = 0xFFFC;
const uint16_t TAG_FNEW          = 0xFFFB;
const uint16_t TAG_BUILTIN       = 0xFFFA;
const uint16_t TAG_VFUNC         = 0xFFF9;

void slots_init(Slots *slots, mps_arena_t arena) {
    // initialize size and capacity
    slots->size = 0;
    slots->capacity = SLOTS_INITIAL_CAPACITY;
    slots->arena = arena;
    slots->data = malloc(sizeof(uint64_t) * slots->capacity);

    mps_root_t root_o;

    mps_res_t res = mps_root_create_table_masked(&root_o,
                                                 arena,
                                                 mps_rank_exact(),
                                                 (mps_rm_t)0,
                                                 (mps_addr_t *)slots->data,
                                                 slots->capacity,
                                                 (mps_word_t)TAG_MASK);

    slots->root_o = root_o;

    if (res != MPS_RES_OK) fprintf(stderr,"Couldn't create slots roots");

    // allocate memory for slots->data

}

void slots_append(Slots *slots, uint64_t value) {
  slots_double_capacity_if_full(slots);
  slots->data[slots->size++] = value;
}

uint64_t slots_get(Slots *slots, uint32_t index) {

    return * slots_get_ptr(slots, index);

}
uint64_t* slots_get_ptr(Slots *slots, uint32_t index) {

    if(slots->capacity != SLOTS_INITIAL_CAPACITY && ((uint32_t)(slots->capacity / 2) * 0.6) >= slots->size) {
        slots_half_capacity(slots);
    }

    if (index >= slots->size || index < 0) {
        fprintf(stderr,"Index %d out of bounds for slots of size %d\n", index, slots->size);
        exit(1);
    }

    return &(slots->data[index]);
}





void slots_set(Slots *slots, uint32_t index, uint64_t value) {

    while (index >= slots->size) {
        slots_append(slots, 0);
    }
    slots->data[index] = value;
}

static mps_addr_t obj_isfwd(mps_addr_t addr) {
    struct obj_stub *obj = addr;
    if (obj->type == OBJ_MPS_TYPE_FORWARD) {
        return obj->ref[0];
    }
    return NULL;
}


// Only call if you have checked that the halfed
// slot capacity will fit in the new slot
void slots_half_capacity(Slots *slots) {

    slots->capacity = (uint32_t) slots->capacity / 2;

    uint64_t* data = malloc(sizeof(uint64_t) * slots->capacity);

    mps_root_t root_o_old;
    root_o_old = slots->root_o;

    mps_root_t root_o_new;
    mps_res_t res = mps_root_create_table_masked(&root_o_new,
                                                 slots->arena,
                                                 mps_rank_exact(),
                                                 (mps_rm_t)0,
                                                 (mps_addr_t *)data,
                                                 slots->capacity,
                                                 (mps_word_t)TAG_MASK);
    if (res != MPS_RES_OK) fprintf(stderr,"Couldn't create root for increased slots\n");

    for(int i = 0; i != slots->size; i++) {
        data[i] = slots->data[i];
    }

    free(slots->data);

    slots->root_o = root_o_new;
    slots->data = data;
    mps_root_destroy(root_o_old);

}

void slots_double_capacity_if_full(Slots *slots) {

    if (slots->size >= slots->capacity) {

        /* Double the size of the Stack */
        slots->capacity *= 2;

        uint64_t* data = malloc(sizeof(uint64_t) * slots->capacity);

        mps_root_t root_o_old;
        root_o_old = slots->root_o;

        mps_root_t root_o_new;
        mps_res_t res = mps_root_create_table_masked(&root_o_new,
                                                     slots->arena,
                                                     mps_rank_exact(),
                                                     (mps_rm_t)0,
                                                     (mps_addr_t *)data,
                                                     slots->capacity,
                                                     (mps_word_t)TAG_MASK);
        if (res != MPS_RES_OK) fprintf(stderr,"Couldn't create root for decreased slots\n");


        for(int i = 0; i != slots->size; i++) {
            data[i] = slots->data[i];
        }

        free(slots->data);

        slots->root_o = root_o_new;
        slots->data = data;

        mps_root_destroy(root_o_old);
    }
}

void slots_free(Slots *slots) {
  free(slots->data);
}


//////////////////////////SLOT FUNCTION//////////////////////////

uint64_t invert_non_negative(uint64_t slot) {
    uint64_t mask =  ( ~((int64_t)slot)  >> 63) & !(1LU << 63);
    return slot ^ mask;
}

uint16_t tag(uint64_t slot) {
    return (slot >> 48 & 0xFFFF);
}

bool is_pointer(uint64_t slot) {
    uint16_t t = tag(slot);
    return  !is_nil(slot) && (t == TAG_POINTER_LO || t == TAG_POINTER_HI);
}


// ---------------- Double Function ----------------
bool is_double(uint64_t slot) {
    uint16_t t = tag(slot);
    return t >= TAG_DOUBLE_MIN && t <= TAG_DOUBLE_MAX;
}
double get_double(uint64_t slot) {
    if (is_double(slot)) {
        union slot bits;
        bits.raw = invert_non_negative(slot);
        return *((double *)(void*) bits.ptr);
    } else {
        fprintf(stderr,"get_double called with nondouble\n");
        return 0.0;
    }
}
uint64_t to_double(double num) {
    union slot bits;
    bits.dbl = num;
    return invert_non_negative(bits.raw);
}
// ---------------- Type Function ----------------
uint64_t to_type(uint16_t type) {
    uint64_t int0 = ((uint64_t) TAG_TYPE) << 48;
    uint64_t result = int0 | type;
    return result;
}
bool is_type(uint64_t slot) {
    uint16_t t = tag(slot);
    return t == TAG_TYPE;
}
uint16_t get_type(uint64_t slot) {
    return (slot & 0xFFFFFFFF);
}

// ---------------- BUILTIN Function --------------
uint64_t to_builtin(builtin_fn fn) {
    uint64_t int0 = ((uint64_t) TAG_BUILTIN) << 48;
    uint64_t result = int0 |  ((uint64_t) fn) ;
    return result;
}
bool is_builtin(uint64_t slot) {
    uint16_t t = tag(slot);
    return t == TAG_BUILTIN;
}
builtin_fn get_builtin(uint64_t slot) {
    return  (builtin_fn) (void *) (slot & 0xFFFFFFFF);
}

// ---------------- VFUNC Function  ----------------
uint64_t to_vfunc(int16_t type) {
    uint64_t int0 = ((uint64_t) TAG_VFUNC) << 48;
    uint64_t result = int0 | type;
    return result;
}
bool is_vfunc(uint64_t slot) {
    uint16_t t = tag(slot);
    return t == TAG_VFUNC;
}
int16_t get_vfunc(uint64_t slot) {
    return (slot & 0xFFFFFFFF);
}
// ---------------- FNEW Function  ----------------
uint64_t to_fnew(int16_t type) {
    uint64_t int0 = ((uint64_t) TAG_FNEW) << 48;
    uint64_t result = int0 | type;
    return result;
}
bool is_fnew(uint64_t slot) {
    uint16_t t = tag(slot);
    return t == TAG_FNEW;
}
int16_t get_fnew(uint64_t slot) {
    return (slot & 0xFFFFFFFF);
}
// ---------------- Int Function ----------------
uint64_t to_small_int(int32_t num) {
    uint64_t int0 = ((uint64_t) TAG_SMALL_INTEGER) << 48;
    uint64_t result = int0 | num;
    return result;
}
bool is_small_int(uint64_t slot) {
    uint16_t t = tag(slot);
    return t == TAG_SMALL_INTEGER;
}
int32_t get_small_int(uint64_t slot) {
    return (slot & 0xFFFFFFFF);
}
// ---------------- Nil Function ----------------
bool is_nil(uint64_t slot) {
    return slot == 0;
}
uint64_t get_nil() {
    return (uint64_t)0;
}
// ---------------- Bool Function ----------------
bool is_bool(uint64_t slot) {
    uint16_t t = tag(slot);
    return t == TAG_BOOL;
}
bool get_bool(uint64_t slot) {
    return (bool) (slot & 0x1);
}
uint64_t to_bool(bool value) {
    uint64_t mask = ((uint64_t) TAG_BOOL) << 48;
    uint64_t result = mask | (uint64_t) value;
    return result;
}
// ---------------- Truthy/Falsy Function ----------------
bool is_falsy(uint64_t slot) {
    fprintf(stderr,"is_truthy \n");
    if(is_bool(slot))
        return  get_bool(slot) == 0;
    if(is_nil(slot))
        return true;
    else
        return false;
}
bool is_truthy(uint64_t slot) {
    fprintf(stderr,"is_truthy \n");
    return !is_falsy(slot);
}