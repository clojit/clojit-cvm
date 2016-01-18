#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "mps.h"
#include "mpsavm.h"
#include "mpscamc.h"
#include "uthash.h"
#include "loader.h"
#include "stack.h"
#include "slots.h"

////////////////////////////// MEMORY MANAGEMENT //////////////////////////////

#define WORD_SIZE   sizeof(uint64_t)
#define ALIGNMENT   WORD_SIZE
#define HEADER_SIZE WORD_SIZE

#define ALIGN_WORD(size) \
  (((size) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

uint8_t OBJ_MPS_TYPE_PADDING = 0x00;
uint8_t OBJ_MPS_TYPE_FORWARD = 0x01;
uint8_t OBJ_MPS_TYPE_OBJECT  = 0x02;
uint8_t OBJ_MPS_TYPE_ARRAY   = 0x03;

static const char *OBJ_MPS_TYPE_NAMES[] = {
    "Padding", "Forward", "Object", "Array"
};

#define ARRAY_LEN(array)    (sizeof(array) / sizeof(array[0]))





/* LANGUAGE EXTENSION */

#define LENGTH(array)	(sizeof(array) / sizeof(array[0]))


///////////////////////// Prototypes ///////////////////////

uint16_t tag(uint64_t slot);

uint64_t get_nil();

bool is_small_int(uint64_t slot);
int32_t get_small_int(uint64_t slot);
uint64_t to_small_int(int32_t num);

bool is_bool(uint64_t slot);
bool get_bool(uint64_t slot);
uint64_t to_bool(bool value);
bool is_falsy(uint64_t slot);
bool is_truthy(uint64_t slot);

double get_double(uint64_t slot);
uint64_t to_double(double num);
bool is_double(uint64_t slot);

bool is_nil(uint64_t slot);

bool is_pointer(uint64_t slot);


// ---------------- Type Function ----------------
uint64_t to_type(uint16_t type);
bool is_type(uint64_t slot);
uint16_t get_type(uint64_t slot);
// ---------------- FNEW Function  ----------------
uint64_t to_fnew(int16_t fnew);
bool is_fnew(uint64_t slot);
int16_t get_fnew(uint64_t slot);

//  ----------------  CONTEXT  ---------------- 
void set_context(Context* ctx);
Context get_context();


mps_res_t rust_mps_alloc_obj(mps_addr_t *addr_o,
                             mps_ap_t ap,
                             uint32_t size,
                             uint16_t cljtype,
                             uint8_t mpstype);
void add_symbol_table_pair(struct sections* section, char * key, uint64_t num);
uint64_t invert_non_negative(uint64_t slot);
uint64_t get_symbol_table_record(struct sections* section, char* key);
void printbits(uint32_t n);
//const char *byte_to_binary(int x);
void print_slots(Slots* s,int size);
void print_slot (uint64_t slot);
void rust_mps_debug_print_reachable(mps_arena_t _arena, mps_fmt_t fmt);
mps_res_t rust_mps_create_ap(mps_ap_t *ap_o, mps_pool_t pool);
mps_res_t rust_mps_create_amc_pool(mps_pool_t *pool_o, mps_fmt_t *fmt_o,mps_chain_t obj_chain,mps_arena_t _arena);
mps_res_t rust_mps_create_vm_area(mps_arena_t *arena_o,size_t arenasize);
uint64_t get_symbol_table_record(struct sections* section, char* key);
void add_symbol_table_pair(struct sections* section, char * key, uint64_t num);
uint64_t get_symbol_table_record(struct sections* section, char* key);
void add_symbol_table_record(struct sections* section,
                             struct symbol_table_record *record);
void add_symbol_table_pair(struct sections* section, char * key, uint64_t num);
uint64_t get_symbol_table_record(struct sections* section, char* key);

////////////////////////////////////////////////////////////


///////////////////////// SYMBOL TABLE //////////////////////////////

void add_symbol_table_record(struct sections* section,
                             struct symbol_table_record *record) {
    HASH_ADD_KEYPTR(hh,section->symbol_table,record->symbol,strlen(record->symbol),record);
}

void add_symbol_table_pair(struct sections* section, char * key, uint64_t num) {

    struct symbol_table_record *record;
    record = (struct symbol_table_record*)malloc(sizeof(struct symbol_table_record));

    record->symbol = key;
    record->number = num;

    HASH_ADD_KEYPTR( hh, section->symbol_table, record->symbol, strlen(record->symbol), record );
}

uint64_t get_symbol_table_record(struct sections* section, char* key) {
    struct symbol_table_record *v;
    HASH_FIND_STR(section->symbol_table,key, v);

    return v->number;
}


///////////////////////////////////////////////////////////////////


#define CSTR  0
#define CKEY  1
#define CINT  2
#define CFLOAT  3
#define CTYPE   4
#define CBOOL  5
#define CNIL  6
#define CSHORT  7
#define SETF  8
#define NSSET  9
#define NSGET  10
#define ADDVV  11
#define SUBVV  12
#define MULVV  13
#define DIVVV  14
#define POWVV  15
#define MODVV  16
#define ISLT  17
#define ISGE  18
#define ISLE  19
#define ISGT  20
#define ISEQ  21
#define ISNEQ  22
#define MOV  23
#define NOT  24
#define NEG  25
#define JUMP  26
#define JUMPF 27
#define JUMPT  28
#define CALL  29
#define RET  30
#define APPLY  31
#define FNEW  32
#define VFNEW  33
#define GETFREEVAR  34
#define UCLO  35
#define LOOP  36
#define BULKMOV 37
#define NEWARRAY 38
#define GETARRAY 39
#define SETARRAY 40
#define FUNCF 41
#define FUNCV 42
#define ALLOC 43
#define SETFIELD 44
#define GETFIELD 45
#define BREAK 46
#define EXIT 47
#define DROP 48
#define TRANC 49

/////////////////////// Virtual Maschine Data ///////////////////////

static mps_arena_t arena;       /* the arena */
static mps_pool_t obj_pool;     /* pool for Clojure Object */
static mps_ap_t obj_ap;         /* allocation point used to allocate objects */

Slots slots = {0};
Stack stack = {0};

static uint32_t pc = 0;
static uint32_t ip = 0;
static uint32_t base_slot = 0;

/////////////////////////////////////////////////////////////////////

typedef uint32_t instr;

struct OpABC {
    uint8_t op;
    uint8_t a;
    uint8_t b;
    uint8_t c;
} __attribute__((packed));

struct OpAD {
    uint8_t op;
    uint8_t a;
    uint16_t d;
} __attribute__((packed));

const uint16_t TAG_DOUBLE_MAX    = 0xFFF8;
const uint16_t TAG_DOUBLE_MIN    = 0x0007;
const uint16_t TAG_POINTER_LO    = 0x0000;
const uint16_t TAG_POINTER_HI    = 0xFFFF;
const uint16_t TAG_SMALL_INTEGER = 0xFFFE;
const uint16_t TAG_BOOL          = 0xFFFD;
const uint16_t TAG_TYPE          = 0xFFFC;
const uint16_t TAG_FNEW          = 0xFFFB;


//////////////////////////////////// GC ////////////////////////////////////

/* obj_gen_params -- initial setup for generational GC          %%MPS
 *
 * Each structure in this array describes one generation of objects. The
 * two members are the capacity of the generation in kilobytes, and the
 * mortality, the proportion of objects in the generation that you expect
 * to survive a collection of that generation.
 * 
 * These numbers are *hints* to the MPS that it may use to make decisions
 * about when and what to collect: nothing will go wrong (other than
 * suboptimal performance) if you make poor choices.
 * 
 * Note that these numbers have deliberately been chosen to be small,
 * so that the MPS is forced to collect often so that you can see it
 * working. Don't just copy these numbers unless you also want to see
 * frequent garbage collections! See topic/collection.
 */

#ifndef MAXIMUM_HEAP_SIZE
#define MAXIMUM_HEAP_SIZE (512 * 1024 * 1024)
#endif

static mps_gen_param_s obj_gen_params[] = {
  { 8 * 1024, 0.45 },
  { MAXIMUM_HEAP_SIZE/1024 - 8 * 1024, 0.99 }
};

////////////////////////////////////////////////////////////////////////////

uint64_t invert_non_negative(uint64_t slot) {
    uint64_t mask =  ( ~((int64_t)slot)  >> 63) & !(1LU << 63);
    return slot ^ mask;
}

uint16_t tag(uint64_t slot) {
    return (slot >> 48 & 0xFFFF);
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
        printf("get_double called with nondouble\n");
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
    return 0;
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


bool is_falsy(uint64_t slot) {
    printf("is_truthy \n");
    if(is_bool(slot))
        return  get_bool(slot) == 0;
    if(is_nil(slot))
        return true;
    else
        return false;
}

bool is_truthy(uint64_t slot) {
    printf("is_truthy \n");
    return !is_falsy(slot);
}

// ---------------- OBJ Function ----------------

bool is_pointer(uint64_t slot) {
    uint16_t t = tag(slot);
    return  !is_nil(slot) && (t == TAG_POINTER_LO || t == TAG_POINTER_HI); 
}

struct obj_stub {
    uint8_t type;
    uint8_t _;
    uint16_t cljtype;
    uint32_t size; // incl. header
    mps_addr_t ref[];
} __attribute__((packed));

static mps_res_t obj_scan(mps_ss_t ss, mps_addr_t base, mps_addr_t limit) {
  MPS_SCAN_BEGIN(ss) {
    while (base < limit) {
        struct obj_stub *obj = base;
        // FIXME: we currently only scan objects, arrays are not supported yet
        if (obj->type == OBJ_MPS_TYPE_OBJECT) {
            mps_addr_t ref_base =  obj->ref;
            mps_addr_t ref_limit = (uint8_t*)ref_base + obj->size;
            for (mps_addr_t ref = ref_base; ref < ref_limit; ref++) {
                mps_res_t res = MPS_FIX12(ss, &ref);
                if (res != MPS_RES_OK) return res;
            }
        }
        base = (uint8_t*)base + obj->size;
    }
  } MPS_SCAN_END(ss);
  return MPS_RES_OK;
}

static mps_addr_t obj_skip(mps_addr_t base)
{
    struct obj_stub *obj = base;
    base = (uint8_t*)base + obj->size;
    return base;
}

static mps_addr_t obj_isfwd(mps_addr_t addr)
{
    struct obj_stub *obj = addr;
    if (obj->type == OBJ_MPS_TYPE_FORWARD) {
        return obj->ref[0];
    }

    return NULL;
}

static void obj_fwd(mps_addr_t old,
                    mps_addr_t new)
{
    struct obj_stub *obj = old;
    mps_addr_t limit = obj_skip(old);
    uint32_t size = (uint32_t)((uint8_t*)limit - (uint8_t*)old);

    obj->type = OBJ_MPS_TYPE_FORWARD;
    obj->size = size;
    obj->ref[0] = new;
}

static void obj_pad(mps_addr_t addr, size_t size)
{
    struct obj_stub *obj = addr;
    obj->type = OBJ_MPS_TYPE_PADDING;
    obj->size = size;
}

mps_res_t rust_mps_create_vm_area(mps_arena_t *arena_o,
                                  size_t arenasize)
{
    mps_res_t res;

    MPS_ARGS_BEGIN(args) {
        MPS_ARGS_ADD(args, MPS_KEY_ARENA_SIZE, arenasize);
        res = mps_arena_create_k(arena_o, mps_arena_class_vm(),  args);
    } MPS_ARGS_END(args);

    return res;
}

// caller needs to make sure to root addr_o before calling this!
// size is the size in bytes (including header)
mps_res_t rust_mps_alloc_obj(mps_addr_t *addr_o,
                             mps_ap_t ap,
                             uint32_t size,
                             uint16_t cljtype,
                             uint8_t mpstype)
{
    assert(addr_o != NULL);
    assert(size > HEADER_SIZE);
    mps_res_t res;

    do {
        res = mps_reserve(addr_o, ap, size);
        if (res != MPS_RES_OK) return res;
        struct obj_stub *obj = *addr_o;

        obj->type = mpstype;
        obj->cljtype = cljtype;
        obj->size = size;

        // zero all fields
        memset(obj->ref, 0, size - HEADER_SIZE);

    } while (!mps_commit(ap, *addr_o, size));

    return res;
}

mps_res_t rust_mps_create_amc_pool(mps_pool_t *pool_o, mps_fmt_t *fmt_o,mps_chain_t obj_chain,mps_arena_t _arena)
{
    mps_res_t res;
    MPS_ARGS_BEGIN(args) {
        MPS_ARGS_ADD(args, MPS_KEY_FMT_ALIGN, ALIGNMENT);
        MPS_ARGS_ADD(args, MPS_KEY_FMT_SCAN, obj_scan);
        MPS_ARGS_ADD(args, MPS_KEY_FMT_SKIP, obj_skip);
        MPS_ARGS_ADD(args, MPS_KEY_FMT_FWD, obj_fwd);
        MPS_ARGS_ADD(args, MPS_KEY_FMT_ISFWD, obj_isfwd);
        MPS_ARGS_ADD(args, MPS_KEY_FMT_PAD, obj_pad);
        res = mps_fmt_create_k(fmt_o, _arena, args);
    } MPS_ARGS_END(args);

    if (res != MPS_RES_OK) return res;

    MPS_ARGS_BEGIN(args) {
        MPS_ARGS_ADD(args, MPS_KEY_CHAIN, obj_chain);
        MPS_ARGS_ADD(args, MPS_KEY_FORMAT, *fmt_o);
        res = mps_pool_create_k(pool_o, _arena, mps_class_amc(), args);
    } MPS_ARGS_END(args);

    return res;
}

mps_res_t rust_mps_create_ap(mps_ap_t *ap_o, mps_pool_t pool) {
    return mps_ap_create_k(ap_o, pool, mps_args_none);
}

static void rust_mps_debug_print_formatted_object(mps_addr_t addr,
                                            mps_fmt_t fmt,
                                            mps_pool_t pool,
                                            void *p, size_t s) {
    assert(p == fmt);
    struct obj_stub *obj = addr;
    assert(obj->type < ARRAY_LEN(OBJ_MPS_TYPE_NAMES));

    const char *mps_type = OBJ_MPS_TYPE_NAMES[obj->type];
    fprintf(stderr, "%s(0x%012"PRIxPTR") [%"PRIu32" bytes] ", mps_type, (uintptr_t)addr, obj->size);
    if (obj->type == OBJ_MPS_TYPE_OBJECT || obj->type == OBJ_MPS_TYPE_ARRAY) {
        fprintf(stderr, "[type: %"PRIu16"]", obj->cljtype);
    }
    fprintf(stderr, "\n");

    if (obj->type == OBJ_MPS_TYPE_OBJECT) {
        size_t count = (obj->size - HEADER_SIZE) / WORD_SIZE;
        for (size_t i=0; i<count; i++) {
            uint16_t tag = ((uintptr_t)obj->ref[i] & TAG_MASK) >> VAL_BITS;
            uint64_t val = (uintptr_t)obj->ref[i] & ~TAG_MASK;

            fprintf(stderr, "  0x%04"PRIx16":%012"PRIx64"\n", tag, val);
        }
    }
}

void rust_mps_debug_print_reachable(mps_arena_t _arena, mps_fmt_t fmt) {
    fprintf(stderr, "==== Walking Reachable Objects ====\n");
    mps_arena_formatted_objects_walk(_arena, rust_mps_debug_print_formatted_object, fmt, 0);
}

void print_slot (uint64_t slot) {

    if( is_small_int(slot) ) {
        printf("i%d ", get_small_int(slot));
        return;
    }

    if(is_nil(slot)) {
        printf("n0 ");
        return;
    }

    if(is_fnew(slot)) {
        printf("fn%d ", get_fnew(slot));
        return;
    }

    if(is_type(slot)) {
        printf("t%d", get_type(slot));
        return;
    }

    if(is_pointer(slot)) {
        printf("P ");
        /*
        uint64_t* header_ptr = (uint64_t*) slot;

        struct obj_stub  *obj = (struct obj_stub *) (void *) header_ptr;

        //printf(" pheader: %016"PRIx64" ", header);

        //< mpstype: %02"PRIx64", _: %02"PRIx64" cljtype: %04"PRIx64", size: %08"PRIx64">
        printf("( [cljtype: %d, size: %d] ", obj->cljtype,obj->size );

        for(int i = 0; i != ((obj->size / 8) - 1); i++) {
            //printf(" <field %i: %016"PRIx64"> ", i,  obj->ref[i] );
            print_slot( (uintptr_t) (void *) obj->ref[i]);
        }

        printf(") ");*/
        return;
    }

    if( is_double(slot) ) {
        printf("f%.2f ", get_double(slot));
        return;
    }

    printf("-");
}

// ------------------------- Print Slots ------------------------

void print_slots(Slots* s,int size) {

    int index = 0;

    printf("Slots: ");
    while (1) {

        if ( !(index >= s->size || index < 0) ) {
            print_slot(slots_get(s, index));
        }

        index++;

        if(index == size) {
            printf("\n");
            return;
        }
    }
}

void printbits(uint32_t n) {
    if (n) {
        printbits(n >> 1);
        printf("%d", n & 1);
    }
}

// ------------------------- Context ------------------------

void set_context(Context* ctx) {
    base_slot = ctx->base_slot;
    ip = ctx->ip;
}

Context get_context() {
    Context old = { .base_slot = base_slot, .ip = ip };
    return old;
}

// ------------------------- True Evaluation ------------------------


static int start(char *file) {
    printf("start\n");
    struct sections sec = {0};

    mps_res_t res;

    /*struct OpAD set1 =   { .op = CSHORT, .a = 0, .d = 1 };
    struct OpAD set2 =   { .op = SETF, .a = 1, .d = 5 };
    struct OpABC add1 =  { .op = ADDVV, .a = 0, .b = 0, .c = 1 };
    struct OpAD mov1 =   { .op = MOV, .a = 0, .d = 1 };
    struct OpABC bmov =  { .op = BULKMOV, .a = 2, .b = 0, .c = 2 };
    struct OpAD init = { .op = ALLOC, .a = 4, .d = 1 };
    struct OpABC set = { .op = SETFIELD, .a = 4, .b = 0, .c = 3 };
    struct OpABC get = { .op = GETFIELD, .a = 5, .b = 4, .c = 0 };
    struct OpAD jump1  = { .op = JUMPF, .a = 0 , .d =  -3 };
    struct OpAD exit  = { .op = EXIT, .a = 0 , .d = 0 };
    struct OpAD setg = { .op = NSSET, .a = 2, .d = 10 };
    struct OpAD getg = { .op = NSGET, .a = 6, .d = 10 };
    struct OpABC div1 = { .op = DIVVV, .a = 7, .b = 1, .c = 1 };
    struct OpABC eq2 = { .op = ISEQ, .a = 8, .b = 2, .c = 2 };

    instr set1instr =  *(instr*)(void*) &set1;
    instr set2instr =  *(instr*)(void*) &set2;
    instr add1instr =  *(instr*)(void*) &add1;
    instr sub1instr =  *(instr*)(void*) &mov1;
    instr bmov1instr = *(instr*)(void*) &bmov;
    instr exit_instr = *(instr*)(void*) &exit;
    instr jump1instr = *(instr*)(void*) &jump1;
    instr init_instr = *(instr*)(void*) &init;
    instr set_instr =  *(instr*)(void*) &set;
    instr get_instr =  *(instr*)(void*) &get;
    instr setg_instr =  *(instr*)(void*) &setg;
    instr getg_instr =  *(instr*)(void*) &getg;
    instr div1_instr =  *(instr*)(void*) &div1;
    instr eq2_instr =  *(instr*)(void*) &eq2;

    instr bytecodes[100] = {
                           set1instr,
                           set2instr,
                           add1instr,
                           sub1instr,
                           bmov1instr,
                           set1instr,
                           jump1instr,
                           init_instr,
                           setg_instr,
                           set_instr,
                           get_instr,
                           getg_instr,
                           div1_instr,
                           eq2_instr,
                           exit_instr };*/

    res = loadfile(file, &sec);
    if (res != MPS_RES_OK) printf("Couldn't load file");

    sec.symbol_table = NULL;

    /*printf("-----------end of loader ---------------t\n");
    for(int j = 0;j < 2; j++) {
        printf("float[%d]: %p\n",j, sec.cfloat + j );
    }
    printf("-----------end of loader ---------------t\n");*/

    printf("------------SLOT--------------\n");

    while (1) {
        instr inst = sec.instr[pc];
        //printf("------------INSTURCTION --------------\n");
        //printbits( ntohl(inst) );
        //printf("\n");

        struct OpABC abc = *((struct OpABC *) &inst);
        struct OpAD  ad  = *((struct OpAD *) &inst);

        uint8_t op = abc.op;

        switch (op) {
            //------------------Constant Table Value Operation------------------
            case CSTR: {
                uint16_t d = ntohs(ad.d);
                printf("CSTR: %d %d\n", ad.a, d);

                slots_set( &slots,(base_slot + ad.a), (uint64_t)(void*)sec.cstr[d]);

                pc++;
                break;
            }
            case CKEY: {
                uint16_t d = ntohs(ad.d);

                printf("CKEY: %d %d\n", ad.a, d);
                slots_set(&slots, (base_slot + ad.a) , (uint64_t)(void*)sec.cstr[d]);

                pc++;
                break;
            }
            /*case CINT: {
                uint16_t d = ntohs(ad.d);
                printf("CINT: %d %d const: %d\n", ad.a, d, sec->cint[d]);

                slots.data[base + ad.a] = sec->cint[d];
                break;
            }*/
            /*case CFLOAT: {
                int target_slot = ad.a;
                printf("sec.cfloat[ad.d]: %f\n", swap(sec.cfloat[0]) );

                printf("CFLOAT: %d %d\n", ad.a, ad.d);
                break;
            }*/
            case CTYPE: {
                uint16_t d = ntohs(ad.d);
                printf("CTYPE: %d %d\n", ad.a, d);

                slots_set(&slots,(base_slot + ad.a),to_type(d));

                pc++;
                break;
            }
            //------------------Constant Value Operation------------------
            case CBOOL: {
                uint16_t d = ntohs(ad.d);
                printf("CBOOL: %d %d\n", ad.a, d);
                slots_set(&slots,(base_slot + ad.a),to_bool(d));
                pc++;
                break;
            }
            case CNIL : {
                printf("CNIL: %d\n", ad.a);
                slots_set(&slots,(base_slot +  ad.a),get_nil());
                pc++;
                break;
            }
            case CSHORT: {
                int target_slot = ad.a;
                uint16_t d16 = ntohs(ad.d);
                uint64_t d = (uint64_t) d16;

                printf("CSHORT: %d %d\n", ad.a, d16);
                slots_set(&slots,(base_slot + target_slot),to_small_int(d));

                pc++;
                break;
            }
            /*case SETF: {
                int target_slot = ad.a;
                slots.data[target_slot] = to_double(3.5);
                printf("SETF: %d %.2f\n", ad.a, 3.5);
                break;
            }*/
            //------------------Global Table Ops------------------
            case NSSET: {
                uint16_t d = ntohs(ad.d);
                printf("NSSET: %d %d\n", ad.a, d);
                add_symbol_table_pair(&sec,sec.cstr[d],
                                      slots_get(&slots,(base_slot + ad.a)));

                pc++;
                break;
            }
            case NSGET: {
                uint16_t d = ntohs(ad.d);
                printf("NSGET: %d %d\n", ad.a, d);
                slots_set(&slots,(base_slot + ad.a),get_symbol_table_record(&sec,sec.cstr[d]));
                pc++;
                break;
            }
            //------------------Variable Slots------------------
            case ADDVV: {
                int target_slot = base_slot + abc.a;
                uint64_t bslot = slots_get(&slots, (base_slot + abc.b));
                uint64_t cslot = slots_get(&slots, (base_slot +abc.c));

                printf("ADDVV: %d %d %d\n", abc.a, abc.b, abc.c);
                
                if (is_small_int(bslot) && is_small_int(cslot))
                    slots_set(&slots,target_slot,to_small_int(get_small_int(bslot) + get_small_int(cslot)));

                if (is_double(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(bslot) + get_double(cslot)));

                if (is_double(bslot) && is_small_int(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(bslot) + get_small_int(cslot)));

                if (is_small_int(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(cslot) + get_small_int(bslot)));

                pc++;
                break;
            }
            case SUBVV: {
                int target_slot = base_slot + abc.a;
                uint64_t bslot = slots_get(&slots, (base_slot + abc.b));
                uint64_t cslot = slots_get(&slots, (base_slot +abc.c));

                printf("SUBVV: %d %d %d\n", abc.a, abc.b, abc.c);

                if (is_small_int(bslot) && is_small_int(cslot) )
                    slots_set(&slots,target_slot, to_small_int(get_small_int(bslot) - get_small_int(cslot)));

                if (is_double(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot, to_double(get_double(bslot) - get_double(cslot)));

                if (is_double(bslot) && is_small_int(cslot))
                    slots_set(&slots,target_slot, to_double(get_double(bslot) - get_small_int(cslot)));

                if (is_small_int(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(cslot) - get_small_int(bslot)));

                pc++;
                break;
            }
            case MULVV: {
                int target_slot = base_slot + abc.a;
                uint64_t bslot = slots_get(&slots, (base_slot + abc.b));
                uint64_t cslot = slots_get(&slots, (base_slot +abc.c));

                printf("MULVV: %d %d %d\n", abc.a, abc.b, abc.c);

                if (is_small_int(bslot) && is_small_int(cslot))
                    slots_set(&slots,target_slot,to_small_int(get_small_int(bslot) * get_small_int(cslot)));

                if (is_double(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(bslot) * get_double(cslot)));

                if (is_double(bslot) && is_small_int(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(bslot) * get_small_int(cslot)));

                if (is_small_int(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(cslot) * get_small_int(bslot)));

                pc++;
                break;
            }
            case MODVV: {

                int target_slot = base_slot + abc.a;
                uint64_t bslot = slots_get(&slots, (base_slot + abc.b));
                uint64_t cslot = slots_get(&slots, (base_slot + abc.c));

                printf("MODVV: %d %d %d\n", abc.a, abc.b, abc.c);

                if (is_small_int(bslot) && is_small_int(cslot)) {
                    slots_set(&slots,target_slot, to_small_int(get_small_int(bslot) % get_small_int(cslot)));
                } else {
                    printf("Type Error. Called Modulo with Float\n");
                    exit(1);
                }

                pc++;
                break;
            }
            case DIVVV: {
                printf("DIVVV: %d %d %d\n", abc.a, abc.b, abc.c);
                
                int target_slot = base_slot + abc.a;
                uint64_t bslot = slots_get(&slots, (base_slot + abc.b));
                uint64_t cslot = slots_get(&slots, (base_slot + abc.c));

                if (is_small_int(bslot) && is_small_int(cslot))
                    slots_set(&slots,target_slot,to_small_int(get_small_int(bslot) / get_small_int(cslot)));

                if (is_double(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(bslot) / get_double(cslot)));

                if (is_double(bslot) && is_small_int(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(bslot) / get_small_int(cslot)));

                if (is_small_int(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(cslot) / get_small_int(bslot)));

                pc++;
                break;
            }
            case ISEQ: {
                printf("ISEQ: %d %d %d\n", abc.a, abc.b, abc.c);
                int target_slot = base_slot + abc.a;
                uint64_t bslot = slots_get(&slots, (base_slot + abc.b));
                uint64_t cslot = slots_get(&slots, (base_slot + abc.c));

                pc++;

                if (is_small_int(bslot) && is_small_int(cslot)) {
                    slots_set(&slots,target_slot, to_bool(get_small_int(bslot) == get_small_int(cslot)));
                    break;
                }
                if (is_double(bslot) && is_double(cslot)) {
                    slots_set(&slots,target_slot, to_bool(get_double(bslot) == get_double(cslot)));
                    break;
                }

                printf("Type Error ISEQ can only called with all Ints or all Double");
                return 1;      

            }
            //------------------Unary Ops------------------
            case MOV: {
                uint8_t target_slot = ad.a;
                uint16_t d = ntohs(ad.d);

                printf("MOV: %d %d\n",target_slot, d);

                slots_set(&slots,(base_slot + target_slot), slots_get(&slots, (base_slot + d)) );

                pc++;
                break;
            }
            case NOT: {
                printf("NOT // not implmented\n");
                pc++;
                break;
            }
            //------------------Jumps------------------
            case JUMP: {
                uint16_t d = ntohs(ad.d);
                printf("JUMP: %d\n",d);
                int16_t offset = (int16_t) d;

                uint32_t new_pc = pc + offset;

                pc = new_pc;
                break;
            }
            case JUMPF: {
                uint16_t d = ntohs(ad.d);
                int16_t offset = (int16_t) d;
                printf("JUMPF: %d %d\n",ad.a,offset);

                if(is_falsy(slots_get(&slots, (base_slot + ad.a)))) {
                    pc = pc + offset;
                } else {
                    pc++;
                }
                break;
            }
            case JUMPT: {
                uint16_t d = ntohs(ad.d);
                int16_t offset = (int16_t) d;
                printf("JUMPT: %d %d\n",ad.a,offset);

                if(is_truthy(slots_get(&slots, (base_slot + ad.a)))) {
                    pc = pc + offset;
                } else {
                    pc++;
                }
                break;
            }
            //------------------Function Calls------------------
            case CALL: {
                uint8_t localbase = ad.a;
                uint16_t lit = ntohs(ad.d);

                printf("CALL %d %d\n", localbase, lit);

                slots_set(&slots,(base_slot + localbase), to_small_int(pc));

                uint64_t fn_slot = slots_get(&slots, (base_slot + localbase + 1));

                uint16_t func = 0;
                if(is_fnew(fn_slot)) {
                    func = get_fnew(fn_slot);
                } else {
                    printf("CALL ERROR NO FUNC\n");
                    exit(0);
                }

                Context old = get_context();
                push(&stack, old);

                uint32_t newbase = base_slot + localbase;

                Context newContext = { .base_slot = newbase, .ip = func };
                set_context(&newContext);

                pc = ip;

                break;
            }
            case RET: {
                uint8_t a = ad.a;
                printf("RET %d\n", a);

                uint32_t ret_addr = get_small_int(slots_get(&slots, base_slot));

                slots_set(&slots, base_slot, slots_get(&slots, (base_slot+a)));

                Context caller = pop(&stack);
                set_context(&caller);

                pc = ret_addr + 1;
                break;
            }
            case APPLY: {
                printf("APPLY // not implmented\n");
                pc++;
                break;
            }
            //------------------Closures and Free Variables------------------
            case FNEW: {

                //Once Closure are added, it needs to check if it is inside of a closure
                uint16_t d = ntohs(ad.d);
                int16_t offset = (int16_t) d;

                printf("FNEW %d %d\n", ad.a, offset);

                slots_set(&slots, (base_slot + ad.a), to_fnew(offset));

                pc++;
                break;
            }
            case VFNEW: { printf("VFNEW // not implmented\n"); pc++; break; }
            case GETFREEVAR: { printf("GETFREEVAR // not implmented\n"); pc++; break; }
            case UCLO: { printf("UCLO // not implmented\n"); pc++; break; }
            //------------------Tail Recursion and Loops------------------
            case LOOP: { printf("LOOP\n"); break; }
            case BULKMOV: {
                for(int i = 0; i != abc.c ;i++ ) {
                    slots_set(&slots, (base_slot + abc.a + i),  slots_get(&slots, (base_slot + abc.b + i)));
                }
                printf("BULKMOV: %d %d %d\n", abc.a, abc.b, abc.c);
                pc++;
                break;
            }
            //------------------Arrays------------------
            case NEWARRAY:{
                printf("NEWARRAY // not implmented\n");
                pc++;
                break;
            }
            case GETARRAY:{
                printf("GETARRAY // not implmented\n");
                pc++;
                break;
            }
            case SETARRAY:{
                printf("SETARRAY // not implmented\n");
                pc++;
                break;
            }

            //------------------Function Def------------------
            case FUNCF:{ printf("FUNCF\n"); pc++; break; }
            case FUNCV:{ printf("FUNCV\n"); pc++; break; }
            //------------------Types------------------
            case ALLOC: {
                uint16_t d = ntohs(ad.d);
                printf("ALLOC: %d %d\n",ad.a,d);
                int target_slot = ad.a;

                struct type_record type = sec.types[d];

                uint64_t alloc_slot = slots_get(&slots,(base_slot + target_slot));

                res = rust_mps_alloc_obj((mps_addr_t*)(void*)&alloc_slot,
                                         obj_ap,
                                         type.type_size,
                                         type.type_id,
                                         OBJ_MPS_TYPE_OBJECT);
                 if (res != MPS_RES_OK) printf("Could't not allocate obj\n");

                 pc++;
                 break;
            }
            case SETFIELD: {
                 printf("SETFIELD: %d %d %d\n",abc.a, abc.b, abc.c);
                 int offset = abc.b;
                 int var_index = abc.c;
                 int ref_index = abc.a;

                 uint64_t* header_ptr = (uint64_t*)slots_get(&slots,(base_slot + ref_index));

                 struct obj_stub  *obj = (struct obj_stub *) (void *) header_ptr;
                 obj->ref[offset] = (uint64_t *)slots_get(&slots,(base_slot + var_index));

                 pc++;
                 break;
            }
            case GETFIELD: {
                printf("GETFIELD: %d %d %d\n",abc.a, abc.b, abc.c);
                int dst = abc.a;
                int offset = abc.c;

                uint64_t* header_ptr = (uint64_t*)slots_get(&slots,(base_slot + abc.b));
                struct obj_stub  *obj = (struct obj_stub *) (void *) header_ptr;

                slots_set(&slots, (base_slot + dst), (uintptr_t) (void *) obj->ref[offset]);

                pc++;
                break;
            }
            //------------------Run-Time Behavior------------------
            case BREAK:  {
                printf("BREAK // not implmented\n");
                pc++;
                break;
            }
            case EXIT: {
                printf("Valid End Reached\n");
                pc++;
                return 1;
            }
            case DROP: {
                printf("DROP // not implmented\n");
                pc++;
                break;
            }
            case TRANC: {
                printf("TRANC // not implmented\n");
                pc++;
                break;
            }
            default: {
                printf("skipping instruction: %d\n\n", op);
                pc++;
                break;
            }
        }
        print_slots(&slots,10);
    }


    return 0;
}

int main(int argc, char **argv) {

    size_t arenasize = 32ul * 1024 * 1024;
    mps_res_t res;
    mps_chain_t obj_chain;
    mps_fmt_t obj_fmt;
    mps_thr_t thread;
    mps_root_t reg_root;
    int exit_code;
    void *marker = &marker;

    res = rust_mps_create_vm_area(&arena, arenasize);
    if (res != MPS_RES_OK) printf("Couldn't create arena");

    res = mps_chain_create(&obj_chain,
                           arena,
                           LENGTH(obj_gen_params),
                           obj_gen_params);
    if (res != MPS_RES_OK) printf("Couldn't create obj chain");

    res = rust_mps_create_amc_pool(&obj_pool, &obj_fmt, obj_chain, arena);
    if (res != MPS_RES_OK) printf("Couldn't create obj pool");

    res = rust_mps_create_ap(&obj_ap, obj_pool);
    if (res != MPS_RES_OK) printf("Couldn't create obj allocation point");

    res = mps_thread_reg(&thread, arena);
    if (res != MPS_RES_OK) printf("Couldn't register thread");
    
    res = mps_root_create_reg(&reg_root,arena,mps_rank_ambig(),0,thread,mps_stack_scan_ambig,marker,0);
    if (res != MPS_RES_OK) printf("Couldn't create root");

    stack_init(&stack);
    slots_init(&slots, arena);

    if(argc > 1) {
        exit_code = start(argv[1]);
    } else {
        exit_code = MPS_RES_OK;
    }

    stack_free(&stack);
    slots_free(&slots);

    mps_arena_park(arena);
    mps_root_destroy(reg_root);
    mps_thread_dereg(thread);
    mps_ap_destroy(obj_ap);
    mps_pool_destroy(obj_pool);
    mps_chain_destroy(obj_chain);
    mps_fmt_destroy(obj_fmt);
//    mps_arena_destroy(arena);

    return exit_code;

}




