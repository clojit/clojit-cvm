#ifndef _ALLOC_H_
#define _ALLOC_H_

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "mps.h"
#include "vm.h"

struct obj_stub {
    uint8_t type;
    uint8_t _;
    uint16_t cljtype;
    uint32_t size; // incl. header
    mps_addr_t ref[];
} __attribute__((packed));


mps_res_t mps_alloc_obj(mps_addr_t *addr_o,
                        mps_ap_t ap,
                        uint32_t size,
                        uint16_t cljtype,
                        uint8_t mpstype);

#endif