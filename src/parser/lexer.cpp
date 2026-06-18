#include "lexer.hpp"

#include "../logs/frog.hpp"

#include <limits>

lexer::lexer(std::string_view source) : src(source) {
  /*
   * Prevent overflow of the length field,
   * which is an unsigned 32 bit integer.
   * 
   * This allows tokens to fit within
   * 8 bytes, which is more efficient for
   * storage and copying purposes.
   * 
   * For a more in-depth explanation,
   * refer to the comment on the `token`
   * struct in `lexer.hpp`.
   */
  if (source.size() > std::numeric_limits<uint32_t>::max()) {
    frog::croak(src, frog::diagnostic {
      frog::level::panic,
      frog::message::input_too_large
    });
  }

  length = static_cast<uint32_t>(source.size());
}

token lexer::bounds(uint32_t start, node_type type) {
  if (idx - start > std::numeric_limits<uint16_t>::max()) {
    token err {
        start, 1, node_type::ERROR,
    };

    frog::croak(src, frog::diagnostic {
      frog::level::error,
      frog::message::token_too_long,
      frog::token_to_span(err),
    });

    return err;
  }

  return {
      start,
      (uint16_t) (idx - start),
      type,
  };
}

void lexer::skip() {
  while (idx < length) {
    switch (src[idx]) {
      // Skip whitespace characters
      case ' ': case ':': case '(': case ')':
      case '{': case '}': case '\r': case '\t':
        idx++; break;
      case '\n': line++; idx++; break;
      // Skip comments
      case '#': while (src[idx] != '\n') idx++; break;
      default: return;
    }
  }
}

token lexer::identifier() {
  uint32_t start = idx;

  // Consume all lowercase letters, digits, and underscores
  while (!lexer::is_eof() && (src[idx] >= 'a' && src[idx] <= 'z') ||
         (src[idx] >= '0' && src[idx] <= '9') || src[idx] == '_')
    idx++;

  /*
   * If the identifier is just `_`,
   * lex it as an `ARGS` token. Otherwise,
   * it is an identifier.
   */
  return lexer::bounds(
    start,
    src[start] == '_' && (idx == start + 1)
    ? node_type::ARGS : node_type::VARIABLE
  );
}

token lexer::builtin() {
  uint32_t start = idx;

  /* Builtin tokens are all uppercase, and
   * are only determined using the first character.
   *
   * Builtin calls can be any string containing
   * uppercase letters and underscores, but the
   * lexer only distinguishes them based on the
   * first character.
   * 
   * e.g. `N` is the same as `NULL` is the same
   * as `NUMBER`, and all are lexed as
   * `node_type::NIL`.
   */
  node_type type;
  switch (src[idx]) {
    case 'A': type = node_type::ASCII; break;
    case 'B': type = node_type::BLOCK; break;
    case 'C': type = node_type::CALL; break;
    case 'D': type = node_type::DUMP; break;
    case 'F': type = node_type::FALSE; break;
    case 'G': type = node_type::GET; break;
    case 'I': type = node_type::IF; break;
    case 'L': type = node_type::LENGTH; break;
    case 'N': type = node_type::NIL; break;
    case 'O': type = node_type::OUTPUT; break;
    case 'P': type = node_type::PROMPT; break;
    case 'Q': type = node_type::QUIT; break;
    case 'R': type = node_type::RANDOM; break;
    case 'S': type = node_type::SET; break;
    case 'T': type = node_type::TRUE; break;
    case 'W': type = node_type::WHILE; break;
    default: type = node_type::ERROR; break;
  }

  while (!lexer::is_eof() && ((src[idx] >= 'A' && src[idx] <= 'Z') ||
         src[idx] == '_'))
    idx++;

  token result = lexer::bounds(start, type);

  if (type == node_type::ERROR) {
    frog::croak(src, frog::diagnostic {
      frog::level::error,
      frog::message::unknown_builtin,
      frog::token_to_span(result),
    });
  }

  return result;
}

token lexer::number() {
  uint32_t start = idx;
  while (!lexer::is_eof() && src[idx] >= '0' && src[idx] <= '9')
    idx++;

  return lexer::bounds(start, node_type::NUMBER);
}

token lexer::string() {
  char delim = src[idx];
  uint32_t start = ++idx;

  /*
   * Consume all characters until reaching the
   * closing quote character. Knight does not
   * support escape sequences, so any matching
   * quote character will close the string.
   */
  while (!lexer::is_eof() && src[idx] != delim) idx++;
  token result = lexer::bounds(start, node_type::STRING);

  if (src[idx] != delim) {
    frog::croak(src, frog::diagnostic {
      frog::level::warning,
      frog::message::unterminated_string,
      frog::token_to_span(result),
    });
  }

  /*
   * Skip the remaining quote character by
   * incrementing the index. It does not matter
   * if the quote was missing, as bounds are checked
   * at the beginning of every `consume()` call.
   */
  idx++;

  return result;
}

node_type lexer::operation() {
  switch (src[idx]) {
    case '@': return node_type::ARRAY;
    case '!': return node_type::NOT;
    case '~': return node_type::NEGATE;
    case ',': return node_type::NOT;
    case '[': return node_type::HEAD;
    case ']': return node_type::TAIL;
    case '+': return node_type::ADD;
    case '-': return node_type::SUBTRACT;
    case '*': return node_type::MULTIPLY;
    case '/': return node_type::DIVIDE;
    case '%': return node_type::MOD;
    case '^': return node_type::POWER;
    case '>': return node_type::GREATER;
    case '<': return node_type::LESS;
    case '?': return node_type::COMPARE;
    case '&': return node_type::AND;
    case '|': return node_type::OR;
    case ';': return node_type::EXPR;
    case '=': return node_type::EQUAL;
    default: return node_type::NONE;
  }
}

token lexer::consume() {
  lexer::skip();

  // Check initial bounds
  if (lexer::is_eof()) return token { idx, 0, node_type::NONE };

  if (src[idx] >= 'a' && src[idx] <= 'z' || src[idx] == '_')
    return lexer::identifier();

  if (src[idx] >= 'A' && src[idx] <= 'Z') return lexer::builtin();
  if (src[idx] >= '0' && src[idx] <= '9') return lexer::number();
  if (src[idx] == '\'' || src[idx] == '"') return lexer::string();

  /*
   * Match an operator as a single character.
   * See the implementation of `operation()`,
   * which guarantees that the operator is a
   * single character.
   */ 
  node_type op = lexer::operation();
  if (op != node_type::NONE)
    return token { idx++, 1, op };

  // Check bounds one last time
  if (lexer::is_eof()) return token { idx, 0, node_type::NONE };

  // Unknown token found, raise an error
  token err {
      idx, 1, node_type::ERROR,
  };

  frog::croak(src, frog::diagnostic {
    frog::level::error,
    frog::message::unexpected_token,
    frog::token_to_span(err),
  });

  // Advance past bad character
  idx++;

  return err;
}

token lexer::peek() {
  // Store the current state
  uint32_t jdx = idx;

  // Read token
  token t = lexer::consume();

  // Restore lexer state
  idx = jdx;
  return t;
}

const bool lexer::is_eof() { return idx >= length; }