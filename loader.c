#define __STDC_FORMAT_MACROS

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <arpa/inet.h>

#include "uthash.h"

#include "loader.h"

struct parse_hdr {
	uint32_t sec_id;
	uint32_t sec_len;
} parse_hdr_instance;

struct vtable {
	uint32_t vtable_protocol;
	uint16_t vtable_type;
	uint32_t vtable_offset;
};

void add_vtable_record(struct sections* section, struct vtable_record *v) {
    HASH_ADD_INT(section->vtable, look_up_pair, v );
}

struct vtable_record *get_vtable_record(struct sections* section, uint64_t lup) {
    struct vtable_record *s;
    HASH_FIND_INT(section->vtable,&lup, s );
    return s;
}

void add_type_record(struct sections* section, struct type_record *v) {
    HASH_ADD_INT(section->types, type_id, v );
}

struct type_record *get_type_record(struct sections* section, uint32_t id) {
    struct type_record *s;
    HASH_FIND_INT(section->types,&id, s );
    return s;
}

double swap(double d)
{
   double a;
   unsigned char *dst = (unsigned char *)&a;
   unsigned char *src = (unsigned char *)&d;

   dst[0] = src[7];
   dst[1] = src[6];
   dst[2] = src[5];
   dst[3] = src[4];
   dst[4] = src[3];
   dst[5] = src[2];
   dst[6] = src[1];
   dst[7] = src[0];

   return a;
}

int64_t swap_int64(int64_t in) {
    uint64_t val = (uint64_t) in;
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    val = (val << 32) | ((val >> 32) & 0xFFFFFFFFULL);
    return (int64_t) val;
}

