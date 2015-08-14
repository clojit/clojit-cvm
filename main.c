#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>

#include "mps.h"
#include "mpsavm.h"
#include "mpscamc.h"
#include "c_hashmap/hashmap.h"

#include <stdbool.h>


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

#define VAL_BITS (48u)
#define TAG_MASK (0xFFFFul << VAL_BITS)


/* LANGUAGE EXTENSION */

#define LENGTH(array)	(sizeof(array) / sizeof(array[0]))



////////////////////////////Hashmap////////////////////////////////////

#define KEY_MAX_LENGTH (256)
#define KEY_PREFIX ("somekey")
#define KEY_COUNT (1024*1024)

typedef struct data_struct_s
{
    char key_string[KEY_MAX_LENGTH];
    uint64_t number;
} data_struct_t;


///////////////////////////////////////////////////////////////////


#define ADDVV 1
#define SUBVV 3
#define SETI 2
#define SETF 60
#define MULVV 4
#define MODVV 5
#define DIVVV 10

#define ISEQ 7
#define MOV 8
#define BULKMOV 9

#define LOOP 20

#define JUMP 30
#define JUMPF 31
#define JUMPT 32

#define FUNCF 50
#define FUNCV 51

#define ALLOC 100
#define GETFIELD 101
#define SETFIELD 102

#define NSSET 120
#define NSGET 121

#define EXIT 150


static mps_arena_t arena;       /* the arena */
static mps_pool_t obj_pool;     /* pool for ordinary Scheme objects */
static mps_ap_t obj_ap;         /* allocation point used to allocate objects */

static uint64_t slots[100] = {0};
static int pc = 0;

static map_t symbol_table;

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

const uint16_t TAG_DOUBLE_MAX = 0xFFF8;
const uint16_t TAG_DOUBLE_MIN = 0x0007;
const uint16_t TAG_POINTER_HI = 0xFFFF;
const uint16_t TAG_POINTER_LO = 0x0000;
const uint16_t TAG_INTEGER    = 0xFFFE;

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

static mps_gen_param_s obj_gen_params[] = {
  { 150, 0.85 },
  { 170, 0.45 }
};


uint64_t invert_non_negative(uint64_t slot) {
    uint64_t mask =  ( ~((int64_t)slot)  >> 63) & !(1 << 63);
    return slot ^ mask;
}

uint16_t tag(uint64_t slot) {
    return (slot >> 48 & 0xFFFF);
}

//Double Function

bool is_double(uint64_t slot) {
    uint16_t t = tag(slot);
    return t >= TAG_DOUBLE_MIN && t <= TAG_DOUBLE_MAX;
}

double get_double(uint64_t slot) {
    if (is_double(slot)) {
        uint64_t bits = invert_non_negative(slot);
        return *((double *)(void *) &bits);
    } else {
        printf("get_double called with nondouble");
        return 0.0;
    }
}

uint64_t to_double(double num) {
    uint64_t result = *((uint64_t*)(void*) &num);
    uint64_t result2 = invert_non_negative(result);
    return result2;
}

//Int Function

bool is_int(uint64_t slot) {
    uint16_t t = tag(slot);
    return t == TAG_INTEGER;
}

int64_t get_int(uint64_t slot) {
    return (slot & 0xFFFFFFFF);
}

uint64_t to_int(uint64_t num) {

 
    uint64_t int0 = ((uint64_t) TAG_INTEGER) << 48;

    //printf("empty : %016"PRIx64"\n", int0);
    //printf("num64 : %016"PRIx64"\n", (uint64_t) num);

    uint64_t result = int0 | num;

    //printf("to_int result: %016"PRIx64"\n", result);

    return result;
}

// Nil Function

bool is_nil(uint64_t slot) {
    return slot == 0;
}

// is Obj Function

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

