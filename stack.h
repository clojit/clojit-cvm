#ifndef _STACK_H_
#define _STACK_H_

#define STACK_INITIAL_CAPACITY 10

typedef struct {
    uint32_t base_slot;
    uint32_t ip;
} Context;

typedef struct {
  int top;
  int capacity;
  Context* data;
} Stack;

void stack_init(Stack *stack);

void push(Stack *stack, Context value);
Context pop(Stack *stack);

void stack_free(Stack *stack);
void stack_double_capacity_if_full(Stack *stack);

#endif