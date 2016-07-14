/*#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "mps.h"
#include "mpsavm.h"
#include "mpscamc.h"
#include "uthash.h"
#include "loader.h"
#include "stack.h"
#include "slots.h"*/


#include "mps.h"
#include "execute.h"

int main(int argc, char **argv) {

    int exit_code;

    if(argc > 1) {
        exit_code = start(argv[1]);
    } else {
        exit_code = MPS_RES_OK;
    }

    return exit_code;

}