mps_res_t rust_mps_create_amc_pool(mps_pool_t *pool_o, mps_fmt_t *fmt_o,mps_chain_t obj_chain,mps_arena_t arena)
{
    mps_res_t res;
    MPS_ARGS_BEGIN(args) {
        MPS_ARGS_ADD(args, MPS_KEY_FMT_ALIGN, ALIGNMENT);
        MPS_ARGS_ADD(args, MPS_KEY_FMT_SCAN, obj_scan);
        MPS_ARGS_ADD(args, MPS_KEY_FMT_SKIP, obj_skip);
        MPS_ARGS_ADD(args, MPS_KEY_FMT_FWD, obj_fwd);
        MPS_ARGS_ADD(args, MPS_KEY_FMT_ISFWD, obj_isfwd);
        MPS_ARGS_ADD(args, MPS_KEY_FMT_PAD, obj_pad);
        res = mps_fmt_create_k(fmt_o, arena, args);
    } MPS_ARGS_END(args);

    if (res != MPS_RES_OK) return res;

    MPS_ARGS_BEGIN(args) {
        MPS_ARGS_ADD(args, MPS_KEY_CHAIN, obj_chain);
        MPS_ARGS_ADD(args, MPS_KEY_FORMAT, *fmt_o);
        res = mps_pool_create_k(pool_o, arena, mps_class_amc(), args);
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

void rust_mps_debug_print_reachable(mps_arena_t arena, mps_fmt_t fmt) {
    fprintf(stderr, "==== Walking Reachable Objects ====\n");
    mps_arena_formatted_objects_walk(arena, rust_mps_debug_print_formatted_object, fmt, 0);
}

void print_slot (uint64_t slot) {
          if( is_int(slot) ) {
            printf("i%ld ", get_int( slot )  );
            //printf(" is int\n");
        }
       
        if( is_double(slot) ) {
            //printf(" is double\n");
            printf("f%.2f ", get_double( slot ) );
        }
        
        if( is_nil( slot ) ) {
            //printf(" is nil\n");
            printf("n0 ");
        }

        if( is_pointer( slot ) ) {
            //printf(" is is_pointer\n");


            uint64_t* header_ptr = (uint64_t*) slot;

            struct obj_stub  *obj = (struct obj_stub *) (void *) header_ptr;

            //printf(" pheader: %016"PRIx64" ", header);

            //< mpstype: %02"PRIx64", _: %02"PRIx64" cljtype: %04"PRIx64", size: %08"PRIx64">
            printf("( [cljtype: %d, size: %d] ", obj->cljtype,obj->size );

            for(int i = 0; i != ((obj->size / 8) - 1); i++) {
                //printf(" <field %i: %016"PRIx64"> ", i,  obj->ref[i] );
                print_slot( (uintptr_t) (void *) obj->ref[i]);
            }
                
            printf(") ");
        }
}

void print_slots(uint64_t* slots ,int size) {

    int i = 0;

    printf("Slots: ");
    while (1) {

        //printf("slots[%i]: %016"PRIx64"\n", i,  (uint64_t) slots[i] );

        print_slot(slots[i]);

        i++;

        if(i == size) {
            printf("\n");
            return;
        }
    }
}

static int start() {

    int map_error;
    char key_string[KEY_MAX_LENGTH];
    data_struct_t* value;
    
    symbol_table = hashmap_new();

    struct OpAD set1 =   { .op = SETI, .a = 0, .d = 1 };
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
                           exit_instr };
  
    mps_res_t res;
    mps_root_t root_o;

    res = mps_root_create_table_masked(&root_o, 
                                       arena,
                                       mps_rank_exact(),
                                       (mps_rm_t)0,
                                       (mps_addr_t *)slots,
                                       100,
                                       (mps_word_t)TAG_MASK);
    if (res != MPS_RES_OK) printf("Couldn't create slots roots");


    while (1) {
        instr inst = bytecodes[pc];

        struct OpABC abc = *((struct OpABC *) &inst);
        struct OpAD  ad  = *((struct OpAD *) &inst);

        uint8_t op = abc.op;

        switch (op) {
            //MATH
            case ADDVV: {
                int target_slot = abc.a;
                uint64_t bslot = slots[abc.b];
                uint64_t cslot = slots[abc.c];
                
                if ( is_int(bslot) && is_int(cslot) )
                    slots[target_slot] = to_int(get_int(bslot) + get_int(cslot));
                if ( is_double(bslot) && is_double(cslot) )
                    slots[target_slot] = to_double(get_double(bslot) + get_double(cslot));
                if ( is_double(bslot) && is_int(cslot) )
                    slots[target_slot] = to_double(get_double(bslot) + get_int(cslot) );
                if ( is_int(bslot) && is_double(cslot) )
                    slots[target_slot] = to_double(get_double(cslot) + get_int(bslot) );

                printf("ADDVV: %d %d %d\n", abc.a, abc.b, abc.c);
                break;
            }
            case SUBVV: {
                int target_slot = abc.a;
                uint64_t bslot = slots[abc.b];
                uint64_t cslot = slots[abc.c];

                if ( is_int(bslot) && is_int(cslot) )
                    slots[target_slot] = to_int(get_int(bslot) - get_int(cslot));
                if ( is_double(bslot) && is_double(cslot) )
                    slots[target_slot] = to_double(get_double(bslot) - get_double(cslot));
                if ( is_double(bslot) && is_int(cslot) )
                    slots[target_slot] = to_double(get_double(bslot) - get_int(cslot) );
                if ( is_int(bslot) && is_double(cslot) )
                    slots[target_slot] = to_double(get_double(cslot) - get_int(bslot) );

                printf("SUBVV: %d %d %d\n", abc.a, abc.b, abc.c);
                break;
            }
            case MULVV: {
                int target_slot = abc.a;
                uint64_t bslot = slots[abc.b];
                uint64_t cslot = slots[abc.c];

                if ( is_int(bslot) && is_int(cslot) )
                    slots[target_slot] = to_int(get_int(bslot) * get_int(cslot));
                if ( is_double(bslot) && is_double(cslot) )
                    slots[target_slot] = to_double(get_double(bslot) * get_double(cslot));
                if ( is_double(bslot) && is_int(cslot) )
                    slots[target_slot] = to_double(get_double(bslot) * get_int(cslot) );
                if ( is_int(bslot) && is_double(cslot) )
                    slots[target_slot] = to_double(get_double(cslot) * get_int(bslot) );

                printf("MULVV: %d %d %d\n", abc.a, abc.b, abc.c);
                break;
            }
            case MODVV: {
                printf("MODVV: %d %d %d\n", abc.a, abc.b, abc.c);
                int target_slot = abc.a;
                uint64_t bslot = slots[abc.b];
                uint64_t cslot = slots[abc.c];

                if ( is_int(bslot) && is_int(cslot) ) {
                    slots[target_slot] = to_int(get_int(bslot) % get_int(cslot));                
                } else {
                    printf("Type Error. Called Modulo with Float");
                    return 0;
                }

                slots[target_slot] =  slots[abc.b] % slots[abc.c];

                break;
            }
            case DIVVV: {
                printf("DIVVV: %d %d %d\n", abc.a, abc.b, abc.c);
                
                int target_slot = abc.a;
                uint64_t bslot = slots[abc.b];
                uint64_t cslot = slots[abc.c];

                if ( is_int(bslot) && is_int(cslot) )
                    slots[target_slot] = to_int(get_int(bslot) / get_int(cslot));

                if ( is_double(bslot) && is_double(cslot) )
                    slots[target_slot] = to_double(get_double(bslot) / get_double(cslot));

                if ( is_double(bslot) && is_int(cslot) )
                    slots[target_slot] = to_double(get_double(bslot) / get_int(cslot) );

                if ( is_int(bslot) && is_double(cslot) )
                    slots[target_slot] = to_double(get_double(cslot) / get_int(bslot) );
                break;
            }
            //EQUALITY
            case ISEQ: {
                printf("ISEQ: %d %d %d\n", abc.a, abc.b, abc.c);
                int target_slot = abc.a;
                uint64_t bslot = slots[abc.b];
                uint64_t cslot = slots[abc.c];


                //print_slot(bslot);
                //printf(" ");
                //print_slot(bslot);
                //printf("\n");
                if ( is_int(bslot) && is_int(cslot) ) {
                    slots[target_slot] = (get_int(bslot) == get_int(cslot));
                    break;
                }
                if ( is_double(bslot) && is_double(cslot) ) {
                    slots[target_slot] = to_int(get_double(bslot) == get_double(cslot));
                    break;
                }
                printf("Type Error ISEQ can only called with all Ints or all Double");
                return 1;      

            }
            //SET and MOVE
            case MOV: {
                int target_slot = ad.a;
                 slots[target_slot] = slots[ad.d];
                 printf("MOV: %d %d\n", ad.a, ad.d);
                 break;
            }            
            case SETI: {
                int target_slot = ad.a;

                uint64_t res =  to_int( (uint64_t) ad.d ) ;
                slots[target_slot] = to_int( (uint64_t) ad.d  );
                printf("SETI: %d %d\n", ad.a, ad.d);

                break;
            }
            case SETF: {
                int target_slot = ad.a;
                slots[target_slot] = to_double( 3.5  );
                printf("SETF: %d %.2f\n", ad.a, 3.5);
                break;                
            }
            case BULKMOV: {
                for(int i = 0; i != abc.c ;i++ ) {
                    slots[abc.a+i] = slots[abc.b+i];
                }
                printf("BULKMOV: %d %d %d\n", abc.a, abc.b, abc.c);
                break;
            }
            //Object stuff
            case ALLOC: {
                printf("ALLOC: %d %d\n",ad.a, ad.d);
                int target_slot = ad.a;
                uint16_t clj_type = ad.d;
                
                res = rust_mps_alloc_obj((mps_addr_t *) (void *) &slots[target_slot],
                                         obj_ap,
                                         16, // 64/8=8 for header   64/8=8 for slots
                                         clj_type,
                                         OBJ_MPS_TYPE_OBJECT);
                 if (res != MPS_RES_OK) printf("Could't not allocate obj");

          


                 break;
            }
            //    OP        A    B            C
            //    SETFIELD  ref  offset(lit)  var
            case SETFIELD: {
                 printf("SETFIELD: %d %d %d\n",abc.a, abc.b, abc.c); 

                 int offset = abc.b;
                 int var_index = abc.c;
                 int ref_index = abc.a;

                 uint64_t* header_ptr = (uint64_t*) slots[ref_index];

                 struct obj_stub  *obj = (struct obj_stub *) (void *) header_ptr;
                 obj->ref[offset] = (uint64_t *) slots[var_index];

                 break;
            }
            //OP        A    B     C
            //GETFIELD  dst  ref   offset(lit)
            case GETFIELD: {
                printf("GETFIELD: %d %d %d\n",abc.a, abc.b, abc.c);

                int dst = abc.a;        
                int offset = abc.c;
                            
                uint64_t* header_ptr = (uint64_t*) slots[abc.b];
                struct obj_stub  *obj = (struct obj_stub *) (void *) header_ptr;

         
                slots[dst] = (uintptr_t) (void *) obj->ref[offset];
                break;
            }
            //JUMP
            case JUMP: {
                int offset = (int16_t) ad.d;
                int new_pc = pc + offset;
                pc = new_pc;
                printf("JUMP: %d\n",ad.d);
                break;
            }
            case JUMPF: {
                int boolslot = ad.a;
                if(slots[boolslot] == 0) {
                    int offset = (int16_t) ad.d;
                    pc = pc + offset;
                }
                printf("JUMPF: %d %d\n", ad.a, (int16_t)ad.d);
                break;
            }            
            case JUMPT: {
                int boolslot = ad.a;
                if(slots[boolslot] == 1) {
                    int offset = (int16_t) ad.d;
                    pc = pc + offset;
                }
                printf("JUMPT: %d %d\n", ad.a,  (int16_t) ad.d);
                break;
            }
            //NSSETS   var     const-str  ->  ns[const-str] = var
            case NSSET: {
                printf("NSSET: %d %d\n", ad.a, ad.d);
                value = malloc(sizeof(data_struct_t));
                snprintf(value->key_string, KEY_MAX_LENGTH, "%s%d", KEY_PREFIX, ad.d);
                //printf("key: %s  val (slots[%d]:", value->key_string, ad.a );
                //print_slot(slots[ad.a]);
                //printf(")\n");                
                value->number = slots[ad.a];

                map_error = hashmap_put(symbol_table, value->key_string, value);
                assert(map_error==MAP_OK);

                break;
            }
            //NSGETS   dst     const-str  ->  dst = ns[const-str]
            case NSGET: {
                printf("NSGET: %d %d\n", ad.a, ad.d);
                snprintf(key_string, KEY_MAX_LENGTH, "%s%d", KEY_PREFIX, ad.d);
                //printf("(get: %s val: ", key_string);
                map_error = hashmap_get(symbol_table, key_string, (void**)(&value));
                assert(map_error==MAP_OK);
                slots[ad.a] = value->number;
                //print_slot(slots[ad.a]);
                //printf(")\n");
                break;
            }
            case LOOP: { break; }
            case FUNCF:  { break; }
            case FUNCV:  { break; }
            case EXIT: { 
                        printf("Valid End Reached\n");
                        return 1;
                        }
            default: {
                printf("skipping instruction: %d", op);
                break;
            }

        }
        pc++;

        print_slots(slots,10);

    }

    return 0;
}

int main() {
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

    exit_code = start();
 
    mps_arena_park(arena);
    mps_root_destroy(reg_root);
    mps_thread_dereg(thread);
    mps_ap_destroy(obj_ap);
    mps_pool_destroy(obj_pool);
    mps_chain_destroy(obj_chain);
    mps_fmt_destroy(obj_fmt);
//    mps_arena_destroy(arena);
}




