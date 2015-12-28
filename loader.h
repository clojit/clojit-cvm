#ifndef _LOADER_H_
#define _LOADER_H_

typedef uint32_t instr;

struct sections {
	instr *instr;
	size_t instr_cnt;

	int64_t *cint;
	size_t cint_cnt;

	float *cfloat;
	size_t cfloat_cnt;
	
	uint32_t *cstr;
	size_t cstr_cnt;

	uint32_t *ckey;
	size_t ckey_cnt;

	// TODO vtable
};

#define HEADER_OFFSET	4

/*#define SECTION_INSTRUCTIONS	0x100
#define SECTION_CINT		0x200
#define SECTION_CFLOAT		0x300
#define SECTION_CSTR		0x400
#define SECTION_CKEY		0x500
#define SECTION_VTABLES		0x600
#define SECTION_TYPES		0x700*/

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
