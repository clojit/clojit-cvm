
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "vm.h"

const uint8_t OBJ_MPS_TYPE_PADDING = 0x00;
const uint8_t OBJ_MPS_TYPE_FORWARD = 0x01;
const uint8_t OBJ_MPS_TYPE_OBJECT  = 0x02;
const uint8_t OBJ_MPS_TYPE_ARRAY   = 0x03;

/* -------------------------- Create Arena ----------------------- */
mps_res_t mps_create_vm_area(mps_arena_t *arena_o,
                                  size_t arenasize)
{
    mps_res_t res;

    MPS_ARGS_BEGIN(args) {
        MPS_ARGS_ADD(args, MPS_KEY_ARENA_SIZE, arenasize);
        res = mps_arena_create_k(arena_o, mps_arena_class_vm(),  args);
    } MPS_ARGS_END(args);

    return res;
}

/* -------------------------- Scan Function ----------------------- */
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

/* -------------------------- Skip Function ----------------------- */
static mps_addr_t obj_skip(mps_addr_t base)
{
    struct obj_stub *obj = base;
    base = (uint8_t*)base + obj->size;
    return base;
}

/* -------------------------- FORWARD Function ----------------------- */
static void obj_fwd(mps_addr_t old,mps_addr_t new) {
    struct obj_stub *obj = old;
    mps_addr_t limit = obj_skip(old);
    uint32_t size = (uint32_t)((uint8_t*)limit - (uint8_t*)old);

    obj->type = OBJ_MPS_TYPE_FORWARD;
    obj->size = size;
    obj->ref[0] = new;
}

/* -------------------------- IS FORWARD FUNCTION ----------------------- */
static mps_addr_t obj_isfwd(mps_addr_t addr) {
    struct obj_stub *obj = addr;
    if (obj->type == OBJ_MPS_TYPE_FORWARD) {
        return obj->ref[0];
    }
    return NULL;
}


/* -------------------------- PADDING Function ----------------------- */
static void obj_pad(mps_addr_t addr, size_t size)
{
    struct obj_stub *obj = addr;
    obj->type = OBJ_MPS_TYPE_PADDING;
    obj->size = size;
}

/* -------------------------- CREATE FORMAT ----------------------- */
mps_res_t mps_create_obj_fmt(mps_fmt_t *fmt_o, mps_arena_t _arena)
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


    return res;

}

/* -------------------------- CREATE AMC POOL ----------------------- */
mps_res_t mps_create_amc_pool(mps_pool_t *pool_o, mps_fmt_t fmt_o,mps_chain_t obj_chain,mps_arena_t _arena) {

    mps_res_t res;

    MPS_ARGS_BEGIN(args) {
        MPS_ARGS_ADD(args, MPS_KEY_CHAIN, obj_chain);
        MPS_ARGS_ADD(args, MPS_KEY_FORMAT, fmt_o);
        res = mps_pool_create_k(pool_o, _arena, mps_class_amc(), args);
    } MPS_ARGS_END(args);

    return res;
}

/* -------------------------- CREATE AMCZ POOL ----------------------- */
mps_res_t mps_create_amcz_pool(mps_pool_t *pool_o, mps_fmt_t fmt_o,mps_chain_t obj_chain,mps_arena_t _arena) {

    mps_res_t res;

    MPS_ARGS_BEGIN(args) {
        MPS_ARGS_ADD(args, MPS_KEY_CHAIN, obj_chain);
        MPS_ARGS_ADD(args, MPS_KEY_FORMAT, fmt_o);
        res = mps_pool_create_k(pool_o, _arena, mps_class_amcz(), args);
    } MPS_ARGS_END(args);

    return res;
}

/* ------------------------ CREATE ALLOCATION POOL --------------------- */
mps_res_t mps_create_ap(mps_ap_t *ap_o, mps_pool_t pool) {
    return mps_ap_create_k(ap_o, pool, mps_args_none);
}

void set_context(VM *vm, Context* ctx) {
    vm->base = ctx->base_slot;
    vm->ip = ctx->ip;
}

Context get_context(VM *vm) {
    Context old = { .base_slot = vm->base, .ip = vm->ip };
    return old;
}


