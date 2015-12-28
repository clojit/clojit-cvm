#define __STDC_FORMAT_MACROS

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <arpa/inet.h>

#include "loader.h"

/*
(defcodec cljbc-format [(repeated {:section-id :int32
                                   :size :int32})
                        (repeated :int32)
                        (repeated :int64)
                        (repeated :float64)

                        {:offsets (repeated :int32)
                         :off     (repeated (string :utf-8))}

                        {:offsets (repeated :int32)
                         :off (repeated (string :utf-8))}
                        (repeated {:protocol :uint32
                                   :type :int16
                                   :fn-jump-offset :int32})
                        (repeated {:type-id :uint16 :size :int32})])
*/

struct parse_hdr {
	uint32_t sec_id;
	uint32_t sec_len;
} parse_hdr_instance;

struct vtable {
	uint32_t vtable_protocol;
	uint16_t vtable_type;
	uint32_t vtable_offset;
};

struct types {
	uint16_t types_id;
	uint32_t types_size;
};

int64_t swap_int64(int64_t in)
{
    uint64_t val = (uint64_t) in;
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    val = (val << 32) | ((val >> 32) & 0xFFFFFFFFULL);
    return (int64_t) val;
}

// *buf must outlive *sections
int parse(uint8_t *buf, struct sections *ret)
{
	size_t offset = 0;

	uint32_t hdr_size = ntohl(*(uint32_t*)(void*)buf);

    printf("hdr_size: %d\n", hdr_size);

	struct parse_hdr *hdr = (void*)(buf + HEADER_OFFSET);

    offset =  (hdr_size * sizeof(parse_hdr_instance)) + HEADER_OFFSET ;

	for (int i = 0; i < hdr_size; i++) {
	    printf("-----------------------\n");

        size_t len = ntohl(hdr[i].sec_len);
        uint32_t sec_id = ntohl(hdr[i].sec_id);

        printf("sec_len: %zd\n",    len);
        printf("sec_id:  %d\n", sec_id);

        uint32_t* section_cnt_ptr = (uint32_t*)(buf + offset);
        uint32_t cnt = ntohl(*section_cnt_ptr);
        printf("offset: %zd\n", offset);
        printf("element count at offset, cnt:  %d\n", cnt);

        uint32_t* section_data_ptr = (uint32_t*)(buf + offset + sizeof(uint32_t));

	switch(sec_id) {
		  case SECTION_INSTRUCTIONS:
		    printf("SECTION_INSTRUCTIONS\n");
		    ret->instr_cnt = cnt;
            ret->instr = (instr*)(void *) section_data_ptr;
			break;
		  case SECTION_CINT:
		    printf("SECTION_CINT\n");
		    ret->cint_cnt = cnt;
		    printf("section_data_ptr: %d\n",  ntohl(*section_data_ptr));
            uint32_t* section_data_ptr_2 = (uint32_t*)(buf + offset + sizeof(uint32_t) + sizeof(uint32_t));
            printf("section_data_ptr_2: %d\n",  ntohl(*section_data_ptr_2));
			ret->cint = (int64_t *)(void *)section_data_ptr;
			printf("ret->cint: %ld\n", swap_int64( *ret->cint));
			break;
		  case SECTION_CFLOAT:
		    printf("SECTION_CFLOAT\n");
		    ret->cfloat_cnt = cnt;
		    printf("section_data_ptr: %d\n",  ntohl(*section_data_ptr));
		    ret->cfloat = (float *)(void *)section_data_ptr;
			break;
		  case SECTION_CSTR:
		  printf("SECTION_CSTR\n"); break;
		  case SECTION_CKEY:
		  printf("SECTION_CKEY\n"); break;
		  case SECTION_VTABLES:
			fprintf(stderr, "ignoring section %x\n", hdr[i].sec_len);
			break;
		  default:
		    printf("default\n");
			return EINVAL;
		}

		offset = offset + len + sizeof(uint32_t);
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
