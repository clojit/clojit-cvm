#ifndef _BUILTIN_H_
#define _BUILTIN_H_

#include "print.h"
#include "vm.h"

void add_builtin_function(VM *vm);
void builtin_println(VM * vm);

#endif