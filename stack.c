// stack.c

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "stack.h"

void stack_init(Stack *stack) {
  stack->top = 0;
  stack->capacity = STACK_INITIAL_CAPACITY;
  stack->data = malloc(sizeof(Context) * stack->capacity);
}

void push(Stack *stack, Context ctx) {
  stack_double_capacity_if_full(stack);
  stack->data[stack->top++] = ctx;
}

Context pop(Stack *stack) {
    stack->top--;
    if(stack->top < 0) {
        printf("Index %d out of bounds for stack of top %d\n",stack->top,stack->top);
        exit(1);
    }
    return stack->data[stack->top];
}

void stack_free(Stack *stack) {
  free(stack->data);
}

void stack_double_capacity_if_full(Stack *stack) {
  if (stack->top >= stack->capacity) {
    // double stack->capacity and resize the allocated memory accordingly
    stack->capacity *= 2;
    stack->data = realloc(stack->data, sizeof(Context) * stack->capacity);
  }
}
