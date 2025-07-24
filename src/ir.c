#include "ir.h"
#include "parser.h"
#include "jit/value.h"

const char* debug_ir_op_string(ir_op_t op) {
    switch (op) {
        case IR_CONST_NUMBER: return "CONST_NUMBER";
        case IR_CONST_STRING: return "CONST_STRING";
        case IR_CONST_BOOLEAN: return "CONST_BOOLEAN";
        case IR_CONST_NULL: return "CONST_NULL";
        case IR_CONST_ARRAY: return "CONST_ARRAY";
        case IR_LOAD: return "LOAD";
        case IR_STORE: return "STORE";
        case IR_ADD: return "ADD";
        case IR_SUB: return "SUB";
        case IR_MUL: return "MUL";
        case IR_DIV: return "DIV";
        case IR_MOD: return "MOD";
        case IR_POW: return "POW";
        case IR_NEG: return "NEG";
        case IR_LT: return "LT";
        case IR_GT: return "GT";
        case IR_EQ: return "EQ";
        case IR_AND: return "AND";
        case IR_OR: return "OR";
        case IR_NOT: return "NOT";
        case IR_GUARD: return "GUARD";
        case IR_COERCE: return "COERCE";
        case IR_BRANCH: return "BRANCH";
        case IR_JUMP: return "JUMP";
        case IR_CALL: return "CALL";
        case IR_RETURN: return "RETURN";
        case IR_OUTPUT: return "OUTPUT";
        case IR_DUMP: return "DUMP";
        case IR_RANDOM: return "RANDOM";
        case IR_PROMPT: return "PROMPT";
        case IR_QUIT: return "QUIT";
        case IR_PHI: return "PHI";
        case IR_BLOCK: return "BLOCK";
        case IR_BOX: return "BOX";
        case IR_ASCII: return "ASCII";
        case IR_PRIME: return "PRIME";
        case IR_ULTIMATE: return "ULTIMATE";
        case IR_LENGTH: return "LENGTH";
        case IR_GET: return "GET";
        case IR_SET: return "SET";
        default: panic("Unknown IR operation");
    }
};

ir_worklist_t* ir_create_worklist(size_t capacity, arena_t* arena) {
    ir_worklist_t* worklist = arena_alloc(arena, sizeof(ir_worklist_t));
    if (!worklist) panic("Failed to allocate memory for IR worklist");

    worklist->items = arena_alloc(arena, sizeof(ir_worklist_item_t*) * capacity);
    if (!worklist->items) {
        panic("Failed to allocate memory for IR worklist items");
    }

    worklist->arena = arena;
    worklist->size = 0;
    worklist->capacity = capacity;

    return worklist;
}

void ir_worklist_add(ir_worklist_t* worklist, ast_node_t* node, ir_block_t* block, int state) {
    if (worklist->size >= worklist->capacity) {
        worklist->capacity *= 2;
        worklist->items = arena_realloc(worklist->arena, worklist->items, sizeof(ir_worklist_item_t*) * worklist->capacity);
        if (!worklist->items) panic("Failed to reallocate memory for IR worklist items");
    }

    ir_worklist_item_t* item = arena_alloc(worklist->arena, sizeof(ir_worklist_item_t));
    if (!item) panic("Failed to allocate memory for IR worklist item");

    item->node = node;
    item->block = block;
    item->state = state;

    worklist->items[worklist->size++] = item;
}

ir_worklist_item_t* ir_worklist_remove(ir_worklist_t* worklist) {
    if (worklist->size == 0) {
        panic("Attempted to remove from an empty IR worklist");
    }

    ir_worklist_item_t* item = worklist->items[--worklist->size];
    worklist->items[worklist->size] = NULL;

    return item;
}

v_t ir_load_literal(const char* value, size_t length, ast_literal_kind_t kind) {
    switch (kind) {
        case AST_LITERAL_BOOLEAN:
            return v_create(TYPE_BOOLEAN, (void*)(intptr_t) (value[0] == 'T'));
        case AST_LITERAL_STRING:
            return v_create_string(value, length);
        case AST_LITERAL_NUMBER:
            return v_create(TYPE_NUMBER, (void*)(intptr_t) strtol(value, NULL, 10));
        case AST_LITERAL_NULL:
            return v_create(TYPE_NULL, NULL);
        case AST_LITERAL_ARRAY:
            return v_create_list(2);
        case AST_LITERAL_IDENTIFIER:
            panic("Identifiers should not be loaded as literals in IR");
    }
}

