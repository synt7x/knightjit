#include "opt.h"

#include "jit/reg.h"
#define REGISTERS 8

regs_t reg_allocate(ir_function_t* ir, opt_liveness_t* liveness) {
    for (int b = 0; b < ir->block_count; b++) {
        ir_block_t* block = ir->blocks[b];
        for (int i = block->instruction_count - 1; i >= 0; i--) {
            ir_instruction_t instr = block->instructions[i];

            
        }
    }
}