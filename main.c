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




