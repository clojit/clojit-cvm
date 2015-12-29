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

int64_t swap_int64(int64_t in) {
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
        printf("sec len:%d -  sec id:%d:\n",
                ntohl(hdr[i].sec_len),
                ntohl(hdr[i].sec_id));
    }

    printf("offset: %zd\n", offset);

	for (int i = 0; i < hdr_size; i++) {
	    printf("-----------------------\n");

        size_t len = ntohl(hdr[i].sec_len);
        uint32_t sec_id = ntohl(hdr[i].sec_id);

        printf("sec_len: %zd\n",len);
        printf("sec_id:  %d\n", sec_id);

        uint32_t* section_cnt_ptr = (uint32_t*)(void *)(buf + offset);
        uint32_t cnt = ntohl(*section_cnt_ptr);

        printf("element count at offset, cnt:  %d\n", cnt);

        uint32_t* section_data_ptr = (uint32_t*)(buf + offset + sizeof(uint32_t));

	    switch(sec_id) {
		  case SECTION_INSTRUCTIONS:
		    printf("SECTION_INSTRUCTIONS\n");
		    ret->instr_cnt = cnt;
            ret->instr = (instr*)(void *) section_data_ptr;

            printf("offset: %zd\n", offset);
            offset = offset + len + sizeof(uint32_t);
            printf("offset: %zd\n", offset);

			break;
		  case SECTION_CINT:
		    printf("SECTION_CINT\n");
		    ret->cint_cnt = cnt;
            ret->cint = (int64_t *)(void *)section_data_ptr;
			//printf("ret->cint: %ld\n", swap_int64( *ret->cint));

            printf("offset: %zd\n", offset);
            offset = offset + len + sizeof(uint32_t);
            printf("offset: %zd\n", offset);

			break;
		  case SECTION_CFLOAT:
		    printf("SECTION_CFLOAT\n");
		    ret->cfloat_cnt = cnt;
		    ret->cfloat = (double *)(void *)section_data_ptr;

            printf("offset: %zd\n", offset);
            offset = offset + len + sizeof(uint32_t);
            printf("offset: %zd\n", offset);
			break;
		  case SECTION_CSTR:
		    printf("SECTION_CSTR\n");
            uint32_t* start_of_index = (uint32_t*)(void *)section_data_ptr;
            uint32_t* start_of_character_data_32 = start_of_index + cnt + 1; // 1 for HEADER_SIZE of character section
            char*  start_of_character_data_8 = (char*)(void *)start_of_character_data_32;

            char **strptr;
            strptr = malloc(cnt*sizeof(char*));

            for(int j = 0;j < cnt; j++) {
                   uint32_t charskip = 0;
                   if(j != 0)
                        charskip = ntohl(*(start_of_index + j-1));

                   strptr[j] = start_of_character_data_8 + charskip;
            }
            ret->cstr_cnt = cnt;
            ret->cstr = strptr;

            printf("offset: %zd\n", offset);
            uint32_t bytesOfStr = ntohl(*(start_of_index + (cnt-1)));
            offset = offset + bytesOfStr  + sizeof(uint32_t) + sizeof(uint32_t) + (cnt * sizeof(uint32_t));
            printf("offset: %zd\n", offset);

		    break;
		  case SECTION_CKEY:
		    printf("SECTION_CKEYR\n");
            uint32_t* keys_start_of_index = (uint32_t*)(void *)section_data_ptr;
            uint32_t* keys_start_of_character_data_32 = keys_start_of_index + cnt + 1; // 1 for HEADER_SIZE of character section
            uint8_t*  keys_start_of_character_data_8 = (uint8_t*)(void *)keys_start_of_character_data_32;

            char **keyptr;
            keyptr = malloc(cnt*sizeof(char*));

            for(int j = 0;j < cnt; j++) {
                   uint32_t charskip = 0;
                   if(j != 0)
                        charskip = ntohl(*(keys_start_of_index + j-1));

                   keyptr[j] = keys_start_of_character_data_8 + charskip;
            }
            ret->ckey_cnt = cnt;
            ret->ckey     = keyptr;

            printf("offset: %zd\n", offset);
            uint32_t bytesOfKeys = ntohl(*(keys_start_of_index + (cnt-1)));
            offset = offset + bytesOfKeys  + sizeof(uint32_t) + sizeof(uint32_t) + (cnt * sizeof(uint32_t));
            printf("offset: %zd\n", offset);

		  case SECTION_VTABLES:

			fprintf(stderr, "ignoring section %x\n", hdr[i].sec_len);
			 break;
		  default:
		    printf("default\n");
			return EINVAL;
		}
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
