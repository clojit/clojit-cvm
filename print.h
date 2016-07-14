#ifndef _PRINT_H_
#define _PRINT_H_

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "slots.h"

void print_slot(uint64_t slot);
void print_slots(Slots* s);

#endif