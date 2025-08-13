#if defined(JIT_ON) && defined(X64)

#include <stdio.h>
#include <stdlib.h>

#include "reg.h"
#include "compile.h"

#include "dasm_x86.h"

void* compile(ir_function_t* ir, opt_liveness_t* liveness) {
    | .arch x64

    dasm_State* d;
    unsigned int apc = 8;
    unsigned int cpc = 0;

    | .section code
    dasm_init(&d, DASM_MAXSECTION);

    | .globals label_
    void* labels[label__MAX];
    dasm_setupglobal(&d, labels, label__MAX);

    | .actionlist kn_actions
    dasm_setup(&d, kn_actions);

    dasm_State** Dst = &d;
    dasm_growpc(Dst, apc);

    | .code
    for (int b = 0; b < ir->block_count; b++) {
        ir_block_t* block = ir->blocks[b];

        | =>pc(&d, cpc, apc):
        for (int i = 0; i < block->instruction_count; i++) {
            ir_instruction_t instr = block->instructions[i];

            switch (instr.op) {
                
            }
        }
        | ret
    }

    void* program = link(&d) + dasm_getpclabel(&d, 0);
    dasm_free(&d);

    return (void*) program;
}

#endif