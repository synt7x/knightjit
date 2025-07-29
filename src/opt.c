#include "opt.h"

void ir_fold(ir_function_t* function) {
    for (int i = 0; i < function->block_count; i++) {
        ir_block_t* block = function->blocks[i];
        for (int j = 0; j < block->instruction_count; j++) {
            ir_instruction_t* instr = &block->instructions[j];
            
            if (
                instr->op == IR_ADD || instr->op == IR_SUB || instr->op == IR_MUL || instr->op == IR_DIV
                || instr->op == IR_MOD || instr->op == IR_GT || instr->op == IR_LT || instr->op == IR_EQ
                || instr->op == IR_POW
            ) {
                ir_instruction_t* l = ir_fetch(block, instr->generic.operands[0]);
                ir_instruction_t* r = ir_fetch(block, instr->generic.operands[1]);

                if (!l || !r) continue;
                if (!ir_is_constant(l) || !ir_is_constant(r)) continue;

                v_t lv = l->constant.value;
                v_t rv = r->constant.value;

                v_t result;

                switch (instr->op) {
                    case IR_ADD: result = vm_add(lv, rv); break;
                    case IR_SUB: result = vm_sub(lv, rv); break;
                    case IR_MUL: result = vm_mul(lv, rv); break;
                    case IR_DIV: result = vm_div(lv, rv); break;
                    case IR_MOD: result = vm_mod(lv, rv); break;
                    case IR_GT:  result = vm_gt(lv, rv);  break;
                    case IR_LT:  result = vm_lt(lv, rv);  break;
                    case IR_EQ:  result = vm_eq(lv, rv);  break;
                    case IR_POW: result = vm_pow(lv, rv); break;
                    default: continue;
                }

                ir_to_const(instr, result);
            } else if (
                instr->op == IR_LENGTH || instr->op == IR_BOX || instr->op == IR_ASCII  || instr->op == IR_NOT || instr->op == IR_NEG
                || instr->op == IR_PRIME || instr->op == IR_ULTIMATE
            ) {
                ir_instruction_t* operand = ir_fetch(block, instr->generic.operands[0]);
                if (!operand || !ir_is_constant(operand)) continue;

                v_t value = operand->constant.value;
                v_t result;

                switch (instr->op) {
                    case IR_LENGTH: result = vm_length(value); break;
                    case IR_BOX:    result = vm_box(value); break;
                    case IR_ASCII:  result = vm_ascii(value); break;
                    case IR_NOT:    result = v_coerce_to_boolean(value) ^ (1 << 3); break;
                    case IR_NEG:    result = ((v_number_t) v_coerce_to_number(value)) * -1; break;
                    case IR_PRIME:  result = vm_prime(value); break;
                    case IR_ULTIMATE: result = vm_ultimate(value); break;
                    default: continue;
                }

                ir_to_const(instr, result);
            } else if (instr->op == IR_GET) {
                ir_instruction_t* value = ir_fetch(block, instr->generic.operands[0]);
                ir_instruction_t* index = ir_fetch(block, instr->generic.operands[1]);
                ir_instruction_t* range = ir_fetch(block, instr->generic.operands[2]);

                if (!value || !index || !range || !ir_is_constant(value) || !ir_is_constant(index) || !ir_is_constant(range)) {
                    continue;
                }

                v_t v = value->constant.value;
                v_t idx = index->constant.value;
                v_t r = range->constant.value;

                v_t result = vm_get(v, idx, r);
                ir_to_const(instr, result);
            }
        }
    }
}

void ir_drop(ir_function_t* function) {
    for (int b = 0; b < function->block_count; b++) {
        ir_block_t* block = function->blocks[b];

        int count = 0;
        for (int i = 0; i < block->instruction_count; i++) {
            ir_instruction_t* instr = &block->instructions[i];
            int preserve =
                instr->op == IR_OUTPUT ||
                instr->op == IR_DUMP   ||
                instr->op == IR_STORE  ||
                instr->op == IR_RETURN ||
                instr->op == IR_BRANCH ||
                instr->op == IR_JUMP   ||
                instr->op == IR_CALL   ||
                instr->op == IR_QUIT;
            if (ir_first_use(function, instr->result) || preserve) {
                if (count != i) block->instructions[count] = block->instructions[i];
                count++;
            }
        }
        block->instruction_count = count;
    }
}

void ir_optimize(ir_function_t* function) {
    ir_fold(function);
    ir_drop(function);
}