#ifndef _EXECUTE_H_
#define _EXECUTE_H_

#include <inttypes.h>
#include <arpa/inet.h>

#include "slots.h"
#include "stack.h"
#include "alloc.h"
#include "print.h"
#include "namespace.h"


#define CSTR  0
#define CKEY  1
#define CINT  2
#define CFLOAT  3
#define CTYPE   4
#define CBOOL  5
#define CNIL  6
#define CSHORT  7
#define SETF  8
#define NSSET  9
#define NSGET  10
#define ADDVV  11
#define SUBVV  12
#define MULVV  13
#define DIVVV  14
#define POWVV  15
#define MODVV  16
#define ISLT  17
#define ISGE  18
#define ISLE  19
#define ISGT  20
#define ISEQ  21
#define ISNEQ  22
#define MOV  23
#define NOT  24
#define NEG  25
#define JUMP  26
#define JUMPF 27
#define JUMPT  28
#define CALL  29
#define RET  30
#define APPLY  31
#define FNEW  32
#define VFNEW  33
#define GETFREEVAR  34
#define UCLO  35
#define LOOP  36
#define BULKMOV 37
#define NEWARRAY 38
#define GETARRAY 39
#define SETARRAY 40
#define FUNCF 41
#define FUNCV 42
#define ALLOC 43
#define SETFIELD 44
#define GETFIELD 45
#define BREAK 46
#define EXIT 47
#define DROP 48
#define TRANC 49

struct OpABC {
    uint8_t op;
    uint8_t a;
    uint8_t b;
    uint8_t c;
} __attribute__((packed));

struct OpAD {
    uint8_t op;
    uint8_t a;
    uint16_t d;
} __attribute__((packed));


typedef uint32_t instr;



int start(char *file);

#endif