#ifndef _LOADER_H_
#define _LOADER_H_

#include "uthash.h"

typedef uint32_t instr;

struct vtable_record {
    uint64_t look_up_pair;
    uint32_t jump_offset;
    UT_hash_handle hh;
};

struct type_record {
    uint32_t type_id;
    uint32_t type_size;
    UT_hash_handle hh;
};

struct symbol_table_record {
    const char *symbol;          /* key */
    uint64_t number;
    UT_hash_handle hh;         /* makes this structure hashable */
};

struct sections {
	instr *instr;
	size_t instr_cnt;

	int64_t *cint;
	size_t cint_cnt;

	double *cfloat;
	size_t cfloat_cnt;
	
	char **cstr;
	size_t cstr_cnt;

	char **ckey;
	size_t ckey_cnt;

    struct vtable_record *vtable;
    size_t vtable_cnt;

    struct type_record *types;
    size_t types_cnt;

    struct symbol_table_record *symbol_table;
};

#define HEADER_OFFSET	4

#define SECTION_INSTRUCTIONS	0
#define SECTION_CINT		1
#define SECTION_CFLOAT		2
#define SECTION_CSTR		3
#define SECTION_CKEY		4
#define SECTION_VTABLES		5
#define SECTION_TYPES		6

int parse(uint8_t *buf, struct sections *ret);
int loadfile(const char *filename, struct sections *ret);

#endif /* _LOADER_H_*/