// *buf must outlive *sections
int parse(uint8_t *buf, struct sections *sec)
{
	size_t offset = 0;

	uint32_t hdr_size = ntohl(*(uint32_t*)(void*)buf);

    //printf("hdr_size: %d\n", hdr_size);

	struct parse_hdr *hdr = (void*)(buf + HEADER_OFFSET);

    offset =  (hdr_size * sizeof(parse_hdr_instance)) + HEADER_OFFSET ;
    //printf("offset: %zd\n", offset);

	for (int i = 0; i < hdr_size; i++) {
	    printf("-----------------------\n");

        size_t len = ntohl(hdr[i].sec_len);
        uint32_t sec_id = ntohl(hdr[i].sec_id);

        //printf("sec_len: %zd\n",len);
        //printf("sec_id:  %d\n", sec_id);

        uint32_t* section_cnt_ptr = (uint32_t*)(void *)(buf + offset);
        uint32_t cnt = ntohl(*section_cnt_ptr);

        //printf("element count at offset, cnt:  %d\n", cnt);

        uint32_t* section_data_ptr = (uint32_t*)(buf + offset + sizeof(uint32_t));

	    switch(sec_id) {
		  case SECTION_INSTRUCTIONS:
		    printf("SECTION_INSTRUCTIONS  %d\n", cnt);
		    sec->instr_cnt = cnt;
            sec->instr = (instr*)(void *) section_data_ptr;

            //printf("offset: %zd\n", offset);
            offset = offset + len + sizeof(uint32_t);
            //printf("offset: %zd\n", offset);

			break;
		  case SECTION_CINT:
		    printf("SECTION_CINT  %d\n", cnt);
		    sec->cint_cnt = cnt;
            sec->cint = (int64_t *)(void *)section_data_ptr;
			//printf("sec->cint: %ld\n", swap_int64( *sec->cint));

            //printf("offset: %zd\n", offset);
            offset = offset + len + sizeof(uint32_t);
            //printf("offset: %zd\n", offset);

            for(int j = 0;j < cnt; j++) {
                printf("int[%d]: %" PRId64 "\n",j, swap_int64(sec->cint[j]) );
            }

			break;
		  case SECTION_CFLOAT:
		    printf("SECTION_CFLOAT  %d\n", cnt);
		    sec->cfloat_cnt = cnt;
		    sec->cfloat = (double *)(void *)section_data_ptr;

            for(int j = 0;j < cnt; j++) {
                printf("float[%d]: %f\n",j, swap(sec->cfloat[j]));
            }

            //printf("offset: %zd\n", offset);
            offset = offset + len + sizeof(uint32_t);
            //printf("offset: %zd\n", offset);
			break;
		  case SECTION_CSTR:
		    printf("SECTION_CSTR  %d\n", cnt);
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

                   printf("str[%d] (address %p) : %s\n",j, strptr[j] , strptr[j]);
            }
            sec->cstr_cnt = cnt;
            sec->cstr = strptr;

            //printf("offset: %zd\n", offset);
            uint32_t bytesOfStr = ntohl(*(start_of_index + (cnt-1)));
            offset = offset + bytesOfStr  + sizeof(uint32_t) + sizeof(uint32_t) + (cnt * sizeof(uint32_t));
            //printf("offset: %zd\n", offset);

		    break;
		  case SECTION_CKEY:
		    printf("SECTION_CKEYR  %d\n", cnt);
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

                   printf("key[%d]: %s\n",j, keyptr[j]);
            }
            sec->ckey_cnt = cnt;
            sec->ckey     = keyptr;

            //printf("offset: %zd\n", offset);
            uint32_t bytesOfKeys = 0;

            if(cnt != 0)
              bytesOfKeys = ntohl(*(keys_start_of_index + (cnt-1)));

            offset = offset +
                     bytesOfKeys  +
                     sizeof(uint32_t) + sizeof(uint32_t) + (cnt * sizeof(uint32_t));
            //printf("offset: %zd\n", offset);
            break;
		  case SECTION_VTABLES:
            printf("SECTION_VTABLES  %d\n", cnt);
            sec->vtable = NULL;
            sec->vtable_cnt = cnt;
            for(int j = 0;j < cnt; j++) {

              uint32_t jump_offset = ntohl(*section_data_ptr);
              uint32_t protocol_nr = ntohl(*(section_data_ptr + 1));
              uint32_t type_nr = ntohl(*(section_data_ptr + 2));

              printf("protocol_nr: %d type_nr: %d  jump_offset: %d\n",protocol_nr, type_nr,jump_offset);
              uint64_t look_up_pair = (uint64_t) protocol_nr << 32 |  (uint64_t) type_nr;

              struct vtable_record *record = malloc(sizeof(struct vtable_record));
              record->look_up_pair = look_up_pair;
              record->jump_offset = jump_offset;

              add_vtable_record(sec,record);

              section_data_ptr = section_data_ptr + 3;
            }

            //printf("offset: %zd\n", offset);
            offset = offset + len + sizeof(uint32_t);
            //printf("offset: %zd\n", offset);

			break;
		  case SECTION_TYPES:
            printf("SECTION_TYPES  %d\n", cnt);

            sec->types = NULL;
            sec->types_cnt = cnt;
            for(int j = 0;j < cnt; j++) {
              uint32_t t_id = ntohl(*section_data_ptr);
              uint32_t type_size = ntohl(*(section_data_ptr + 1));
              printf("type_id: %d type_size: %d\n", t_id, type_size);
              struct type_record *record = malloc(sizeof(struct type_record));
              record->type_id = t_id;
              record->type_size = type_size;
              add_type_record(sec,record);
              section_data_ptr = section_data_ptr + 2;
            }
            break;
		  default:
		    printf("default\n");
			return EINVAL;
		}
	}

    /*printf("-----------end of loader ---------------t\n");
    for(int j = 0;j < 2; j++) {
        printf("float[%d]: %p\n",j, sec->cfloat + j);
    }
    printf("-----------end of loader ---------------t\n");*/

	return 0;
}
              //struct vtable_record *rec = find_table(ret, look_up_pair);
              //uint32_t o = rec->jump_offset;

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
