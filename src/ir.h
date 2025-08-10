#ifndef IR_H
#define IR_H

#include "map.h"
#include "arena.h"
#include "cli.h"

#include "parser.h"
#include "jit/value.h"

typedef int ir_id_t;
typedef int ir_var_t;

typedef enum ir_type {
    IR_TYPE_NUMBER,
    IR_TYPE_STRING,
    IR_TYPE_BOOLEAN,
    IR_TYPE_NULL,
    IR_TYPE_ARRAY,
    IR_TYPE_BLOCK
} ir_type_t;

typedef enum ir_op {
    IR_CONST_NUMBER,
    IR_CONST_STRING,
    IR_CONST_BOOLEAN,
    IR_CONST_NULL,
    IR_CONST_ARRAY,

    IR_LOAD,
    IR_STORE,

    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_MOD,
    IR_POW,
    IR_NEG,

    IR_LT,
    IR_GT,
    IR_EQ,

    IR_AND,
    IR_OR,
    IR_NOT,

    IR_GUARD,
    IR_COERCE,

    IR_BRANCH,
    IR_JUMP,
    IR_CALL,
    IR_RETURN,

    IR_OUTPUT,
    IR_DUMP,
    IR_RANDOM,
    IR_PROMPT,
    IR_QUIT,
    IR_BOX,
    IR_ASCII,
    IR_PRIME,
    IR_ULTIMATE,
    IR_LENGTH,
    IR_GET,
    IR_SET,

    IR_PHI,
    IR_BLOCK,

    IR_SAVE,
    IR_RESTORE,
} ir_op_t;

typedef struct ir_block ir_block_t;

typedef struct ir_instruction {
    ir_id_t result;
    ir_op_t op;
    ir_type_t type;

    union {
        struct { // IR_*
            ir_id_t* operands;
            int operand_count;
        } generic;

        struct { // IR_CONST_NUMBER, IR_CONST_STRING, IR_CONST_BOOLEAN, IR_CONST_NULL
            v_t value;
        } constant;

        struct { // IR_LOAD, IR_STORE
            ir_var_t var_id;
            ir_id_t value;
        } var;

        struct { // IR_PHI
            ir_id_t* phi_values;
            ir_block_t** phi_blocks;
            int phi_count;
            int phi_capacity;
        } phi;

        struct { // IR_BRANCH
            ir_block_t* truthy;
            ir_block_t* falsey;
            ir_id_t condition;
        } branch;

        struct {
            ir_block_t* block;
        } jump;

        struct {
            ir_id_t result_id;
            ir_block_t* function;
        } block;
    };
} ir_instruction_t;

typedef struct ir_block {
    ir_id_t id;
    ir_instruction_t* instructions;
    int instruction_count;
    int instruction_capacity;

    ir_instruction_t* phis;
    int phi_count;
    int phi_capacity;

    int terminator;

    ir_id_t* predecessors;
    int predecessor_count;

    ir_id_t* successors;
    int successor_count;
    int successor_capacity;

    ir_id_t follow_id;

    arena_t* arena;
} ir_block_t;

typedef struct ir_function {
    ir_block_t** blocks;
    int block_count;
    int block_capacity;

    ir_id_t next_value_id;
    ir_id_t next_block_id;

    map_t* symbol_table;
    arena_t* arena;
    ir_var_t var_id;

    ir_block_t* block;
} ir_function_t;

typedef struct ir_worklist_item {
    ast_node_t* node;
    ir_block_t* block;
    int state;
} ir_worklist_item_t;

typedef struct ir_worklist {
    ir_worklist_item_t** items;
    size_t size;
    size_t capacity;
    arena_t* arena;
} ir_worklist_t;

const char* debug_ir_op_string(ir_op_t op);
ir_function_t* ir_create(ast_node_t* tree, arena_t* arena, map_t* symbol_table, cli_config_t* config);

#endif