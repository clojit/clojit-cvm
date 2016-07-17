#include "builtin.h"

void void add_builtin_function(VM *vm) {

    add_symbol_table_pair(vm,"println",println);

}

void println(void * vm) {

    VM * cvm = (VM *) vm;

    printf("%s",get(cvm, 2));
    set(cvm, 0,get_nil());
}


