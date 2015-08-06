#include "mps.h"
#include "mpsavm.h"
#include "mpscamc.h"

int main(int argc, char *argv[])
{
    mps_arena_t arena;
    size_t arenasize = 32 << 20;
    mps_res_t res;

    MPS_ARGS_BEGIN(args) {
        MPS_ARGS_ADD(args, MPS_KEY_ARENA_SIZE, arenasize);
        res = mps_arena_create_k(&arena, mps_arena_class_vm(),  args);
    } MPS_ARGS_END(args);

    if (res != MPS_RES_OK)
        return 1;

    mps_arena_destroy(arena);

    return 0;
}