ir_id_t ir_next(ir_function_t* function) {
    return function->next_value_id++;
}

ir_id_t ir_reverse(ir_function_t* function) {
    return function->next_value_id--;
}

ir_instruction_t* ir_emit(ir_op_t op, ir_function_t* function, ir_block_t* block) {
    if (block->instruction_count >= block->instruction_capacity) {
        block->instruction_capacity = block->instruction_capacity ? block->instruction_capacity * 2 : 8;
        block->instructions = arena_realloc(function->arena, block->instructions, sizeof(ir_instruction_t) * block->instruction_capacity);
    }

    ir_instruction_t* instr = &block->instructions[block->instruction_count++];
    instr->op = op;
    instr->result = ir_next(function);
    instr->type = IR_TYPE_NULL;

    return instr;
}

void ir_successor_add(ir_block_t* block, ir_id_t successor) {
    if (block->successor_count >= block->successor_capacity) {
        block->successor_capacity = block->successor_capacity ? block->successor_capacity * 2 : 8;
        block->successors = arena_realloc(block->arena, block->successors, sizeof(ir_id_t) * block->successor_capacity);
    }

    if (!block->successors) panic("Failed to allocate memory for successors in block %d", block->id);

    block->successors[block->successor_count++] = successor;
}

void ir_successor_emit(ir_function_t* function, ir_block_t* block, ir_block_t* jump) {
    if (block->successor_count == 0) {
        ir_instruction_t* instr = ir_emit(IR_JUMP, function, block);
        instr->jump.block = jump;
        ir_successor_add(block, jump->id);
    } else {
        for (int i = 0; i < block->successor_count; ++i) {
            ir_block_t* succ = NULL;
            for (int j = 0; j < function->block_count; ++j) {
                if (function->blocks[j]->id == block->successors[i] && function->blocks[j] != jump) {
                    succ = function->blocks[j];
                    break;
                }
            }
            
            if (succ) {
                ir_successor_emit(function, succ, jump);
            }
        }
    }
}

void ir_successor_branch(ir_function_t* function, ir_block_t* block, ir_block_t* truthy, ir_block_t* falsey, ir_id_t condition) {
    if (block->successor_count == 0) {
        ir_instruction_t* instr = ir_emit(IR_BRANCH, function, block);
        instr->branch.condition = condition;
        instr->branch.truthy = truthy;
        instr->branch.falsey = falsey;

        ir_successor_add(block, truthy->id);
    } else {
        for (int i = 0; i < block->successor_count; ++i) {
            ir_block_t* succ = NULL;
            for (int j = 0; j < function->block_count; ++j) {
                if (function->blocks[j]->id == block->successors[i] && function->blocks[j] != truthy && function->blocks[j] != falsey) {
                    succ = function->blocks[j];
                    break;
                }
            }
            
            if (succ) {
                ir_successor_branch(function, succ, truthy, falsey, condition);
            }
        }
    }
}

ir_block_t* ir_create_block(ir_function_t* function) {
    if (function->block_count >= function->block_capacity) {
        function->block_capacity = function->block_capacity ? function->block_capacity * 2 : 8;
        function->blocks = arena_realloc(function->arena, function->blocks, sizeof(ir_block_t*) * function->block_capacity);
    }

    ir_block_t* block = arena_alloc(function->arena, sizeof(ir_block_t));
    block->id = function->next_block_id++;
    block->instructions = arena_alloc(function->arena, sizeof(ir_instruction_t) * 8);
    block->instruction_count = 0;
    block->instruction_capacity = 8;

    block->phis = arena_alloc(function->arena, sizeof(ir_instruction_t) * 2);
    block->phi_count = 0;
    block->phi_capacity = 2;

    block->predecessors = arena_alloc(function->arena, sizeof(ir_id_t) * 2);
    block->predecessor_count = 0;
    block->successors = arena_alloc(function->arena, sizeof(ir_id_t) * 3);
    block->successor_count = 0;
    block->successor_capacity = 3;

    block->arena = function->arena;
    function->blocks[function->block_count++] = block;
    return block;
}