////////////////////////////////////////////

uint64_t get(VM *vm, uint32_t index) {
    return slots_get( &(vm->slots), (vm->base + index));
}

void set(VM *vm, uint32_t index, uint64_t value) {
    slots_set( &(vm->slots), (vm->base + index) , value );
}

void move(VM *vm, uint32_t to, uint32_t from) {
    set(vm, to, get(vm, from));
}

////////////////////////////////////////////

uint64_t get_symbol_table(VM *vm, char * key) {
    return (uint64_t) (void *) g_hash_table_lookup(vm->symbol_table, (void *) key);
}

void add_symbol_table_pair(VM *vm, char * key, uint64_t value) {
    g_hash_table_insert(vm->symbol_table, key, (void *) value);
}


////////////////////////////////////////////////////////////////////////////////////

void vm_init(VM *vm, size_t arenasize) {

    mps_res_t res;
    void *marker = &marker;

    // -------------------------- ARENA ----------------------------
    res = mps_create_vm_area(&vm->arena, arenasize);
    if (res != MPS_RES_OK) printf("Couldn't create arena");

    // ------------------ GRATE GENERATION CHAIN --------------------------
    res = mps_chain_create(&vm->obj_chain,
                           vm->arena,
                           LENGTH(obj_gen_params),
                           obj_gen_params);
    if (res != MPS_RES_OK) printf("Couldn't create obj chain");

    // ------------- CONSERVATIVE SCAN THREAD STACK AND REG ---------------
    res = mps_thread_reg(&vm->thread, vm->arena);
    if (res != MPS_RES_OK) printf("Couldn't register thread");
    res = mps_root_create_reg(&vm->reg_root,vm->arena,mps_rank_ambig(),0,vm->thread,mps_stack_scan_ambig,marker,0);
    if (res != MPS_RES_OK) printf("Couldn't create root");

    // ---------------------- CREATE OBJ FORMAT -------------------------
    res = mps_create_obj_fmt(&vm->obj_fmt, vm->arena);
    if (res != MPS_RES_OK) printf("Couldn't create Obj Format");

    // ---------------------- CREATE AMC POOL -------------------------
    Pool amc = {0};
    res = mps_create_amc_pool(&amc.pool, vm->obj_fmt, vm->obj_chain, vm->arena);
    if (res != MPS_RES_OK) printf("Couldn't create obj pool (amc)");
    res = mps_create_ap(&amc.ap, amc.pool);
    if (res != MPS_RES_OK) printf("Couldn't create obj allocation point (amc)");
    vm->amc = &amc;

    // ---------------------- CREATE AMC Allocation Point -------------------------

    // ---------------------- CREATE AMCZ POOL -------------------------
    Pool amcz = {0};
    res = mps_create_amcz_pool(&amcz.pool, vm->obj_fmt, vm->obj_chain, vm->arena);
    if (res != MPS_RES_OK) printf("Couldn't create obj pool (amcz)");
    res = mps_create_ap(&amcz.ap,amcz.pool);
    if (res != MPS_RES_OK) printf("Couldn't create obj allocation point (amcz)");
    vm->amcz = &amcz;

    // ---------------------- INIT SLOTS -------------------------
    Slots slots = {0};

    slots_init(&slots, vm->arena);

    vm->slots = slots;

    // ------------------- INIT CONTEXT STACK ----------------------
    Stack stack = {0};

    stack_init(&stack);

    vm->stack = stack;


    // ------------------- Symbol Table ----------------------
     vm->symbol_table = g_hash_table_new(g_str_hash, g_str_equal);

     add_builtin_function((void *) vm); //TODO GANDRO
}

void free_vm (VM *vm) {

    mps_arena_park(vm->arena);
    mps_thread_dereg(vm->thread);
    mps_chain_destroy(vm->obj_chain);
    mps_root_destroy(vm->reg_root);
    mps_fmt_destroy(vm->obj_fmt);
    mps_arena_destroy(vm->arena);

    mps_ap_destroy(vm->amc->ap);
    mps_pool_destroy(vm->amc->pool);

    mps_ap_destroy(vm->amcz->ap);
    mps_pool_destroy(vm->amcz->pool);

    free(vm);
}