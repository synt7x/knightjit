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
                instr->op == IR_QUIT   ||
                instr->op == IR_SAVE   ||
                instr->op == IR_RESTORE;
            if (ir_first_use(function, instr->result) || preserve) {
                if (count != i) block->instructions[count] = block->instructions[i];
                count++;
            }
        }
        block->instruction_count = count;
    }
}

opt_liveness_t* ir_ranges(ir_function_t* function) {
    int n = function->next_value_id;
    opt_liveness_t* tracked = malloc(sizeof(opt_liveness_t) * n);
    if (!tracked) panic("Failed to allocate memory for liveness analysis");

    for (int i = 0; i < n; i++) {
        tracked[i].start = -1;
        tracked[i].end = -1;

        tracked[i].id = i;
    }

    for (int b = 0; b < function->block_count; b++) {
        ir_block_t* block = function->blocks[b];

        for (int i = 0; i < block->instruction_count; i++) {
            ir_instruction_t* instr = &block->instructions[i];
            ir_id_t id = instr->result;

            tracked[id].start = i;
            
            switch (instr->op) {
                case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV:
                case IR_MOD: case IR_POW: case IR_GT: case IR_LT:
                case IR_EQ: case IR_AND: case IR_OR: case IR_NOT:
                case IR_NEG: case IR_LENGTH: case IR_BOX:
                case IR_ASCII: case IR_PRIME: case IR_ULTIMATE:
                case IR_GET: case IR_SET: case IR_CALL:
                case IR_OUTPUT: case IR_DUMP: case IR_QUIT:
                    for (int j = 0; j < instr->generic.operand_count; j++) {
                        ir_id_t operand = instr->generic.operands[j];
                        if (operand < n) {
                            if (tracked[operand].end < i) {
                                tracked[operand].end = i;
                            }
                        }
                    }
                    break;
                case IR_STORE:
                    if (instr->var.value < n) {
                        if (tracked[instr->var.value].end < i) {
                            tracked[instr->var.value].end = i;
                        }
                    }
                    break;
                case IR_BRANCH:
                    if (instr->branch.condition < n) {
                        if (tracked[instr->branch.condition].end < i) {
                            tracked[instr->branch.condition].end = i;
                        }
                    }
                    break;
                case IR_RETURN:
                    if (instr->generic.operand_count > 0 && instr->generic.operands[0] < n) {
                        if (tracked[instr->generic.operands[0]].end < i) {
                            tracked[instr->generic.operands[0]].end = i;
                        }
                    }
                    break;
                case IR_PHI:
                    for (int j = 0; j < instr->phi.phi_count; j++) {
                        ir_id_t phi_value = instr->phi.phi_values[j];
                        if (phi_value < n) {
                            if (tracked[phi_value].end < -1) {
                                tracked[phi_value].end = i;
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }

    return tracked;
}

opt_liveness_t* ir_preserve(ir_function_t* function, opt_liveness_t* tracked) {
    for (int b = 0; b < function->block_count; b++) {
        ir_block_t* block = function->blocks[b];

        for (int i = 0; i < block->instruction_count; i++) {
            ir_instruction_t* instr = &block->instructions[i];
            
            if (instr->op == IR_CALL) {
                ir_instruction_t* save = &block->instructions[i - 1];
                ir_instruction_t* restore = &block->instructions[i + 1];

                save->generic.operand_count = 0;
                restore->generic.operand_count = 0;

                for (int j = 0; j < i; j++) {
                    ir_instruction_t* pre = &block->instructions[j];
                    if (ir_is_constant(pre)) continue;
                    opt_liveness_t lifetime = tracked[pre->result];

                    if (lifetime.end > i && lifetime.start >= 0) {
                        if (!save->generic.operand_count) {
                            save->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t));
                            restore->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t));

                            save->generic.operands[0] = pre->result;
                            restore->generic.operands[0] = pre->result;
                        } else {
                            save->generic.operands = arena_realloc(function->arena, save->generic.operands, sizeof(ir_id_t) * save->generic.operand_count + 1);
                            restore->generic.operands = arena_realloc(function->arena, restore->generic.operands, sizeof(ir_id_t) * restore->generic.operand_count + 1);
                        
                            save->generic.operands[save->generic.operand_count] = pre->result;
                            restore->generic.operands[restore->generic.operand_count] = pre->result;
                        }

                        save->generic.operand_count++;
                        restore->generic.operand_count++;
                    }
                }

                if (!save->generic.operand_count) {
                    for (int k = i; k < block->instruction_count; k++) {
                        if (k == i - 1 || k == i + 1) continue;
                        int target = k > i + 1 ? k - 2 : k - 1;
                        block->instructions[target] = block->instructions[k];

                        free(tracked);
                        tracked = ir_ranges(function);
                    }

                    block->instruction_count -= 2;
                    i -= 1;
                }
            }
        }
    }

    return tracked;
}

void ir_optimize(ir_function_t* function) {
    ir_fold(function);
    ir_drop(function);

    opt_liveness_t* liveness = ir_preserve(function, ir_ranges(function));
    free(liveness);
}