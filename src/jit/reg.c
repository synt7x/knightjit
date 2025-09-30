#include "opt.h"

#include "jit/reg.h"
#define REGISTERS (16 - 4)

reg_info_t reg_allocate(ir_function_t* ir, opt_liveness_t* liveness) {
    int alloc[REGISTERS];
    for (int i = 0; i < REGISTERS; i++) {
        alloc[i] = 0;
    }

    regs_t* regs = malloc(sizeof(uint64_t) * ir->next_value_id);

    int stack_slot = 0;
    for (int i = 0; i < ir->next_value_id; i++) {
        regs[i].reg = -1;
        regs[i].slot = -1;
    }

    for (int b = 0; b < ir->block_count; b++) {
        ir_block_t* block = ir->blocks[b];
        for (int i = block->instruction_count - 1; i >= 0; i--) {
            ir_instruction_t* instr = &block->instructions[i];
            ir_id_t result = instr->result;
            opt_liveness_t lifetime = liveness[result];
            
            if (lifetime.end == -1) continue;
            if (instr->op == IR_BLOCK || instr->op == IR_CONST_NULL || instr->op == IR_CONST_BOOLEAN || instr->op == IR_CONST_NUMBER) continue;

            // Free registers for values whose lifetime ends at this instruction
            for (int p = 0; p < ir->next_value_id; p++) {
                if (liveness[p].end == i) {
                    int reg = regs[p].reg;
                    if (reg != -1) {
                        alloc[reg] = 0;
                    }
                }
            }

            int reg = -1;
            for (int r = 0; r < REGISTERS; r++) {
                if (!alloc[r]) {
                    reg = r;
                    break;
                }
            }

            if (reg == -1) {
                regs[result].slot = stack_slot++;
            } else {
                alloc[reg] = 1;
                regs[result].reg = reg;
            }
        }
    }

    return (reg_info_t) {
        .regs = regs,
        .max_slot = stack_slot
    };
}