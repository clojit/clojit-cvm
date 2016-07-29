#include "builtin.h"

#include "debug.h"

void add_builtin_function(VM *vm) {


    if(debug_level > 1)
        fprintf(stderr,"------ BUILT IN FUNCTION: -------\n");
    uint64_t f = to_builtin( (builtin_fn) builtin_println);

    if(debug_level > 1)
        print_slot(f);
    if(debug_level > 1)
        fprintf(stderr,"\n------ BUILT IN FUNCTION: -------\n");
    add_symbol_table_pair(vm,"println",f);
}

void builtin_println(VM * vm) {
    println_slot( get(vm,2) );
    fprintf(stderr,"\n");
    set(vm,0,get_nil());
}