ir_id_t ir_generate_ssa(ast_node_t* root, ir_function_t* function, ir_block_t* entry) {
    ir_worklist_t* worklist = ir_create_worklist(64, function->arena);
    ir_worklist_add(worklist, root, entry, 0);

    while (worklist->size > 0) {
        ir_worklist_item_t* item = ir_worklist_remove(worklist);
        ast_node_t* node = item->node;
        ir_block_t* block = item->block;

        switch (node->kind) {
            case AST_LITERAL:
                ir_op_t op;
                switch (node->literal_kind) {
                    case AST_LITERAL_NUMBER: op = IR_CONST_NUMBER; break;
                    case AST_LITERAL_STRING: op = IR_CONST_STRING; break;
                    case AST_LITERAL_BOOLEAN: op = IR_CONST_BOOLEAN; break;
                    case AST_LITERAL_NULL: op = IR_CONST_NULL; break;
                    case AST_LITERAL_ARRAY: op = IR_CONST_ARRAY; break;
                    default: panic("Unknown literal type");
                }

                ir_instruction_t* instr = ir_emit(op, function, block);
                instr->constant.value = ir_load_literal(node->value, node->length, node->literal_kind);
                node->result = instr->result;
                break;
            case AST_IDENTIFIER:
                instr = ir_emit(IR_LOAD, function, block);
                ir_var_t var_id = map_get(function->symbol_table, node->value, node->length);
                if (var_id == -1) {
                    map_set(function->symbol_table, node->value, node->length, ++function->var_id);
                    var_id = function->var_id;
                }

                instr->var.var_id = var_id;
                node->result = instr->result;
                break;
            case AST_PROMPT:
                instr = ir_emit(IR_PROMPT, function, block);
                node->result = instr->result;
                break;
            case AST_RANDOM:
                instr = ir_emit(IR_RANDOM, function, block);
                node->result = instr->result;
                break;
            case AST_BLOCK:
                if (item->state == 0) {
                    ir_block_t* generated_block = ir_create_block(function);
                    instr = ir_emit(IR_BLOCK, function, block);
                    instr->block.function = (v_t) generated_block;
                    node->instruction = instr;

                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg1, generated_block, 0);
                } else {
                    node->instruction->block.result_id = node->arg1->result;
                    node->result = instr->result;
                }
                break;
            case AST_CALL:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_CALL, function, block);
                    instr->generic.operand_count = 1;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t));
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                }
                break;
            case AST_QUIT:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_QUIT, function, block);
                    instr->generic.operand_count = 1;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t));
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                }
                break;
            case AST_DUMP:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_DUMP, function, block);
                    instr->generic.operand_count = 1;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t));
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                }
                break;
            case AST_OUTPUT:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_OUTPUT, function, block);
                    instr->generic.operand_count = 1;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t));
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                }
                break;
            case AST_LENGTH:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_LENGTH, function, block);
                    instr->generic.operand_count = 1;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t));
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                }
                break;
            case AST_NOT:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_NOT, function, block);
                    instr->generic.operand_count = 1;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t));
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                }
                break;
            case AST_NEGATE:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_NEG, function, block);
                    instr->generic.operand_count = 1;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t));
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                }
                break;
            case AST_ASCII:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_ASCII, function, block);
                    instr->generic.operand_count = 1;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t));
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                }
                break;
            case AST_BOX:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_BOX, function, block);
                    instr->generic.operand_count = 1;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t));
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                }
                break;
            case AST_PRIME:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_PRIME, function, block);
                    instr->generic.operand_count = 1;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t));
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                }
                break;
            case AST_ULTIMATE:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_ULTIMATE, function, block);
                    instr->generic.operand_count = 1;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t));
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                }
                break;
            case AST_PLUS:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_ADD, function, block);
                    instr->generic.operand_count = 2;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t) * 2);
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                    instr->generic.operands[1] = node->arg2->result;
                }
                break;
            case AST_MINUS:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_SUB, function, block);
                    instr->generic.operand_count = 2;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t) * 2);
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                    instr->generic.operands[1] = node->arg2->result;
                }
                break;
            case AST_MULTIPLY:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_MUL, function, block);
                    instr->generic.operand_count = 2;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t) * 2);
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                    instr->generic.operands[1] = node->arg2->result;
                }
                break;
            case AST_DIVIDE:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_DIV, function, block);
                    instr->generic.operand_count = 2;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t) * 2);
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                    instr->generic.operands[1] = node->arg2->result;
                }
                break;
            case AST_MODULO:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_MOD, function, block);
                    instr->generic.operand_count = 2;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t) * 2);
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                    instr->generic.operands[1] = node->arg2->result;
                }
                break;
            case AST_POWER:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_POW, function, block);
                    instr->generic.operand_count = 2;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t) * 2);
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                    instr->generic.operands[1] = node->arg2->result;
                }
                break;
            case AST_LESS:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_LT, function, block);
                    instr->generic.operand_count = 2;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t) * 2);
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                    instr->generic.operands[1] = node->arg2->result;
                }
                break;
            case AST_GREATER:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_GT, function, block);
                    instr->generic.operand_count = 2;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t) * 2);
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                    instr->generic.operands[1] = node->arg2->result;
                }
                break;
            case AST_EQUAL:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_EQ, function, block);
                    instr->generic.operand_count = 2;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t) * 2);
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                    instr->generic.operands[1] = node->arg2->result;
                }
                break;
            case AST_AND:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_AND, function, block);
                    instr->generic.operand_count = 2;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t) * 2);
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                    instr->generic.operands[1] = node->arg2->result;
                }
                break;
            case AST_OR:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_OR, function, block);
                    instr->generic.operand_count = 2;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t) * 2);
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                    instr->generic.operands[1] = node->arg2->result;
                }
                break;
            case AST_EXPR:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    node->result = node->arg2->result;
                }
                break;
            case AST_ASSIGN:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                } else {
                    instr = ir_emit(IR_STORE, function, block);
                    var_id = map_get(function->symbol_table, node->arg1->value, node->arg1->length);
                    if (var_id == -1) {
                        map_set(function->symbol_table, node->arg1->value, node->arg1->length, ++function->var_id);
                        var_id = function->var_id;
                    }

                    instr->var.var_id = var_id;
                    instr->var.value = node->arg2->result;
                    node->result = node->arg2->result;
                }
                break;
            case AST_IF:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else if (item->state == 1) {
                    ir_block_t* then_block = ir_create_block(function);
                    ir_block_t* else_block = ir_create_block(function);

                    ir_worklist_add(worklist, node, block, 2);
                    ir_worklist_add(worklist, node->arg3, else_block, 0);
                    ir_worklist_add(worklist, node->arg2, then_block, 0);

                    instr = ir_emit(IR_BRANCH, function, block);
                    instr->branch.condition = node->arg1->result;
                    node->instruction = instr;
                    instr->branch.truthy = then_block;
                    instr->branch.falsey = else_block;

                    ir_successor_add(block, then_block->id);
                    ir_successor_add(block, else_block->id);
                } else {
                    ir_block_t* merge_block = ir_create_block(function);
                    ir_successor_emit(function, node->instruction->branch.truthy, merge_block);
                    ir_successor_emit(function, node->instruction->branch.falsey, merge_block);

                    ir_instruction_t* phi = ir_emit(IR_PHI, function, merge_block);

                    ir_successor_add(node->instruction->branch.truthy, merge_block->id);
                    ir_successor_add(node->instruction->branch.falsey, merge_block->id);

                    phi->phi.phi_count = 2;
                    phi->phi.phi_values = arena_alloc(function->arena, sizeof(ir_id_t) * 2);
                    phi->phi.phi_blocks = arena_alloc(function->arena, sizeof(ir_id_t) * 2);
                    phi->phi.phi_values[0] = node->arg2->result;
                    phi->phi.phi_values[1] = node->arg3->result;
                    phi->phi.phi_blocks[0] = node->instruction->branch.truthy->id;
                    phi->phi.phi_blocks[1] = node->instruction->branch.falsey->id;

                    node->result = phi->result;

                    for (size_t i = 0; i < worklist->size; ++i) {
                        if (worklist->items[i]->block == block) {
                            worklist->items[i]->block = merge_block;
                        }
                    }
                }
                break;
            case AST_WHILE:
                if (item->state == 0) {
                    ir_block_t* condition_block = ir_create_block(function);
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg1, condition_block, 0);

                    instr = ir_emit(IR_JUMP, function, block);
                    instr->jump.block = condition_block;
                    node->block = condition_block;
                    ir_successor_add(block, condition_block->id);
                } else if (item->state == 1) {
                    ir_block_t* body_block = ir_create_block(function);
                    ir_block_t* exit_block = ir_create_block(function);

                    ir_worklist_add(worklist, node, block, 2);
                    ir_worklist_add(worklist, node->arg2, body_block, 0);

                    instr = ir_emit(IR_BRANCH, function, node->block);
                    instr->branch.condition = node->arg1->result;
                    node->instruction = instr;
                    instr->branch.truthy = body_block;
                    instr->branch.falsey = exit_block;

                    ir_successor_add(block, body_block->id);
                    ir_successor_add(block, exit_block->id);
                } else {
                    node->result = node->instruction->result;
                    ir_successor_emit(function, node->instruction->branch.truthy, node->block);

                    for (size_t i = 0; i < worklist->size; ++i) {
                        if (worklist->items[i]->block == block) {
                            worklist->items[i]->block = node->instruction->branch.falsey;
                        }
                    }
                }
                break;
            case AST_GET:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg3, block, 0);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_GET, function, block);
                    instr->generic.operand_count = 3;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t) * 3);
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                    instr->generic.operands[1] = node->arg2->result;
                    instr->generic.operands[2] = node->arg3->result;
                }
                break;
            case AST_SET:
                if (item->state == 0) {
                    ir_worklist_add(worklist, node, block, 1);
                    ir_worklist_add(worklist, node->arg4, block, 0);
                    ir_worklist_add(worklist, node->arg3, block, 0);
                    ir_worklist_add(worklist, node->arg2, block, 0);
                    ir_worklist_add(worklist, node->arg1, block, 0);
                } else {
                    instr = ir_emit(IR_SET, function, block);
                    instr->generic.operand_count = 4;
                    instr->generic.operands = arena_alloc(function->arena, sizeof(ir_id_t) * 4);
                    node->result = instr->result;
                    instr->generic.operands[0] = node->arg1->result;
                    instr->generic.operands[1] = node->arg2->result;
                    instr->generic.operands[2] = node->arg3->result;
                    instr->generic.operands[3] = node->arg4->result;
                }
                break;
            default: panic("Unknown AST node kind: %d", node->kind);
        }
    }

    return root->result;
}

