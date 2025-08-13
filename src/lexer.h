#ifndef LEXER_H
#define LEXER_H

typedef enum token_type {
    TK_NONE,
    
    // Literals
    TK_NUMBER,
    TK_STRING,
    TK_TRUE,
    TK_FALSE,
    TK_NULL,
    TK_IDENTIFIER,
    TK_LIST,

    // Nullary
    TK_PROMPT,
    TK_RANDOM,

    // Unary
    TK_BLOCK,
    TK_CALL,
    TK_QUIT,
    TK_DUMP,
    TK_OUTPUT,
    TK_LENGTH,
    TK_NOT,
    TK_NEGATE,
    TK_ASCII,
    TK_BOX,
    TK_PRIME,
    TK_ULTIMATE,

    // Binary
    TK_PLUS,
    TK_MINUS,
    TK_MULTIPLY,
    TK_DIVIDE,
    TK_MODULO,
    TK_POWER,
    TK_LESS,
    TK_GREATER,
    TK_EQUAL,
    TK_AND,
    TK_OR,
    TK_EXPR,
    TK_ASSIGN,
    TK_WHILE,

    // Ternary
    TK_IF,
    TK_GET,

    // Quaternary
    TK_SET,

    // EOF
    TK_EOF
} token_type_t;

typedef struct token {
    token_type_t type;
    const char *value;
    int length;
    int line;
} token_t;

typedef struct lexer {
    const char* buffer;
    int size;
    int current;
    int linenumber;
    token_t t;
} lexer_t;

void l_init(lexer_t* lexer, const char* input, int size);
void l_load(token_t* token, lexer_t* lexer);

token_t l_peek(lexer_t* lexer);
token_t l_consume(lexer_t* lexer);
token_t l_accept(lexer_t* lexer, token_type_t type);
token_t l_expect(lexer_t* lexer, token_type_t type);

#endif