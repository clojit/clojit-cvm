
#include "execute.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "vm.h"
#include "mps.h"
#include "mpsavm.h"
#include "loader.h"


static int start(char *file) {
    printf("start\n");
    struct sections sec = {0};

    mps_res_t res;

    res = loadfile(file, &sec);
    if (res != MPS_RES_OK) printf("Couldn't load file");

    sec.symbol_table = NULL;

    VM vm = {0};

    size_t arenasize = 32ul * 1024 * 1024;

    vm_init(&vm, arenasize);

    /*
    printf("------------SLOT--------------\n");

    while (1) {
        instr inst = sec.instr[pc];
        //printf("------------INSTURCTION --------------\n");
        //printf("\n");

        struct OpABC abc = *((struct OpABC *) &inst);
        struct OpAD  ad  = *((struct OpAD *) &inst);

        uint8_t op = abc.op;

        switch (op) {
            //------------------Constant Table Value Operation------------------
            case CSTR: {
                uint16_t d = ntohs(ad.d);
                printf("CSTR: %d %d\n", ad.a, d);

                slots_set( &slots,(base_slot + ad.a), (uint64_t)(void*)sec.cstr[d]);

                pc++;
                break;
            }
            case CKEY: {
                uint16_t d = ntohs(ad.d);

                printf("CKEY: %d %d\n", ad.a, d);
                slots_set(&slots, (base_slot + ad.a) , (uint64_t)(void*)sec.cstr[d]);

                pc++;
                break;
            }
            //case CINT: {
            //    uint16_t d = ntohs(ad.d);
            //    printf("CINT: %d %d const: %d\n", ad.a, d, sec->cint[d]);

            //    slots.data[base + ad.a] = sec->cint[d];
            //    break;
            //}
            //case CFLOAT: {
            //    int target_slot = ad.a;
            //    printf("sec.cfloat[ad.d]: %f\n", swap(sec.cfloat[0]) );

            //    printf("CFLOAT: %d %d\n", ad.a, ad.d);
            //    break;
            //}
            case CTYPE: {
                uint16_t d = ntohs(ad.d);
                printf("CTYPE: %d %d\n", ad.a, d);

                slots_set(&slots,(base_slot + ad.a),to_type(d));

                pc++;
                break;
            }
            //------------------Constant Value Operation------------------
            case CBOOL: {
                uint16_t d = ntohs(ad.d);
                printf("CBOOL: %d %d\n", ad.a, d);
                slots_set(&slots,(base_slot + ad.a),to_bool(d));
                pc++;
                break;
            }
            case CNIL : {
                printf("CNIL: %d\n", ad.a);
                slots_set(&slots,(base_slot +  ad.a),get_nil());
                pc++;
                break;
            }
            case CSHORT: {
                int target_slot = ad.a;
                uint16_t d16 = ntohs(ad.d);
                uint64_t d = (uint64_t) d16;

                printf("CSHORT: %d %d\n", ad.a, d16);
                slots_set(&slots,(base_slot + target_slot),to_small_int(d));

                pc++;
                break;
            }
            //------------------Global Table Ops------------------
            case NSSET: {
                uint16_t d = ntohs(ad.d);
                printf("NSSET: %d %d\n", ad.a, d);
                add_symbol_table_pair(&sec,sec.cstr[d],
                                      slots_get(&slots,(base_slot + ad.a)));

                pc++;
                break;
            }
            case NSGET: {
                uint16_t d = ntohs(ad.d);
                printf("NSGET: %d %d\n", ad.a, d);
                slots_set(&slots,(base_slot + ad.a),get_symbol_table_record(&sec,sec.cstr[d]));
                pc++;
                break;
            }
            //------------------Variable Slots------------------
            case ADDVV: {
                int target_slot = base_slot + abc.a;
                uint64_t bslot = slots_get(&slots, (base_slot + abc.b));
                uint64_t cslot = slots_get(&slots, (base_slot +abc.c));

                printf("ADDVV: %d %d %d\n", abc.a, abc.b, abc.c);

                if (is_small_int(bslot) && is_small_int(cslot))
                    slots_set(&slots,target_slot,to_small_int(get_small_int(bslot) + get_small_int(cslot)));

                if (is_double(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(bslot) + get_double(cslot)));

                if (is_double(bslot) && is_small_int(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(bslot) + get_small_int(cslot)));

                if (is_small_int(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(cslot) + get_small_int(bslot)));

                pc++;
                break;
            }
            case SUBVV: {
                int target_slot = base_slot + abc.a;
                uint64_t bslot = slots_get(&slots, (base_slot + abc.b));
                uint64_t cslot = slots_get(&slots, (base_slot +abc.c));

                printf("SUBVV: %d %d %d\n", abc.a, abc.b, abc.c);

                if (is_small_int(bslot) && is_small_int(cslot) )
                    slots_set(&slots,target_slot, to_small_int(get_small_int(bslot) - get_small_int(cslot)));

                if (is_double(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot, to_double(get_double(bslot) - get_double(cslot)));

                if (is_double(bslot) && is_small_int(cslot))
                    slots_set(&slots,target_slot, to_double(get_double(bslot) - get_small_int(cslot)));

                if (is_small_int(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(cslot) - get_small_int(bslot)));

                pc++;
                break;
            }
            case MULVV: {
                int target_slot = base_slot + abc.a;
                uint64_t bslot = slots_get(&slots, (base_slot + abc.b));
                uint64_t cslot = slots_get(&slots, (base_slot +abc.c));

                printf("MULVV: %d %d %d\n", abc.a, abc.b, abc.c);

                if (is_small_int(bslot) && is_small_int(cslot))
                    slots_set(&slots,target_slot,to_small_int(get_small_int(bslot) * get_small_int(cslot)));

                if (is_double(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(bslot) * get_double(cslot)));

                if (is_double(bslot) && is_small_int(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(bslot) * get_small_int(cslot)));

                if (is_small_int(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(cslot) * get_small_int(bslot)));

                pc++;
                break;
            }
            case MODVV: {

                int target_slot = base_slot + abc.a;
                uint64_t bslot = slots_get(&slots, (base_slot + abc.b));
                uint64_t cslot = slots_get(&slots, (base_slot + abc.c));

                printf("MODVV: %d %d %d\n", abc.a, abc.b, abc.c);

                if (is_small_int(bslot) && is_small_int(cslot)) {
                    slots_set(&slots,target_slot, to_small_int(get_small_int(bslot) % get_small_int(cslot)));
                } else {
                    printf("Type Error. Called Modulo with Float\n");
                    exit(1);
                }

                pc++;
                break;
            }
            case DIVVV: {
                printf("DIVVV: %d %d %d\n", abc.a, abc.b, abc.c);

                int target_slot = base_slot + abc.a;
                uint64_t bslot = slots_get(&slots, (base_slot + abc.b));
                uint64_t cslot = slots_get(&slots, (base_slot + abc.c));

                if (is_small_int(bslot) && is_small_int(cslot))
                    slots_set(&slots,target_slot,to_small_int(get_small_int(bslot) / get_small_int(cslot)));

                if (is_double(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(bslot) / get_double(cslot)));

                if (is_double(bslot) && is_small_int(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(bslot) / get_small_int(cslot)));

                if (is_small_int(bslot) && is_double(cslot))
                    slots_set(&slots,target_slot,to_double(get_double(cslot) / get_small_int(bslot)));

                pc++;
                break;
            }
            case ISEQ: {
                printf("ISEQ: %d %d %d\n", abc.a, abc.b, abc.c);
                int target_slot = base_slot + abc.a;
                uint64_t bslot = slots_get(&slots, (base_slot + abc.b));
                uint64_t cslot = slots_get(&slots, (base_slot + abc.c));

                pc++;

                if (is_small_int(bslot) && is_small_int(cslot)) {
                    slots_set(&slots,target_slot, to_bool(get_small_int(bslot) == get_small_int(cslot)));
                    break;
                }
                if (is_double(bslot) && is_double(cslot)) {
                    slots_set(&slots,target_slot, to_bool(get_double(bslot) == get_double(cslot)));
                    break;
                }

                printf("Type Error ISEQ can only called with all Ints or all Double");
                return 1;

            }
            //------------------Unary Ops------------------
            case MOV: {
                uint8_t target_slot = ad.a;
                uint16_t d = ntohs(ad.d);

                printf("MOV: %d %d\n",target_slot, d);

                slots_set(&slots,(base_slot + target_slot), slots_get(&slots, (base_slot + d)) );

                pc++;
                break;
            }
            case NOT: {
                printf("NOT // not implmented\n");
                pc++;
                break;
            }
            //------------------Jumps------------------
            case JUMP: {
                uint16_t d = ntohs(ad.d);
                printf("JUMP: %d\n",d);
                int16_t offset = (int16_t) d;

                uint32_t new_pc = pc + offset;

                pc = new_pc;
                break;
            }
            case JUMPF: {
                uint16_t d = ntohs(ad.d);
                int16_t offset = (int16_t) d;
                printf("JUMPF: %d %d\n",ad.a,offset);

                if(is_falsy(slots_get(&slots, (base_slot + ad.a)))) {
                    pc = pc + offset;
                } else {
                    pc++;
                }
                break;
            }
            case JUMPT: {
                uint16_t d = ntohs(ad.d);
                int16_t offset = (int16_t) d;
                printf("JUMPT: %d %d\n",ad.a,offset);

                if(is_truthy(slots_get(&slots, (base_slot + ad.a)))) {
                    pc = pc + offset;
                } else {
                    pc++;
                }
                break;
            }
            //------------------Function Calls------------------
            case CALL: {
                uint8_t localbase = ad.a;
                uint16_t lit = ntohs(ad.d);

                printf("CALL %d %d\n", localbase, lit);

                slots_set(&slots,(base_slot + localbase), to_small_int(pc));

                uint64_t fn_slot = slots_get(&slots, (base_slot + localbase + 1));

                uint16_t func = 0;
                if(is_fnew(fn_slot)) {
                    func = get_fnew(fn_slot);
                } else {
                    printf("CALL ERROR NO FUNC\n");
                    exit(0);
                }

                Context old = get_context();
                push(&stack, old);

                uint32_t newbase = base_slot + localbase;

                Context newContext = { .base_slot = newbase, .ip = func };
                set_context(&newContext);

                pc = ip;

                break;
            }
            case RET: {
                uint8_t a = ad.a;
                printf("RET %d\n", a);

                uint32_t ret_addr = get_small_int(slots_get(&slots, base_slot));

                slots_set(&slots, base_slot, slots_get(&slots, (base_slot+a)));

                slots.size = (base_slot+a);

                Context caller = pop(&stack);
                set_context(&caller);

                pc = ret_addr + 1;
                break;
            }
            case APPLY: {
                printf("APPLY // not implmented\n");
                pc++;
                break;
            }
            //------------------Closures and Free Variables------------------
            case FNEW: {

                //Once Closure are added, it needs to check if it is inside of a closure
                uint16_t d = ntohs(ad.d);
                int16_t offset = (int16_t) d;

                printf("FNEW %d %d\n", ad.a, offset);

                slots_set(&slots, (base_slot + ad.a), to_fnew(offset));

                pc++;
                break;
            }
            case VFNEW: { printf("VFNEW // not implmented\n"); pc++; break; }
            case GETFREEVAR: { printf("GETFREEVAR // not implmented\n"); pc++; break; }
            case UCLO: { printf("UCLO // not implmented\n"); pc++; break; }
            //------------------Tail Recursion and Loops------------------
            case LOOP: { printf("LOOP\n"); break; }
            case BULKMOV: {
                for(int i = 0; i != abc.c ;i++ ) {
                    slots_set(&slots, (base_slot + abc.a + i),  slots_get(&slots, (base_slot + abc.b + i)));
                }
                printf("BULKMOV: %d %d %d\n", abc.a, abc.b, abc.c);
                pc++;
                break;
            }
            //------------------Arrays------------------
            case NEWARRAY:{
                printf("NEWARRAY // not implmented\n");
                pc++;
                break;
            }
            case GETARRAY:{
                printf("GETARRAY // not implmented\n");
                pc++;
                break;
            }
            case SETARRAY:{
                printf("SETARRAY // not implmented\n");
                pc++;
                break;
            }

            //------------------Function Def------------------
            case FUNCF:{ printf("FUNCF\n"); pc++; break; }
            case FUNCV:{ printf("FUNCV\n"); pc++; break; }
            //------------------Types------------------
            case ALLOC: {
                uint16_t d = ntohs(ad.d);
                printf("ALLOC: %d %d\n",ad.a,d);
                int target_slot = ad.a;

                struct type_record type = sec.types[d];

                uint64_t alloc_slot = slots_get(&slots,(base_slot + target_slot));

                res = rust_mps_alloc_obj((mps_addr_t*)(void*)&alloc_slot,
                                         obj_ap,
                                         type.type_size,
                                         type.type_id,
                                         OBJ_MPS_TYPE_OBJECT);
                 if (res != MPS_RES_OK) printf("Could't not allocate obj\n");

                 pc++;
                 break;
            }
            case SETFIELD: {
                 printf("SETFIELD: %d %d %d\n",abc.a, abc.b, abc.c);
                 int offset = abc.b;
                 int var_index = abc.c;
                 int ref_index = abc.a;

                 uint64_t* header_ptr = (uint64_t*)slots_get(&slots,(base_slot + ref_index));

                 struct obj_stub  *obj = (struct obj_stub *) (void *) header_ptr;
                 obj->ref[offset] = (uint64_t *)slots_get(&slots,(base_slot + var_index));

                 pc++;
                 break;
            }
            case GETFIELD: {
                printf("GETFIELD: %d %d %d\n",abc.a, abc.b, abc.c);
                int dst = abc.a;
                int offset = abc.c;

                uint64_t* header_ptr = (uint64_t*)slots_get(&slots,(base_slot + abc.b));
                struct obj_stub  *obj = (struct obj_stub *) (void *) header_ptr;

                slots_set(&slots, (base_slot + dst), (uintptr_t) (void *) obj->ref[offset]);

                pc++;
                break;
            }
            //------------------Run-Time Behavior------------------
            case BREAK:  {
                printf("BREAK // not implmented\n");
                pc++;
                break;
            }
            case EXIT: {
                printf("Valid End Reached\n");
                pc++;
                return 1;
            }
            case DROP: {
                printf("DROP // not implmented\n");
                pc++;
                break;
            }
            case TRANC: {
                printf("TRANC // not implmented\n");
                pc++;
                break;
            }
            default: {
                printf("skipping instruction: %d\n\n", op);
                pc++;
                break;
            }
        }
        print_slots(&slots);
    }*/

    free_vm(&vm);

    return 0;
}