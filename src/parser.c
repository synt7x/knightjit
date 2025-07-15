#include "arena.h"
#include "debug.h"

#include "parser.h"
#include "lexer.h"

static inline ast_node_t* literal(ast_node_t* node, token_t token) {
    node->kind = AST_LITERAL;
    node->value = token.value;
    node->length = token.length;

    switch (token.type) {
        case TK_NUMBER:
            node->literal_kind = AST_LITERAL_NUMBER;
            break;
        case TK_STRING:
            node->literal_kind = AST_LITERAL_STRING;
            break;
        case TK_TRUE: case TK_FALSE:
            node->literal_kind = AST_LITERAL_BOOLEAN;
            break;
        case TK_NULL:
            node->literal_kind = AST_LITERAL_NULL;
            break;
        case TK_IDENTIFIER:
            node->literal_kind = AST_LITERAL_IDENTIFIER;
            break;
        case TK_LIST:
            node->literal_kind = AST_LITERAL_ARRAY;
            break;
        default:
            panic("Unexpected token type for literal: %d", token.type);
            break;
    }

    return node;
}

static inline ast_node_t* nullary(ast_node_t* node, token_t token) {
    switch (token.type) {
        case TK_PROMPT:
            node->kind = AST_PROMPT;
            break;
        case TK_RANDOM:
            node->kind = AST_RANDOM;
            break;
        default:
            panic("Unexpected token type for nullary operation: %d", token.type);
            break;
    }

    return node;
}

static inline ast_node_t* unary(ast_node_t* node, lexer_t* lexer, arena_t* arena) {
    switch (lexer->t.type) {
        case TK_BLOCK:
            node->kind = AST_BLOCK;
            break;
        case TK_CALL:
            node->kind = AST_CALL;
            break;
        case TK_QUIT:
            node->kind = AST_QUIT;
            break;
        case TK_DUMP:
            node->kind = AST_DUMP;
            break;
        case TK_OUTPUT:
            node->kind = AST_OUTPUT;
            break;
        case TK_LENGTH:
            node->kind = AST_LENGTH;
            break;
        case TK_NOT:
            node->kind = AST_NOT;
            break;
        case TK_NEGATE:
            node->kind = AST_NEGATE;
            break;
        case TK_ASCII:
            node->kind = AST_ASCII;
            break;
        case TK_BOX:
            node->kind = AST_BOX;
            break;
        case TK_PRIME:
            node->kind = AST_PRIME;
            break;
        case TK_ULTIMATE:
            node->kind = AST_ULTIMATE;
            break;
        default:
            panic("Unexpected token type for unary operation");
            break;
    }

    node->arg1 = expression(lexer, arena);
    if (!node->arg1) {
        panic("Expected expression after unary operator: %c", *lexer->t.value);
    }

    return node;
}

static inline ast_node_t* binary(ast_node_t* node, lexer_t* lexer, arena_t* arena) {
    switch (lexer->t.type) {
        case TK_PLUS:
            node->kind = AST_PLUS;
            break;
        case TK_MINUS:
            node->kind = AST_MINUS;
            break;
        case TK_MULTIPLY:
            node->kind = AST_MULTIPLY;
            break;
        case TK_DIVIDE:
            node->kind = AST_DIVIDE;
            break;
        case TK_MODULO:
            node->kind = AST_MODULO;
            break;
        case TK_POWER:
            node->kind = AST_POWER;
            break;
        case TK_LESS:
            node->kind = AST_LESS;
            break;
        case TK_GREATER:
            node->kind = AST_GREATER;
            break;
        case TK_EQUAL:
            node->kind = AST_EQUAL;
            break;
        case TK_AND:
            node->kind = AST_AND;
            break;
        case TK_OR:
            node->kind = AST_OR;
            break;
        case TK_EXPR:
            node->kind = AST_EXPR;
            break;
        case TK_ASSIGN:
            node->kind = AST_ASSIGN;
            break;
        case TK_WHILE:
            node->kind = AST_WHILE;
            break;
        default:
            panic("Unexpected token type for binary operation");
            break;
    }

    node->arg1 = expression(lexer, arena);
    if (!node->arg1) {
        panic("Expected expression after binary operator: %c", *lexer->t.value);
    }

    node->arg2 = expression(lexer, arena);
    if (!node->arg2) {
        panic("Expected second expression after binary operator: %c", *lexer->t.value);
    }

    return node;
}

