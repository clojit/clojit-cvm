// slots.c

#include <stdio.h>
#include <stdlib.h>
#include "slots.h"

void slots_init(Slots *slots) {
  // initialize size and capacity
  slots->size = 0;
  slots->capacity = SLOTS_INITIAL_CAPACITY;

  // allocate memory for slots->data
  slots->data = malloc(sizeof(uint64_t) * slots->capacity);
}

void slots_append(Slots *slots, uint64_t value) {
  slots_double_capacity_if_full(slots);
  slots->data[slots->size++] = value;
}

uint64_t slots_get(Slots *slots, uint32_t index) {
  if (index >= slots->size || index < 0) {
    printf("Index %d out of bounds for slots of size %d\n", index, slots->size);
    exit(1);
  }
  return slots->data[index];
}

void slots_set(Slots *slots, uint32_t index, uint64_t value) {
  while (index >= slots->size) {
    slots_append(slots, 0);
  }

  slots->data[index] = value;
}

void slots_double_capacity_if_full(Slots *slots) {
  if (slots->size >= slots->capacity) {
    slots->capacity *= 2;
    slots->data = realloc(slots->data, sizeof(uint64_t) * slots->capacity);
  }
}

void slots_free(Slots *slots) {
  free(slots->data);
}