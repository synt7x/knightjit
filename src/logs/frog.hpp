#pragma once

#include <cstdint>
#include <string_view>

#include "../parser/lexer.hpp"

namespace frog {

constexpr std::string_view levels[] = {
    "note: ", "warning: ", "error: ", "panic: "};

enum class level : uint8_t { note, warning, error, panic };

constexpr std::string_view messages[] = {
    "unable to open file ",
    "unexpected token ",
    "expected expression, found end of file",
    "unknown identifier ",
    "unknown builtin function ",
    "when parsing expression for ",
    "token is too long starting at ",
    "unterminated string"};

enum class message : uint8_t {
  unavailable_file,
  unexpected_token,
  expected_expression,
  unknown_identifier,
  unknown_builtin,
  when_parsing,
  token_too_long,
  unterminated_string
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
  return {.start = tk.start, .length = tk.length};
}

bool croak(level lvl, message id);
void croak(level lvl, message id, std::string_view str);
void croak(std::string_view src, diagnostic dg);
void croak(std::string_view src, diagnostic d1, diagnostic d2);

} // namespace frog