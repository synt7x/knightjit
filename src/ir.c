#include "ir.h"
#include "parser.h"
#include "jit/value.h"

v_t ir_load_literal(const char* value, size_t length, ast_literal_kind_t kind) {
    switch (kind) {
        case AST_LITERAL_BOOLEAN:
            return v_create(TYPE_BOOLEAN, (void*)(intptr_t) (value[0] == 'T'));
        case AST_LITERAL_STRING:
            return v_create_static_string(value, length);
        case AST_LITERAL_NUMBER:
            return v_create(TYPE_NUMBER, (void*)(intptr_t) strtol(value, NULL, 10));
        case AST_LITERAL_NULL:
            return v_create(TYPE_NULL, NULL);
    }
}

ir_id_t ir_next(ir_function_t* function) {
    return function->next_value_id++;
}

ir_instruction_t* ir_emit(ir_op_t op, ir_function_t* function, ir_block_t* block) {
    if (block->instruction_count >= block->instruction_capacity) {
        block->instruction_capacity = block->instruction_capacity ? block->instruction_capacity * 2 : 8;
        block->instructions = realloc(block->instructions, sizeof(ir_instruction_t) * block->instruction_capacity);
    }

    ir_instruction_t* instr = &block->instructions[block->instruction_count++];
    instr->op = op;
    instr->result = ir_next(function);
    instr->type = IR_TYPE_NULL;

    return instr;
}

ir_id_t ir_generate_ssa(ast_node_t* node, ir_function_t* function, ir_block_t* block) {
    switch(node->kind) {
        case AST_LITERAL:
            ir_op_t op;
            switch (node->literal_kind) {
                case AST_LITERAL_NUMBER:
                    op = IR_CONST_NUMBER;
                    break;
                case AST_LITERAL_STRING:
                    op = IR_CONST_STRING;
                    break;
                case AST_LITERAL_BOOLEAN:
                    op = IR_CONST_BOOLEAN;
                    break;
                case AST_LITERAL_NULL:
                    op = IR_CONST_NULL;
                    break;
                case AST_LITERAL_IDENTIFIER:
                    ir_var_t var_id = map_get(function->symbol_table, node->value, node->length);
                    ir_instruction_t* var_instr = ir_emit(IR_LOAD, function, block);
                    var_instr->var.var_id = var_id;

                    return var_instr->result;
            }

            ir_instruction_t* instr = ir_emit(op, function, block);
            instr->constant.value = ir_load_literal(node->value, node->length, node->literal_kind);
            return instr->result;
        case AST_PROMPT:
            ir_instruction_t* prompt_instr = ir_emit(IR_PROMPT, function, block);
            return prompt_instr->result;
        case AST_RANDOM:
            ir_instruction_t* random_instr = ir_emit(IR_RANDOM, function, block);
            return random_instr->result;
            
    }
}