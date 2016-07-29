#include "alloc.h"

// caller needs to make sure to root addr_o before calling this!
// size is the size in bytes (including header)
mps_res_t mps_alloc_obj(mps_addr_t *addr_o,
                        mps_ap_t ap,
                        uint32_t size,
                        uint16_t cljtype,
                        uint8_t mpstype) {

    assert(addr_o != NULL);
    assert(size > HEADER_SIZE);

    mps_res_t res;

    do {
        res = mps_reserve(addr_o, ap, size);
        if (res != MPS_RES_OK) return res;
        struct obj_stub *obj = *addr_o;

        obj->type = mpstype;
        obj->cljtype = cljtype;
        obj->size = size;

        // zero all fields
        memset(obj->ref, 0, size - HEADER_SIZE);

    } while (!mps_commit(ap, *addr_o, size));

    return res;
}