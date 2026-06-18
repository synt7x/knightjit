#pragma once

#include <cstdint>
#include <string_view>

/**
 * @brief Node type of a token or AST node.
 */
enum class node_type : uint8_t {
  NONE, ERROR, STRING, NUMBER, VARIABLE,
  TRUE, FALSE, NIL, BLOCK, ARRAY,
  PROMPT, RANDOM, CALL, QUIT, DUMP,
  OUTPUT, LENGTH, NOT, NEGATE, ASCII,
  BOX, HEAD, TAIL, ADD, SUBTRACT,
  MULTIPLY, DIVIDE, MOD, POWER, GREATER,
  LESS, COMPARE, AND, OR, EXPR,
  EQUAL, WHILE, IF, GET, SET, ARGS
};

/**
 * @brief A token produced by the lexer.
 */
struct token {
  uint32_t start = 0;
  uint16_t length = 0;
  node_type type = node_type::NONE;
};


/**
 * @brief A pull-based lexer that produces tokens from an input source string.
 * @note Continually call `consume()` until `is_eof()` returns `true`.
 */
class lexer {
private:
  /// @brief The source string being lexed, stored as a non-owning view for efficiency.
  std::string_view src;

  /// @brief Length of the source string, used for bounds checking.
  uint32_t length;

  /// @brief Current index in the source string, used for lexing.
  uint32_t idx = 0;

  /// @brief Current line number, used for error reporting.
  /// @note Zero-indexed, so the first line is line 0.
  uint32_t line = 0;

public:
  /**
   * @brief Instance of the lexer, which produces tokens from an input source string.
   * @note Use as a pull-based lexer, continually calling `consume()` until `is_eof()` returns `true`.
   */
  lexer(std::string_view source);

  /**
   * @brief Consume and return the next token from the input source.
   *
   * @return The next `token` from the source string,
   * `node_type::ERROR` if an unexpected token is encountered,
   * or `node_type::NONE` if the end of the source string is reached.
  */
  [[nodiscard]]
  token consume();

  /**
   * @brief Peek at the next token without consuming it.
   *
   * @return The next `token` from the source string,
   * `node_type::ERROR` if an unexpected token is encountered,
   * or `node_type::NONE` if the end of the source string is reached.
   */
  [[nodiscard]]
  token peek();

  /**
   * @brief Checks if the end of the source string has been reached.
   * 
   * @return `true` if the end of the source string has been reached,
   * @return `false` otherwise.
   */
  const bool is_eof();

private:
  /**
   * @brief Checks the bounds of a token, preventing overflow of the length field.
   * 
   * @param start Index of the start of the token in the source string
   * @param type Type of the token
   * @return `token`
   */
  token bounds(uint32_t start, node_type type);

  /**
   * @brief Skips whitespace and comments in the input
   * source string. Comments start with a '#' character and
   * continue until the end of the line.
   */
  void skip();

  /**
   * @brief Builds an identifier token from the input source string,
   * starting at the current index.
   * 
   * @return `token` of type `node_type::VARIABLE` or `node_type::ARGS`
   * if the identifier is `_`.
   */
  token identifier();

  /**
   * @brief Builds a builtin token from the input source string,
   * starting at the current index. Builtin tokens only depend
   * upon the first character, which must be an uppercase letter.
   * 
   * @return token 
   */
  token builtin();

  /**
   * @brief Builds a number token from the input source string,
   * starting at the current index.
   * 
   * @return `token` of type `node_type::NUMBER`
   */
  token number();

  /**
   * @brief Builds a string token from the input source string,
   * starting at the current index. String literals are enclosed in
   * either single or double quotes.
   * 
   * @return `token` of type `node_type::STRING`
   */
  token string();

  /**
   * @brief Determine the nodetype of the next operator token.
   * 
   * @return The type of the next operator token,
   * or `node_type::NONE` if the next token is not an operator.
   */
  node_type operation();
};

/*
 * Keep token sizes within a 64 bit integer,
 * specifically for efficiency purposes.
 *
 * The `start` field is a 32 bit unsigned integer,
 * allowing for source strings up to 4 GB in size.
 * The `length` field is a 16 bit unsigned integer,
 * allowing for tokens up to 64 KB in size.
 *
 * Generally, having tokens larger than 64 KB is unlikely,
 * and in the case the user wants a long string,
 * they can split it into multiple string literals.
 */
static_assert(sizeof(token) == 8);