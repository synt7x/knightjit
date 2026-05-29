#pragma once

#include <cstdint>
#include <string_view>

#include "../parser/lexer.hpp"

namespace frog {

constexpr std::string_view levels[] = {
    "note: ",
    "warning: ",
    "error: ",
    "panic: "
};

enum class level : uint8_t {
    note,
    warning,
    error,
    panic
};

constexpr std::string_view messages[] = {
    "unexpected token ",
    "expected expression, found end of file",
    "unknown identifier ",
    "when parsing expression for ",
    "identifier is too long starting at "
};

enum class message : uint8_t {
    unexpected_token,
    expected_expression,
    unknown_identifier,
    when_parsing,
    identifier_too_long,
};

struct span {
    uint32_t start;
    uint16_t length;
};

struct position {
    uint32_t line;
    uint32_t column;
};

struct diagnostic {
    level level;
    message id;
    span span;
};

static inline span token_to_span(token tk) {
    return {
        .start = tk.start,
        .length = tk.length
    };
}

void croak(std::string_view src, diagnostic dg);
void croak(std::string_view src, diagnostic d1, diagnostic d2);

}