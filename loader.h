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
    char *symbol;          /* key */
    uint64_t number;
    UT_hash_handle hh;         /* makes this structure hashable */
};

struct context {
    uint32_t pc;
    uint32_t ip;
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

double swap(double d);
int64_t swap_int64(int64_t in);
struct type_record *get_type_record(struct sections* section, uint32_t id);
void add_type_record(struct sections* section, struct type_record *v);
struct vtable_record *get_vtable_record(struct sections* section, uint64_t lup);
void add_vtable_record(struct sections* section, struct vtable_record *v);



#endif /* _LOADER_H_*/
