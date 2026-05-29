#pragma once

#include <cstdint>
#include <string_view>

enum class token_type : uint8_t {
    NONE,
    ERROR,
    STRING,
    VARIABLE,
    TRUE,
    FALSE,
    NIL,
    LIST,
    BLOCK,
    ARRAY,
    PROMPT,
    RANDOM,
    CALL,
    QUIT,
    DUMP,
    LENGTH,
    NOT,
    NEGATE,
    ASCII,
    BOX,
    HEAD,
    TAIL,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    MOD,
    POWER,
    GREATER,
    LESS,
    COMPARE,
    AND,
    OR,
    EXPR,
    EQUAL,
    WHILE,
    IF,
    GET,
    SET,
    ARGS
};

struct token {
    uint32_t start;
    uint16_t length;
    token_type type = token_type::NONE;
};

class lexer {
private:
    std::string_view src;
    size_t length;
    uint32_t idx = 0;

    uint32_t line = 0;

    token lex();
    void skip();

public:
    lexer(std::string_view input);

    token consume();
    token peek();

private:
    token consume_identifier();
};

static_assert(sizeof(token) == 8);