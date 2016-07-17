#include "builtin.h"

#include "debug.h"

void add_builtin_function(VM *vm) {


    if(debug_level > 0)
        printf("------ BUILT IN FUNCTION: -------\n");
    uint64_t f = to_builtin( (builtin_fn) builtin_println);

    if(debug_level > 0)
        print_slot(f);
    if(debug_level > 0)
        printf("\n------ BUILT IN FUNCTION: -------\n");
    add_symbol_table_pair(vm,"println",f);
}

void builtin_println(VM * vm) {
    println_slot( get(vm,2) );
    printf("\n");
    set(vm,0,get_nil());
}


