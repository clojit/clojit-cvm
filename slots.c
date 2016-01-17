// slots.c

#include <stdio.h>
#include <stdlib.h>

#include "slots.h"

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

    if (res != MPS_RES_OK) printf("Couldn't create slots roots");

    // allocate memory for slots->data

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

        printf("Doubling Slots capacity is not possible at the moment");
        exit(0);

        slots->capacity *= 2;
        slots->data = realloc(slots->data, sizeof(uint64_t) * slots->capacity);

        /*mps_root_t root_o_old;
        root_o_old = slots->root_o;

        mps_root_t root_o_new;
        mps_res_t res = mps_root_create_table_masked(&root_o_new,
                                                     arena,
                                                     mps_rank_exact(),
                                                     (mps_rm_t)0,
                                                     (mps_addr_t *)slots->data,
                                                     slots->capacity,
                                                     (mps_word_t)TAG_MASK);
        if (res != MPS_RES_OK) printf("Couldn't increase slots roots");

        slots->root_o = root_o_new;

        mps_root_destroy(root_o_old);*/

    }

}

void slots_free(Slots *slots) {
  free(slots->data);
}