ir_function_t* ir_create(ast_node_t* tree, arena_t* arena, map_t* symbol_table) {
    ir_function_t* function = arena_alloc(arena, sizeof(ir_function_t));
    function->blocks = arena_alloc(arena, sizeof(ir_block_t*) * 8);
    function->block_count = 0;
    function->block_capacity = 8;

    function->next_value_id = 0;
    function->next_block_id = 0;

    function->symbol_table = symbol_table;
    function->arena = arena;
    function->var_id = 0;

    ir_block_t* entry_block = ir_create_block(function);
    function->block = entry_block;
    ir_generate_ssa(tree, function, entry_block);

    printf("IR Function:\n");
    for (int i = 0; i < function->block_count; ++i) {
        ir_block_t* block = function->blocks[i];
        printf("  Block %zu (id=%d):\n", i, block->id);
        for (int j = 0; j < block->instruction_count; ++j) {
            ir_instruction_t* instr = &block->instructions[j];
            printf("    [%d] op=%d", instr->result, instr->op);

            switch (instr->op) {
                case IR_CONST_NUMBER:
                    printf(" NUMBER CONST value=%ld", (long)instr->constant.value >> 3);
                    break;
                case IR_CONST_BOOLEAN:
                    printf(" BOOLEAN CONST value=%ld", (long)instr->constant.value >> 3);
                    break;
                case IR_CONST_STRING:
                    printf(" STRING CONST");
                    break;
                case IR_CONST_NULL:
                    printf(" NULL CONST");
                    break;
                case IR_CONST_ARRAY:
                    printf(" @");
                    break;
                case IR_LOAD:
                    printf(" LOAD var_id=%d", instr->var.var_id);
                    break;
                case IR_STORE:
                    printf(" STORE var_id=%d value=%d", instr->var.var_id, instr->var.value);
                    break;
                case IR_BLOCK:
                    printf(" BLOCK function=%p result_id=%d", (void*)(intptr_t)instr->block.function, instr->block.result_id);
                    break;
                case IR_BRANCH:
                    printf(" BRANCH true=%d false=%d condition=%d", instr->branch.truthy->id, instr->branch.falsey->id, instr->branch.condition);
                    break;
                case IR_JUMP:
                    printf(" JUMP block_id=%d", instr->jump.block->id);
                    break;
                case IR_PHI:
                    printf(" PHI phi_count=%d", instr->phi.phi_count);
                    printf(" values=");
                    for (int k = 0; k < instr->phi.phi_count; ++k) {
                        printf("%d ", instr->phi.phi_values[k]);
                    }
                    printf(" blocks=");
                    for (int k = 0; k < instr->phi.phi_count; ++k) {
                        printf("%d ", instr->phi.phi_blocks[k]);
                    }
                    break;
                case IR_RANDOM:
                    printf(" RANDOM");
                    break;
                case IR_PROMPT:
                    printf(" PROMPT");
                    break;
                default:
                    printf(" %s operands=", debug_ir_op_string(instr->op));
                    for (int k = 0; k < instr->generic.operand_count; ++k) {
                        printf("%d ", instr->generic.operands[k]);
                    }
                    break;
                
            }

            printf("\n");
        }
    }


    return function;
}