#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "loader.h"

struct parse_hdr {
	uint32_t sec_id;
	uint32_t sec_len;
};

// *buf must outlive *sections
int parse(uint8_t *buf, struct sections *ret)
{
	size_t offset = 0;
	uint32_t hdr_size = *(uint32_t*)(void*)buf;
	struct parse_hdr *hdr = (void*)(buf + HEADER_OFFSET);
	
	for (int i = 0; i < hdr_size; i++) {
		size_t len = hdr[i].sec_len;
		switch(hdr[i].sec_id) {
		case SECTION_INSTRUCTIONS:
			ret->instr = (instr*)(buf + offset);
			ret->instr_cnt = len / sizeof(instr);
			break;
		case SECTION_CINT:
			ret->cint = (int64_t*)(buf + offset);
			ret->cint_cnt = len / sizeof(int64_t);
			break;
		case SECTION_CFLOAT:
			ret->cfloat = (float*)(buf + offset);
			ret->cfloat_cnt = len / sizeof(float);
			break;
		case SECTION_CSTR:
		case SECTION_CKEY:
		case SECTION_VTABLES:
			fprintf(stderr, "ignoring section %x\n", hdr[i].sec_len);
			break;
		default:
			return EINVAL;
		}

		offset += hdr[i].sec_len;
	}
	return 0;
}

int loadfile(const char *filename, struct sections *ret)
{
	uint8_t *buf;
	int fd, err;
	struct stat st;

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		return errno;
	}
	
	err = fstat(fd, &st);
	if (err == -1) {
		return errno;
	}

	buf = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (buf == NULL) {
		return errno;
	}
	
	err = parse(buf, ret);
	if (err) {
		return errno;
	}
	
	return 0;
}
