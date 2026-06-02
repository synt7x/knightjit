#include "lexer.hpp"

#include "../logs/frog.hpp"

#include <limits>

lexer::lexer(std::string_view source) : src(source), length(source.size()) {}

token lexer::bounds(uint32_t start, node_type type) {
  if (idx - start > std::numeric_limits<uint16_t>::max()) {
    token err = {
        .start = start,
        .length = 1,
        .type = node_type::ERROR,
    };

    frog::croak(src, {
                         .level = frog::level::error,
                         .id = frog::message::token_too_long,
                         .span = frog::token_to_span(err),
                     });

    return {
        .start = start,
        .length = 1,
        .type = node_type::ERROR,
    };
  }

  return {
      .start = start,
      .length = (uint16_t)(idx - start),
      .type = type,
  };
}

void lexer::skip() {
  while (idx < length) {
    switch (src[idx]) {
    case ' ':
    case ':':
    case '(':
    case ')':
    case '{':
    case '}':
    case '\r':
    case '\t':
      idx++;
      break;
    case '\n':
      line++;
      idx++;
      break;
    case '#':
      while (src[idx] != '\n')
        idx++;
      break;
    default:
      return;
    }
  }
}

token lexer::consume_identifier() {
  uint32_t start = idx;

  while (!lexer::is_eof() && (src[idx] >= 'a' && src[idx] <= 'z') ||
         (src[idx] >= '0' && src[idx] <= '9') || src[idx] == '_')
    idx++;

  return lexer::bounds(start, src[start] == '_' && (idx == start + 1)
                                  ? node_type::ARGS
                                  : node_type::VARIABLE);
}

token lexer::consume_builtin() {
  uint32_t start = idx;

  node_type type;
  switch (src[idx]) {
  case 'A':
    type = node_type::ASCII;
    break;
  case 'B':
    type = node_type::BLOCK;
    break;
  case 'C':
    type = node_type::CALL;
    break;
  case 'D':
    type = node_type::DUMP;
    break;
  case 'F':
    type = node_type::FALSE;
    break;
  case 'G':
    type = node_type::GET;
    break;
  case 'I':
    type = node_type::IF;
    break;
  case 'L':
    type = node_type::LENGTH;
    break;
  case 'N':
    type = node_type::NIL;
    break;
  case 'O':
    type = node_type::OUTPUT;
    break;
  case 'P':
    type = node_type::PROMPT;
    break;
  case 'Q':
    type = node_type::QUIT;
    break;
  case 'R':
    type = node_type::RANDOM;
    break;
  case 'S':
    type = node_type::SET;
    break;
  case 'T':
    type = node_type::TRUE;
    break;
  case 'W':
    type = node_type::WHILE;
    break;
  default:
    type = node_type::ERROR;
    break;
  }

  while (!lexer::is_eof() && (src[idx] >= 'A' && src[idx] <= 'Z') ||
         src[idx] == '_')
    idx++;

  token result = lexer::bounds(start, type);

  if (type == node_type::ERROR) {
    frog::croak(src, {
                         .level = frog::level::error,
                         .id = frog::message::unknown_builtin,
                         .span = frog::token_to_span(result),
                     });
  }

  return result;
}

token lexer::consume_number() {
  uint32_t start = idx;
  while (!lexer::is_eof() && src[idx] >= '0' && src[idx] <= '9')
    idx++;

  return lexer::bounds(start, node_type::NUMBER);
}

token lexer::consume_string() {
  char delim = src[idx];
  uint32_t start = ++idx;

  while (!lexer::is_eof() && src[idx] != delim)
    idx++;
  token result = lexer::bounds(start, node_type::STRING);

  if (src[idx] != delim) {
    frog::croak(src, {
                         .level = frog::level::warning,
                         .id = frog::message::unterminated_string,
                         .span = frog::token_to_span(result),
                     });
  }

  idx++;

  return result;
}

node_type lexer::consume_operator() {
  switch (src[idx]) {
  case '@':
    return node_type::ARRAY;
  case '!':
    return node_type::NOT;
  case '~':
    return node_type::NEGATE;
  case ',':
    return node_type::NOT;
  case '[':
    return node_type::HEAD;
  case ']':
    return node_type::TAIL;
  case '+':
    return node_type::ADD;
  case '-':
    return node_type::SUBTRACT;
  case '*':
    return node_type::MULTIPLY;
  case '/':
    return node_type::DIVIDE;
  case '%':
    return node_type::MOD;
  case '^':
    return node_type::POWER;
  case '>':
    return node_type::GREATER;
  case '<':
    return node_type::LESS;
  case '?':
    return node_type::COMPARE;
  case '&':
    return node_type::AND;
  case '|':
    return node_type::OR;
  case ';':
    return node_type::EXPR;
  case '=':
    return node_type::EQUAL;
  default:
    return node_type::NONE;
  }
}

token lexer::consume() {
  lexer::skip();

  if (idx >= length)
    return {
        .start = idx,
        .length = 0,
        .type = node_type::NONE,
    };

  if (src[idx] >= 'a' && src[idx] <= 'z' || src[idx] == '_') {
    return lexer::consume_identifier();
  }

  if (src[idx] >= 'A' && src[idx] <= 'Z') {
    return lexer::consume_builtin();
  }

  if (src[idx] >= '0' && src[idx] <= '9') {
    return lexer::consume_number();
  }

  if (src[idx] == '\'' || src[idx] == '"') {
    return lexer::consume_string();
  }

  node_type op = lexer::consume_operator();
  if (op != node_type::NONE)
    return {
        .start = idx++,
        .length = 1,
        .type = op,
    };

  if (idx >= length)
    return {
        .start = idx,
        .length = 0,
        .type = node_type::NONE,
    };

  token err = {
      .start = idx,
      .length = 1,
      .type = node_type::ERROR,
  };

  frog::croak(src, {
                       .level = frog::level::error,
                       .id = frog::message::unexpected_token,
                       .span = frog::token_to_span(err),
                   });

  idx++;

  return err;
}

bool lexer::is_eof() { return idx >= length; }