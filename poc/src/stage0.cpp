#include <cstdint>
#include <iostream>

#include "dasm_proto.h"
#include "dasm_x86.h"

#include "shared.hpp"

void* link(dasm_State** d) {
    size_t size;
    void* ptr;

    dasm_link(d, &size);
    ptr = jalloc(size);
    
    dasm_encode(d, ptr);
    jprotect(ptr, size);

    return ptr;
}

| .arch x64

void output(uint64_t val) {
    uint8_t type = val & 0x7;
    uint64_t value = val >> 3;

    switch (type) {
        case 0: // Number
            printf("%llu\n", value);
            break;
        default:
            std::cout << "Error: This PoC should output a number!" << std::endl;
            break;
    }

    return;
}

using pool = uint64_t*;

void* stage0(pool trace_pool) {
    /*
     * ; = list ...75,000 ints... ... 25,000 strings ...
     * ; = i 0
     * ; = result 0
     * WHILE > #list i
     *   ; = result + result GET list i 1
     *   = i + i 1
     * OUTPUT result
     */

    dasm_State* d;
    uint32_t apc = 8;
    uint32_t cpc = 0;

    | .section code, constants
    dasm_init(&d, DASM_MAXSECTION);

    | .globals label_
    void* labels[label__MAX];
    dasm_setupglobal(&d, labels, label__MAX);

    | .actionlist kn_actions
    dasm_setup(&d, kn_actions);

    dasm_State** Dst = &d;
    dasm_growpc(Dst, apc);
    /*
     * ; = list ...75,000 ints... ... 25,000 strings ...
     */
    | .constants
    | .align 8

    | ->string:
    | .qword 3
    | .byte '5'
    | .byte '5'
    | .byte '5'

    | =>0:
    | ->array:
    | .qword 10000000
    for (size_t i = 0; i < 7500000; i++) {
        | .qword (i % 10 + 1) << 3
    }

    for (size_t i = 0; i < 2500000; i++) {
        | .qword ->string
    }

    /* This is a simulation of the generated code */
    | .code
    | =>1:
    | mov64 r14, (uint64_t) trace_pool
    /*
     * = i 0
     */
    | mov rax, 0
    /*
     * = result 0
     */
    | mov rbx, 0

    | 3: // Loop head
    /*
     * WHILE > #list i
     */
    | mov rcx, [->array]
    | cmp rax, rcx
    | jge >4

    /*
     * GET list i 1
     */
    | lea rcx, [->array]
    | mov rcx, [rcx + rax * 8 + 8]
    
    /* Add */
    |=>2:
    | mov rdx, rcx
    | and rdx, 0x7
    | jz >9

    | cmp rdx, 3
    | jne >6
    /* Modify trace pool slot 2 */
    | inc qword [r14 + 8]

    | and rcx, ~7
    | mov r8, [rcx]
    | mov r9, 0
    | mov r10, 0
    
    | 7:
    | cmp r9, r8
    | jge >8

    | movzx r11, byte [rcx + r9 + 8]
    | cmp r11, '0'
    | jl >8
    | cmp r11, '9'
    | jg >8
    
    | sub r11, '0'
    | imul r10, r10, 10
    | add r10, r11
    
    | inc r9
    | jmp <7

    | 8:
    | shl r10, 3
    | mov rcx, r10

    | 5:
    | add rbx, rcx

    | 6:
    | inc rax
    | jmp <3

    | 9:
    /* Modify trace pool slot 1 */
    | inc qword [r14]

    | add rbx, rcx
    |=>3:
    | inc rax
    | jmp <3

    /* Exit loop */
    | 4:

    | sub rsp, 40
    | mov rcx, rbx
    | mov64 rax, (uint64_t) output
    | call rax
    | add rsp, 40

    | ret

    byte* linked = (byte*) link(&d);
    void* program = linked + dasm_getpclabel(&d, 1);    
    uintptr_t *table = (uintptr_t*) (linked + dasm_getpclabel(&d, 0));
    std::cout << "[JIT] Patchable code exists as " << (dasm_getpclabel(&d, 3) - dasm_getpclabel(&d, 2)) << " bytes from offset " << dasm_getpclabel(&d, 2) << std::endl;
    dasm_free(&d);


    /* Fixup types, this is because we are storing constants alongside the assembly */
    for (size_t i = 0; i < 2500000; i++) {
        table[i + 7500001] |= 3;
    }


    return program;
}