
#include "execute.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "vm.h"
#include "mps.h"
#include "mpsavm.h"
#include "debug.h"
#include "loader.h"

int start(char *file) {
    struct sections sec = {0};

    mps_res_t res;

    res = loadfile(file, &sec);
    if (res != MPS_RES_OK) fprintf(stderr,"Couldn't load file");

    VM vm = {0};

    size_t arenasize = 32ul * 1024 * 1024;

    vm_init(&vm, arenasize);

    if(debug_level > 0)
        fprintf(stderr,"------------SLOT--------------\n");

    while (1) {

        uint32_t pc = vm.pc;
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
                if(debug_level > 0)
                    fprintf(stderr,"CSTR: %d %d -> %s\n", ad.a, d, sec.cstr[d]);

                //TODO REAL STRING HANDLING
                //set(&vm, ad.a,   (uint64_t)(void*)sec.cstr[d] );

                set(&vm, ad.a, to_small_int(d) );

                vm.pc++;
                break;
            }
            case CKEY: {
                uint16_t d = ntohs(ad.d);

                if(debug_level > 0)
                    fprintf(stderr,"CKEY: %d %d\n", ad.a, d);
                set(&vm, ad.a, (uint64_t)(void*)sec.cstr[d] );


                vm.pc++;
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
                if(debug_level > 0)
                    fprintf(stderr,"CTYPE: %d %d\n", ad.a, d);

                set(&vm,ad.a,to_type(d));

                vm.pc++;
                break;
            }
            //------------------Constant Value Operation------------------
            case CBOOL: {
                uint16_t d = ntohs(ad.d);
                if(debug_level > 0)
                    fprintf(stderr,"CBOOL: %d %d\n", ad.a, d);
                set(&vm,ad.a,to_bool(d));
                vm.pc++;
                break;
            }
            case CNIL : {
                if(debug_level > 0)
                    fprintf(stderr,"CNIL: %d\n", ad.a);
                set(&vm,ad.a,get_nil());
                vm.pc++;
                break;
            }
            case CSHORT: {
                int target_slot = ad.a;
                uint16_t d16 = ntohs(ad.d);
                uint64_t d = (uint64_t) d16;

                if(debug_level > 0)
                    fprintf(stderr,"CSHORT: %d %d\n", ad.a, d16);
                set(&vm,target_slot,to_small_int(d));

                vm.pc++;
                break;
            }
            //------------------Global Table Ops------------------
            case NSSET: {
                uint16_t d = ntohs(ad.d);
                if(debug_level > 0)
                    fprintf(stderr,"NSSET: %d %d                              -> %s\n", ad.a, d, sec.cstr[d]);

                add_symbol_table_pair(&vm, sec.cstr[d], get(&vm,ad.a));

                vm.pc++;
                break;
            }
            case NSGET: {
                uint16_t d = ntohs(ad.d);

                if(debug_level > 0)
                    fprintf(stderr,"NSGET: %d %d                              -> %s\n ", ad.a, d, sec.cstr[d]);

                set(&vm,ad.a, get_symbol_table(&vm, sec.cstr[d]));

                vm.pc++;
                break;
            }
            //------------------Variable Slots------------------
            case ADDVV: {
                int target_slot = abc.a;
                uint64_t bslot = get(&vm,abc.b);
                uint64_t cslot = get(&vm,abc.c);

                if(debug_level > 0)
                    fprintf(stderr,"ADDVV: %d %d %d\n", abc.a, abc.b, abc.c);

                if (is_small_int(bslot) && is_small_int(cslot))
                    set(&vm, target_slot, to_small_int(get_small_int(bslot) + get_small_int(cslot)));

                if (is_double(bslot) && is_double(cslot))
                    set(&vm, target_slot,to_double(get_double(bslot) + get_double(cslot)));

                if (is_double(bslot) && is_small_int(cslot))
                    set(&vm, target_slot,to_double(get_double(bslot) + get_small_int(cslot)));

                if (is_small_int(bslot) && is_double(cslot))
                    set(&vm, target_slot, to_double(get_double(cslot) + get_small_int(bslot)));

                vm.pc++;
                break;
            }
            case SUBVV: {
                int target_slot = abc.a;
                uint64_t bslot = get(&vm,abc.b);
                uint64_t cslot = get(&vm,abc.c);

                if(debug_level > 0)
                    fprintf(stderr,"SUBVV: %d %d %d\n", abc.a, abc.b, abc.c);

                if (is_small_int(bslot) && is_small_int(cslot) )
                    set(&vm,target_slot, to_small_int(get_small_int(bslot) - get_small_int(cslot)));

                if (is_double(bslot) && is_double(cslot))
                    set(&vm,target_slot, to_double(get_double(bslot) - get_double(cslot)));

                if (is_double(bslot) && is_small_int(cslot))
                    set(&vm,target_slot, to_double(get_double(bslot) - get_small_int(cslot)));

                if (is_small_int(bslot) && is_double(cslot))
                    set(&vm,target_slot, to_double(get_double(cslot) - get_small_int(bslot)));

                vm.pc++;
                break;
            }
            case MULVV: {
                int target_slot = abc.a;
                uint64_t bslot = get(&vm,abc.b);
                uint64_t cslot = get(&vm,abc.c);

                if(debug_level > 0)
                    fprintf(stderr,"MULVV: %d %d %d\n", abc.a, abc.b, abc.c);

                if (is_small_int(bslot) && is_small_int(cslot))
                    set(&vm,target_slot,to_small_int(get_small_int(bslot) * get_small_int(cslot)));

                if (is_double(bslot) && is_double(cslot))
                    set(&vm,target_slot,to_double(get_double(bslot) * get_double(cslot)));

                if (is_double(bslot) && is_small_int(cslot))
                    set(&vm,target_slot,to_double(get_double(bslot) * get_small_int(cslot)));

                if (is_small_int(bslot) && is_double(cslot))
                    set(&vm,target_slot,to_double(get_double(cslot) * get_small_int(bslot)));

                vm.pc++;
                break;
            }
            case MODVV: {

                int target_slot = abc.a;
                uint64_t bslot = get(&vm,abc.b);
                uint64_t cslot = get(&vm,abc.c);

                if(debug_level > 0)
                    fprintf(stderr,"MODVV: %d %d %d\n", abc.a, abc.b, abc.c);

                if (is_small_int(bslot) && is_small_int(cslot)) {
                    set(&vm,target_slot, to_small_int(get_small_int(bslot) % get_small_int(cslot)));
                } else {
                    fprintf(stderr,"Type Error. Called Modulo with Float\n");
                    exit(1);
                }

                vm.pc++;
                break;
            }
            case DIVVV: {
                if(debug_level > 0)
                    fprintf(stderr,"DIVVV: %d %d %d\n", abc.a, abc.b, abc.c);

                int target_slot = abc.a;
                uint64_t bslot = get(&vm,abc.b);
                uint64_t cslot = get(&vm,abc.c);

                if (is_small_int(bslot) && is_small_int(cslot))
                    set(&vm,target_slot,to_small_int(get_small_int(bslot) / get_small_int(cslot)));

                if (is_double(bslot) && is_double(cslot))
                    set(&vm,target_slot,to_double(get_double(bslot) / get_double(cslot)));

                if (is_double(bslot) && is_small_int(cslot))
                    set(&vm,target_slot,to_double(get_double(bslot) / get_small_int(cslot)));

                if (is_small_int(bslot) && is_double(cslot))
                    set(&vm,target_slot,to_double(get_double(cslot) / get_small_int(bslot)));

                vm.pc++;
                break;
            }
            case ISEQ: {
                if(debug_level > 0)
                    fprintf(stderr,"ISEQ: %d %d %d\n", abc.a, abc.b, abc.c);
                int target_slot = abc.a;
                uint64_t bslot = get(&vm,abc.b);
                uint64_t cslot = get(&vm,abc.c);

                vm.pc++;

                if (is_small_int(bslot) && is_small_int(cslot)) {
                    set(&vm,target_slot, to_bool(get_small_int(bslot) == get_small_int(cslot)));
                    break;
                }
                if (is_double(bslot) && is_double(cslot)) {
                    set(&vm,target_slot, to_bool(get_double(bslot) == get_double(cslot)));
                    break;
                }

                fprintf(stderr,"Type Error ISEQ can only called with all Ints or all Double");
                return 1;

            }
            //------------------Unary Ops------------------
            case MOV: {
                uint8_t target_slot = ad.a;
                uint16_t d = ntohs(ad.d);

                if(debug_level > 0)
                    fprintf(stderr,"MOV: %d %d\n",target_slot, d);

                move(&vm, target_slot, d);

                vm.pc++;
                break;
            }
            case NOT: {
                if(debug_level > 0)
                    fprintf(stderr,"NOT // not implmented\n");
                vm.pc++;
                break;
            }
            //------------------Jumps------------------
            case JUMP: {
                uint16_t d = ntohs(ad.d);
                if(debug_level > 0)
                    fprintf(stderr,"JUMP: %d\n",d);
                int16_t offset = (int16_t) d;

                uint32_t new_pc = vm.pc + offset;

                vm.pc = new_pc;
                break;
            }
            case JUMPF: {
                uint16_t d = ntohs(ad.d);
                int16_t offset = (int16_t) d;
                if(debug_level > 0)
                    fprintf(stderr,"JUMPF: %d %d\n",ad.a,offset);

                if(is_falsy( get(&vm,ad.a) )) {
                    vm.pc = vm.pc + offset;
                } else {
                    vm.pc++;
                }
                break;
            }
            case JUMPT: {
                uint16_t d = ntohs(ad.d);
                int16_t offset = (int16_t) d;
                if(debug_level > 0)
                    fprintf(stderr,"JUMPT: %d %d\n",ad.a,offset);

                if(is_truthy(get(&vm,ad.a))) {
                    vm.pc = vm.pc + offset;
                } else {
                    vm.pc++;
                }
                break;
            }
            //------------------Function Calls------------------
            case CALL: {
                uint8_t localbase = ad.a;
                uint16_t lit = ntohs(ad.d);

                if(debug_level > 0)
                    fprintf(stderr,"CALL %d %d", localbase, lit);

                set(&vm,localbase, to_small_int(vm.pc));

                uint64_t fn_slot = get(&vm,localbase+1);

                uint16_t func = 0;
                if(is_fnew(fn_slot)) {
                    if(debug_level > 0)
                        fprintf(stderr,"                               -> is func\n");
                    func = get_fnew(fn_slot);
                } else if(is_builtin(fn_slot)) {
                    if(debug_level > 0)
                        fprintf(stderr,"                               -> is builtin\n");
                    func = 0;
                } else if(is_vfunc(fn_slot)) {

                    if(debug_level > 0)
                        fprintf(stderr, "                              -> is vfunc\n");

                    uint16_t vfunc = get_vfunc(fn_slot);

                    uint64_t obj_slot = get(&vm, localbase+2);

                    struct obj_stub  *obj = (struct obj_stub *) (void *) obj_slot;

                    uint16_t type_int = obj->cljtype;
                    
                    uint64_t key = composeKey(vfunc, type_int);

                    struct vtable_record * record = get_vtable_record(&sec, key);

                    func = record->jump_offset;
                } else {
                    print_slots(&vm.slots, vm.base);
                    fprintf(stderr,"\nCALL ERROR NO FUNC\n\n");
                    exit(0);
                }

                Context old = get_context(&vm);
                push(&vm.stack, old);

                uint32_t newbase = vm.base + localbase;

                Context newContext = { .base_slot = newbase, .ip = func };
                set_context(&vm, &newContext);

                if(func == 0) {
                    builtin_fn f =  get_builtin(fn_slot);
                    f(&vm);

                    for(int i = 2; i != (localbase + 10); i++) {
                        set(&vm, i, get_nil());
                    }

                    Context caller = pop(&vm.stack);
                    set_context(&vm, &caller);

                    vm.pc++;

                } else {
                    if(debug_level > 0)
                        fprintf(stderr,"func %d\n\n", func);
                    vm.pc = func;
                }

                break;
            }
            case RET: {
                uint8_t a = ad.a;
                if(debug_level > 0) {
                    fprintf(stderr,"RET %d                                 -> ", a);
                    print_slot(get(&vm,a));
                    fprintf(stderr,"\n");
                }

                uint32_t ret_addr = get_small_int( get(&vm,0) );

                move(&vm,0,a);

                vm.slots.size = (vm.base+a);

                Context caller = pop(&vm.stack);
                set_context(&vm, &caller);

                vm.pc = ret_addr + 1;
                break;
            }
            case APPLY: {
                if(debug_level > 0)
                    fprintf(stderr,"APPLY // not implmented\n");
                vm.pc++;
                break;
            }
            //------------------Closures and Free Variables------------------
            case FNEW: {

                //Once Closure are added, it needs to check if it is inside of a closure
                uint16_t d = ntohs(ad.d);
                int16_t offset = (int16_t) d;

                if(debug_level > 0)
                    fprintf(stderr,"FNEW %d %d\n", ad.a, offset);

                set(&vm, ad.a, to_fnew(offset));

                vm.pc++;
                break;
            }
            case VFNEW: {

                uint16_t d = ntohs(ad.d);
                int16_t offset = (int16_t) d;

                if(debug_level > 0)
                    fprintf(stderr,"VFNEW %d %d\n",ad.a, ntohs(ad.d));

                //let vfunc = args.d as int;
                //vm.slots[args.a] = VFunc(vfunc as uint);
                //vm.fetch_next()

                set(&vm, ad.a, to_vfunc(offset) );
                vm.pc++;
                break;
            }
            case GETFREEVAR: {
                if(debug_level > 0)
                    fprintf(stderr,"GETFREEVAR // not implmented\n");
                vm.pc++;
                break;
            }
            case UCLO: {
                if(debug_level > 0)
                    fprintf(stderr,"UCLO // not implmented\n");
                vm.pc++;
                break;
                }
            //------------------Tail Recursion and Loops------------------
            case LOOP: {
                if(debug_level > 0)
                    fprintf(stderr,"LOOP\n");
                vm.pc++;
                break;
            }
            case BULKMOV: {
                for(int i = 0; i != abc.c ;i++ ) {
                    move(&vm, abc.a+i, abc.b+i);
                }
                if(debug_level > 0)
                    fprintf(stderr,"BULKMOV: %d %d %d\n", abc.a, abc.b, abc.c);
                vm.pc++;
                break;
            }
            //------------------Arrays------------------
            case NEWARRAY:{
                if(debug_level > 0)
                    fprintf(stderr,"NEWARRAY // not implmented\n");
                vm.pc++;
                break;
            }
            case GETARRAY: {
                if(debug_level > 0)
                    fprintf(stderr,"GETARRAY // not implmented\n");
                vm.pc++;
                break;
            }
            case SETARRAY:{
                if(debug_level > 0)
                    fprintf(stderr,"SETARRAY // not implmented\n");
                vm.pc++;
                break;
            }

            //------------------Function Def------------------
            case FUNCF:{
                if(debug_level > 0)
                    fprintf(stderr,"FUNCF\n");
                vm.pc++;
                break;
            }
            case FUNCV:{
                if(debug_level > 0)
                    fprintf(stderr,"FUNCV\n");
                vm.pc++;
                break;
            }
            //------------------Types------------------
            case ALLOC: {
                uint16_t d = ntohs(ad.d);

                if(debug_level > 0)
                    fprintf(stderr,"ALLOC: %d %d\n",ad.a,d);

                int target_slot = ad.a;

                uint16_t type_index = get_type(get(&vm, d));

                struct type_record type = sec.types[type_index];

                uint64_t* alloc_slot_ptr = get_ptr(&vm, target_slot);

                res = mps_alloc_obj((mps_addr_t*)(void*)alloc_slot_ptr,
                                     vm.amc.ap,
                                     type.type_size,
                                     type.type_id,
                                     OBJ_MPS_TYPE_OBJECT);
                if (res != MPS_RES_OK) fprintf(stderr,"Could't not allocate obj\n");

                vm.pc++;
                break;
            }
            case GETFIELD: {
                if(debug_level > 0)
                    fprintf(stderr,"GETFIELD: %d %d %d\n",abc.a, abc.b, abc.c);

                int dst = abc.a;
                int offset = abc.c;

                uint64_t* header_ptr = (uint64_t*)get(&vm,abc.b);
                struct obj_stub  *obj = (struct obj_stub *) (void *) header_ptr;

                set(&vm,dst, (uintptr_t) (void *) obj->ref[offset]);

                vm.pc++;
                break;
            }
            case SETFIELD: {
                if(debug_level > 0)
                    fprintf(stderr,"SETFIELD: %d %d %d\n",abc.a, abc.b, abc.c);


                int offset = abc.b;
                int var_index = abc.c;
                int ref_index = abc.a;

                uint64_t* header_ptr = (uint64_t*) get(&vm, ref_index);

                struct obj_stub  *obj = (struct obj_stub *) (void *) header_ptr;

                obj->ref[offset] = (uint64_t *)get(&vm,var_index);

                vm.pc++;
                break;
            }
            //------------------Run-Time Behavior------------------
            case BREAK:  {
                if(debug_level > 0)
                    fprintf(stderr,"BREAK // not implmented\n");
                vm.pc++;
                break;
            }
            case EXIT: {
                if(debug_level > 0)
                    fprintf(stderr,"Valid End Reached\n");
                vm.pc++;
                return 1;
            }
            case DROP: {
                if(debug_level > 0)
                    fprintf(stderr,"DROP // not implmented\n");
                vm.pc++;
                break;
            }
            case TRANC: {
                if(debug_level > 0)
                    fprintf(stderr,"TRANC // not implmented\n");
                vm.pc++;
                break;
            }
            default: {
                if(debug_level > 0)
                    fprintf(stderr,"skipping instruction: %d\n\n", op);
                vm.pc++;
                break;
            }
        }
        if(debug_level > 0) {
            print_slots(&vm.slots,vm.base);
            fprintf(stderr,"\n");
        }


    }

    free_vm(&vm);

    return 0;
}