static inline ast_node_t* ternary(ast_node_t* node, lexer_t* lexer, arena_t* arena) {
    switch (lexer->t.type) {
        case TK_IF:
            node->kind = AST_IF;
            break;
        case TK_GET:
            node->kind = AST_GET;
            break;
        default:
            panic("Unexpected token type for ternary operation");
            break;
    }

    node->arg1 = expression(lexer, arena);
    if (!node->arg1) {
        panic("Expected expression after ternary operator: %c", *lexer->t.value);
    }

    node->arg2 = expression(lexer, arena);
    if (!node->arg2) {
        panic("Expected second expression after ternary operator: %c", *lexer->t.value);
    }

    node->arg3 = expression(lexer, arena);
    if (!node->arg3) {
        panic("Expected third expression after ternary operator: %c", *lexer->t.value);
    }

    return node;
}

static inline ast_node_t* quaternary(ast_node_t* node, lexer_t* lexer, arena_t* arena) {
    if (lexer->t.type != TK_SET) {
        panic("Expected SET token for quaternary operation");
    }

    node->kind = AST_SET;

    node->arg1 = expression(lexer, arena);
    if (!node->arg1) {
        panic("Expected first expression after SET operator: %c", *lexer->t.value);
    }

    node->arg2 = expression(lexer, arena);
    if (!node->arg2) {
        panic("Expected second expression after SET operator: %c", *lexer->t.value);
    }

    node->arg3 = expression(lexer, arena);
    if (!node->arg3) {
        panic("Expected third expression after SET operator: %c", *lexer->t.value);
    }

    node->arg4 = expression(lexer, arena);
    if (!node->arg4) {
        panic("Expected fourth expression after SET operator: %c", *lexer->t.value);
    }

    return node;
}

ast_node_t* expression(lexer_t* lexer, arena_t* arena) {
    token_t token = consume(lexer);
    ast_node_t* node = arena_alloc(arena, sizeof(ast_node_t));

    switch (token.type) {
        case TK_EOF: panic("Unexpected end of input");
        case TK_NUMBER: case TK_STRING: case TK_TRUE: case TK_FALSE: case TK_NULL: case TK_IDENTIFIER: case TK_LIST:
            return literal(node, token);
        case TK_PROMPT: case TK_RANDOM:
            return nullary(node, token);
        case TK_BLOCK: case TK_CALL: case TK_QUIT: case TK_DUMP: case TK_OUTPUT: case TK_LENGTH:
        case TK_NOT: case TK_NEGATE: case TK_ASCII: case TK_BOX: case TK_PRIME: case TK_ULTIMATE:
            return unary(node, lexer, arena);
        case TK_PLUS: case TK_MINUS: case TK_MULTIPLY: case TK_DIVIDE: case TK_MODULO:
        case TK_POWER: case TK_LESS: case TK_GREATER: case TK_EQUAL: case TK_AND: case TK_OR:
        case TK_EXPR: case TK_ASSIGN: case TK_WHILE:
            return binary(node, lexer, arena);
        case TK_IF: case TK_GET:
            return ternary(node, lexer, arena);
        case TK_SET:
            return quaternary(node, lexer, arena);
        default:
            panic("Unexpected token type: %d", token.type);
            break;
    }
}

ast_node_t* parse(lexer_t* lexer, arena_t* arena) {
    ast_node_t* tree = expression(lexer, arena);
    consume(lexer);

    if (lexer->t.type != TK_EOF) {
        panic("Unexpected token after body: %s", lexer->t.value);
    }

    if (!tree) {
        panic("Failed to parse expression");
    }

    return tree;
}