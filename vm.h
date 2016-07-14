#ifndef _VM_H_
#define _VM_H_

#include "mps.h"
#include "mpsavm.h"
#include "mpscamc.h"

typedef struct {
    mps_pool_t pool;
    mps_ap_t ap;
} Pool;

typedef struct vm {
    mps_arena_t arena;
    mps_chain_t obj_chain;
    mps_fmt_t obj_fmt;
    mps_thr_t thread;
    mps_root_t reg_root;

    Pool* amc;
    Pool* amcz;

    uint32_t pc;
    uint32_t ip;
    uint32_t base;

    Slots slots;
    Stack stack;

} VM;


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

/*static const char *OBJ_MPS_TYPE_NAMES[] = {
    "Padding", "Forward", "Object", "Array"
};*/

#define ARRAY_LEN(array)    (sizeof(array) / sizeof(array[0]))

/* LANGUAGE EXTENSION */

#define LENGTH(array)	(sizeof(array) / sizeof(array[0]))


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


/* -------------------------- Create Arena ----------------------- */
mps_res_t mps_create_vm_area(mps_arena_t *arena_o,
                                  size_t arenasize);
/* -------------------------- Scan Function ----------------------- */
static mps_res_t obj_scan(mps_ss_t ss, mps_addr_t base, mps_addr_t limit);
/* -------------------------- Skip Function ----------------------- */
static mps_addr_t obj_skip(mps_addr_t base);
/* -------------------------- FORWARD Function ----------------------- */
static void obj_fwd(mps_addr_t old,mps_addr_t new);
/* -------------------------- IS FORWARD Function ----------------------- */
static mps_addr_t obj_isfwd(mps_addr_t addr);
/* -------------------------- PADDING Function ----------------------- */
static void obj_pad(mps_addr_t addr, size_t size);
/* -------------------------- CREATE FORMAT ----------------------- */
mps_res_t mps_create_obj_fmt(mps_fmt_t *fmt_o, mps_arena_t _arena);
/* -------------------------- CREATE AMC POOL ----------------------- */
mps_res_t mps_create_amc_pool(mps_pool_t *pool_o, mps_fmt_t fmt_o,mps_chain_t obj_chain,mps_arena_t _arena);
/* -------------------------- CREATE AMCZ POOL ----------------------- */
mps_res_t mps_create_amcz_pool(mps_pool_t *pool_o, mps_fmt_t fmt_o,mps_chain_t obj_chain,mps_arena_t _arena);
/* ------------------------ CREATE ALLOCATION POOL --------------------- */
mps_res_t mps_create_ap(mps_ap_t *ap_o, mps_pool_t pool);


void vm_init(VM *vm, size_t arenasize);

void free_vm (VM *vm);

#endif