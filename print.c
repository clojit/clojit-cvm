#include "print.h"
#include "debug.h"

void print_slot (uint64_t slot) {

    if( is_small_int(slot) ) {
        printf("i%d", get_small_int(slot));
        return;
    }

    if(is_nil(slot)) {
        printf("n0");
        return;
    }

    if(is_fnew(slot)) {
        printf("fn%d", get_fnew(slot));
        return;
    }

    if(is_builtin(slot)) {
        printf("bfn%d", get_builtin(slot));
        return;
    }

    if(is_type(slot)) {
        printf("t%d", get_type(slot));
        return;
    }

    if(is_pointer(slot)) {
        //printf("P");

        uint64_t* header_ptr = (uint64_t*) slot;

        struct obj_stub  *obj = (struct obj_stub *) (void *) header_ptr;

        //printf(" pheader: %016"PRIx64" ", header);

        //< mpstype: %02"PRIx64", _: %02"PRIx64" cljtype: %04"PRIx64", size: %08"PRIx64">
        printf("( [cljtype: %d, size: %d] ", obj->cljtype,obj->size );

        for(int i = 0; i != ((obj->size / 8) - 1); i++) {
            //printf(" <field %i: %016"PRIx64"> ", i,  obj->ref[i] );
            print_slot( (uintptr_t) (void *) obj->ref[i]);
        }

        printf(") ");
        return;
    }

    if( is_double(slot) ) {
        printf("f%.2f", get_double(slot));
        return;
    }

    printf("-");
}

// ------------------------- Print Slots ------------------------

void print_slots(Slots* s, uint64_t base) {
    int index = 0;

    //printf("Slots<s:%d/b:%d>: ", s->size, base_slot);
    while (1) {

        if(base == index)
            printf("| ");

        if ( !(index >= s->size || index < 0) ) {
            //printf("<%d:",index);
            print_slot(slots_get(s, index));

            printf(" ");
        }

        index++;

        if(index == s->size) {
            printf("\n");
            return;
        }
    }
}

void println_slot (uint64_t slot) {

    if( is_small_int(slot) ) {
        printf("%d", get_small_int(slot));
        return;
    }

    if(is_nil(slot)) {
        printf("nil");
        return;
    }

    if(is_fnew(slot)) {
        printf("fn%d", get_fnew(slot));
        return;
    }

    if(is_builtin(slot)) {
        printf("bfn%d", get_builtin(slot));
        return;
    }

    if(is_type(slot)) {
        printf("t%d", get_type(slot));
        return;
    }

    if(is_pointer(slot)) {
        //printf("P");

        uint64_t* header_ptr = (uint64_t*) slot;

        struct obj_stub  *obj = (struct obj_stub *) (void *) header_ptr;

        //printf(" pheader: %016"PRIx64" ", header);

        //< mpstype: %02"PRIx64", _: %02"PRIx64" cljtype: %04"PRIx64", size: %08"PRIx64">
        printf("( [cljtype: %d, size: %d] ", obj->cljtype,obj->size );

        for(int i = 0; i != ((obj->size / 8) - 1); i++) {
            //printf(" <field %i: %016"PRIx64"> ", i,  obj->ref[i] );
            print_slot( (uintptr_t) (void *) obj->ref[i]);
        }

        printf(") ");
        return;
    }

    if( is_double(slot) ) {
        printf("f%", get_double(slot));
        return;
    }
}



/*
static void rust_mps_debug_print_formatted_object(mps_addr_t addr,
                                            mps_fmt_t fmt,
                                            mps_pool_t pool,
                                            void *p, size_t s) {
    assert(p == fmt);
    struct obj_stub *obj = addr;
    assert(obj->type < ARRAY_LEN(OBJ_MPS_TYPE_NAMES));

    const char *mps_type = OBJ_MPS_TYPE_NAMES[obj->type];
    fprintf(stderr, "%s(0x%012"PRIxPTR") [%"PRIu32" bytes] ", mps_type, (uintptr_t)addr, obj->size);
    if (obj->type == OBJ_MPS_TYPE_OBJECT || obj->type == OBJ_MPS_TYPE_ARRAY) {
        fprintf(stderr, "[type: %"PRIu16"]", obj->cljtype);
    }
    fprintf(stderr, "\n");

    if (obj->type == OBJ_MPS_TYPE_OBJECT) {
        size_t count = (obj->size - HEADER_SIZE) / WORD_SIZE;
        for (size_t i=0; i<count; i++) {
            uint16_t tag = ((uintptr_t)obj->ref[i] & TAG_MASK) >> VAL_BITS;
            uint64_t val = (uintptr_t)obj->ref[i] & ~TAG_MASK;

            fprintf(stderr, "  0x%04"PRIx16":%012"PRIx64"\n", tag, val);
        }
    }
}

void rust_mps_debug_print_reachable(mps_arena_t _arena, mps_fmt_t fmt) {
    fprintf(stderr, "==== Walking Reachable Objects ====\n");
    mps_arena_formatted_objects_walk(_arena, rust_mps_debug_print_formatted_object, fmt, 0);
}*/
