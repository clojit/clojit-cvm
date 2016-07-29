#ifndef _SLOTS_H_
#define _SLOTS_H_

#include <inttypes.h>
#include <stdbool.h>

#include "mps.h"
#include "stack.h"


extern const uint16_t TAG_DOUBLE_MAX;
extern const uint16_t TAG_DOUBLE_MIN;
extern const uint16_t TAG_POINTER_LO;
extern const uint16_t TAG_POINTER_HI;
extern const uint16_t TAG_SMALL_INTEGER;
extern const uint16_t TAG_BOOL;
extern const uint16_t TAG_TYPE;
extern const uint16_t TAG_FNEW;

#define SLOTS_INITIAL_CAPACITY 10
#define VAL_BITS (48u)
#define TAG_MASK (0xFFFFul << VAL_BITS)

uint16_t tag(uint64_t slot);

bool is_pointer(uint64_t slot);

typedef void (*builtin_fn)(void *vm);  //TODO GANDRO VM, get this to work 'typedef void (*builtin_fn)(VM *vm);'

typedef union slot {
    double dbl;
    uintptr_t ptr;
    uint64_t raw;
} Slot;

// Define a slots type
typedef struct slots {
  uint32_t size;      // slots used so far
  uint32_t capacity;  // total available slots
  mps_root_t root_o;
  mps_arena_t arena;
  uint64_t *data;     // array of uint64_tegers we're storing
} Slots;

void slots_init(Slots *slots, mps_arena_t arena);

void slots_append(Slots *slots, uint64_t value);

uint64_t slots_get(Slots *slots, uint32_t index);
uint64_t* slots_get_ptr(Slots *slots, uint32_t index);

void slots_set(Slots *slots, uint32_t index, uint64_t value);

void slots_double_capacity_if_full(Slots *slots);
void slots_half_capacity(Slots *slots);

void slots_free(Slots *slots);


//////////////////////////SLOT FUNCTION//////////////////////////

uint64_t invert_non_negative(uint64_t slot);
uint16_t tag(uint64_t slot);
// ---------------- Double Function ----------------
double get_double(uint64_t slot);
uint64_t to_double(double num);
bool is_double(uint64_t slot);
// ---------------- Type Function ----------------
uint64_t to_type(uint16_t type);
bool is_type(uint64_t slot);
uint16_t get_type(uint64_t slot);
// ---------------- Builtin Function --------------
uint64_t to_builtin(builtin_fn fn);
bool is_builtin(uint64_t slot);
builtin_fn get_builtin(uint64_t slot);
// ---------------- VFUNC Function  ----------------
uint64_t to_vfunc(int16_t type);
bool is_vfunc(uint64_t slot);
int16_t get_vfunc(uint64_t slot);
// ---------------- FNEW Function  ----------------
uint64_t to_fnew(int16_t type);
bool is_fnew(uint64_t slot);
int16_t get_fnew(uint64_t slot);
// ---------------- Int Function ----------------
uint64_t to_small_int(int32_t num);
bool is_small_int(uint64_t slot);
int32_t get_small_int(uint64_t slot);
// ---------------- Nil Function ----------------
bool is_nil(uint64_t slot);
uint64_t get_nil(void);
// ---------------- Bool Function ----------------
bool is_bool(uint64_t slot);
bool get_bool(uint64_t slot);
uint64_t to_bool(bool value);
// ---------------- Truthy/Falsy Function ----------------
bool is_falsy(uint64_t slot);
bool is_truthy(uint64_t slot);

#endif