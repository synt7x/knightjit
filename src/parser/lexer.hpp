#pragma once

#include <cstdint>
#include <string_view>

enum class node_type : uint8_t {
  NONE,
  ERROR,
  STRING,
  NUMBER,
  VARIABLE,
  TRUE,
  FALSE,
  NIL,
  BLOCK,
  ARRAY,
  PROMPT,
  RANDOM,
  CALL,
  QUIT,
  DUMP,
  OUTPUT,
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
  node_type type = node_type::NONE;
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

  token bounds(uint32_t start, node_type type);

  token consume();
  token peek();

  bool is_eof();

private:
  token consume_identifier();
  token consume_builtin();
  token consume_number();
  token consume_string();
  node_type consume_operator();
};

static_assert(sizeof(token) == 8);