#include "builtin.h"

void add_builtin_function(void *vm) {

    printf("------ BUILT IN FUNCTION: -------\n");
    print_slot(to_builtin(builtin_println));

    printf("\n------ BUILT IN FUNCTION: -------\n");
    add_symbol_table_pair(vm,"println",to_builtin(builtin_println));
}

void builtin_println(void * vm) {

    VM * cvm = (VM *) vm;

    printf("%s",get(cvm, 2));
    set(cvm, 0,get_nil());
}

