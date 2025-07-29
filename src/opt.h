#ifndef OPT_H
#define OPT_H

#include "ir.h"
#include "vm.h"

static inline ir_instruction_t* ir_fetch(ir_block_t* block, ir_id_t result) {
    for (int i = 0; i < block->instruction_count; ++i) {
        ir_instruction_t* instr = &block->instructions[i];
        if (instr->result == result) {
            return instr;
        }
    }

    return NULL;
}

static inline ir_instruction_t* ir_first_use(ir_function_t* function, ir_id_t result) {
    for (int i = 0; i < function->block_count; ++i) {
        ir_block_t* block = function->blocks[i];
        for (int j = 0; j < block->instruction_count; ++j) {
            ir_instruction_t* instr = &block->instructions[j];
            
            switch (instr->op) {
                case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV:
                case IR_MOD: case IR_POW: case IR_GT: case IR_LT: case IR_EQ:
                case IR_AND: case IR_OR: case IR_NOT: case IR_NEG:
                case IR_LENGTH: case IR_BOX: case IR_ASCII:
                case IR_PRIME: case IR_ULTIMATE:
                case IR_GET: case IR_SET:
                case IR_CALL: case IR_OUTPUT: case IR_DUMP:
                case IR_QUIT:
                    for (int k = 0; k < instr->generic.operand_count; ++k) {
                        if (instr->generic.operands[k] == result)
                            return instr;
                    }
                    break;
                case IR_STORE:
                    if (instr->var.value == result)
                        return instr;
                    break;
                case IR_BRANCH:
                    if (instr->branch.condition == result)
                        return instr;
                    break;
                case IR_RETURN:
                    if (instr->generic.operands[0] == result)
                        return instr;
                    break;
                case IR_PHI:
                    for (int k = 0; k < instr->phi.phi_count; ++k) {
                        if (instr->phi.phi_values[k] == result)
                            return instr;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    return NULL;
}

static inline void ir_to_const(ir_instruction_t* instr, v_t result) {
    instr->generic.operands = 0;
    instr->generic.operand_count = 0;

    switch (V_TYPE(result)) {
        case TYPE_BOOLEAN:
            instr->op = IR_CONST_BOOLEAN;
            break;
        case TYPE_LIST:
            instr->op = IR_CONST_ARRAY;
            break;
        case TYPE_NUMBER:
            instr->op = IR_CONST_NUMBER;
            break;
        case TYPE_STRING:
            instr->op = IR_CONST_STRING;
            break;
        case TYPE_NULL:
            instr->op = IR_CONST_NULL;
            break;
    }

    instr->constant.value = result;
}

static inline int ir_is_constant(ir_instruction_t* instr) {
    return instr->op == IR_CONST_ARRAY || instr->op == IR_CONST_BOOLEAN || instr->op == IR_CONST_NULL
        || instr->op == IR_CONST_NUMBER || instr->op == IR_CONST_STRING;
}

void ir_optimize(ir_function_t* function);

#endif