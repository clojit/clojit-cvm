#ifndef _SLOTS_H_
#define _SLOTS_H_

#include <inttypes.h>

#define SLOTS_INITIAL_CAPACITY 100

typedef union slot {
    double dbl;
    uintptr_t ptr;
    uint64_t raw;
} Slot;

// Define a slots type
typedef struct slots {
  uint32_t size;      // slots used so far
  uint32_t capacity;  // total available slots
  uint64_t *data;     // array of uint64_tegers we're storing
} Slots;

void slots_init(Slots *slots);

void slots_append(Slots *slots, uint64_t value);

uint64_t slots_get(Slots *slots, uint32_t index);

void slots_set(Slots *slots, uint32_t index, uint64_t value);

void slots_double_capacity_if_full(Slots *slots);

void slots_free(Slots *slots);

#endif