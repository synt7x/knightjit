#ifndef PARSER_H
#define PARSER_H

#include "arena.h"
#include "lexer.h"

typedef int ir_id_t;
typedef struct ir_instruction ir_instruction_t;
typedef struct ir_block ir_block_t;

typedef enum ast_kind {
    AST_IDENTIFIER,
    AST_LITERAL,
    AST_PROMPT,
    AST_RANDOM,
    AST_BLOCK,
    AST_CALL,
    AST_QUIT,
    AST_DUMP,
    AST_OUTPUT,
    AST_LENGTH,
    AST_NOT,
    AST_NEGATE,
    AST_ASCII,
    AST_BOX,
    AST_PRIME,
    AST_ULTIMATE,
    AST_PLUS,
    AST_MINUS,
    AST_MULTIPLY,
    AST_DIVIDE,
    AST_MODULO,
    AST_POWER,
    AST_LESS,
    AST_GREATER,
    AST_EQUAL,
    AST_AND,
    AST_OR,
    AST_EXPR,
    AST_ASSIGN,
    AST_WHILE,
    AST_IF,
    AST_GET,
    AST_SET,
} ast_kind_t;

typedef enum ast_literal_kind {
    AST_LITERAL_STRING,
    AST_LITERAL_NUMBER,
    AST_LITERAL_BOOLEAN,
    AST_LITERAL_NULL,
    AST_LITERAL_IDENTIFIER,
    AST_LITERAL_ARRAY
} ast_literal_kind_t;

typedef struct ast_node ast_node_t;

struct ast_node {
    ast_kind_t kind;
    ir_id_t result;
    ir_instruction_t* instruction;
    ir_block_t* block;
    ir_block_t* top;

    union {
        struct {
            ast_node_t* arg1;
            ast_node_t* arg2;
            ast_node_t* arg3;
            ast_node_t* arg4;
        };

        struct {
            ast_literal_kind_t literal_kind;
            const char* value;
            int length;
        };
    };
};

ast_node_t* parse(lexer_t* lexer, arena_t* arena);
ast_node_t* expression(lexer_t* lexer, arena_t* arena);

#endif