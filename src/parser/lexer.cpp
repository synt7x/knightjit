#include "lexer.hpp"

#include "../logs/frog.hpp"

#include <cstdint>
#include <string_view>
#include <limits>

#include <iostream>

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

lexer::token lexer::bounds(uint32_t start, lexer::token_type type) const {
  /*
   * The length of the token must fit
   * within the 16 bit unsigned integer
   * `length` field of the `token` struct.
   * 
   * See the comment on the `token`
   * struct in `lexer.hpp` for a more
   * in-depth explanation of the size
   * constraints.
   */
  if (idx - start > std::numeric_limits<uint16_t>::max()) {
    lexer::token err {
        start, 1, lexer::token_type::ERROR,
    };

    frog::croak(src, frog::diagnostic {
      frog::level::error,
      frog::message::token_too_long,
      frog::token_to_span(err),
    });

    return err;
  }

  // Create a new token with the given length and type
  return {
      start, static_cast<uint16_t>(idx - start), type,
  };
}

void lexer::skip() {
  while (idx < length) {
    switch (src[idx]) {
      /*
       * Skip whitespace characters, note
       * that `:(){}` characters are also
       * considered whitespace in Knight.
       */
      case ' ': case ':': case '(': case ')':
      case '{': case '}': case '\r': case '\t':
        idx++; break;
      // Skip newlines, whilst counting
      case '\n': line++; idx++; break;
      // Skip comments
      case '#': while (!lexer::is_eof() && src[idx] != '\n') idx++; break;
      default: return;
    }
  }
}

lexer::token lexer::identifier() {
  uint32_t start = idx;

  // Consume all lowercase letters, digits, and underscores
  while (!lexer::is_eof() && ((src[idx] >= 'a' && src[idx] <= 'z') ||
         (src[idx] >= '0' && src[idx] <= '9') || src[idx] == '_'))
    idx++;

  /*
   * If the identifier is just `_`,
   * lex it as an `ARGS` token. Otherwise,
   * it is an identifier.
   */
  return lexer::bounds(
    start,
    src[start] == '_' && (idx == start + 1)
    ? lexer::token_type::ARGS : lexer::token_type::VARIABLE
  );
}

lexer::token lexer::builtin() {
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
   * `token_type::NIL`.
   */
  lexer::token_type type;
  switch (src[idx]) {
    case 'A': type = lexer::token_type::ASCII; break;
    case 'B': type = lexer::token_type::BLOCK; break;
    case 'C': type = lexer::token_type::CALL; break;
    case 'D': type = lexer::token_type::DUMP; break;
    case 'F': type = lexer::token_type::FALSE; break;
    case 'G': type = lexer::token_type::GET; break;
    case 'I': type = lexer::token_type::IF; break;
    case 'L': type = lexer::token_type::LENGTH; break;
    case 'N': type = lexer::token_type::NIL; break;
    case 'O': type = lexer::token_type::OUTPUT; break;
    case 'P': type = lexer::token_type::PROMPT; break;
    case 'Q': type = lexer::token_type::QUIT; break;
    case 'R': type = lexer::token_type::RANDOM; break;
    case 'S': type = lexer::token_type::SET; break;
    case 'T': type = lexer::token_type::TRUE; break;
    case 'W': type = lexer::token_type::WHILE; break;
    default: type = lexer::token_type::ERROR; break;
  }

  while (!lexer::is_eof() && ((src[idx] >= 'A' && src[idx] <= 'Z') ||
         src[idx] == '_'))
    idx++;

  lexer::token result = lexer::bounds(start, type);

  if (type == lexer::token_type::ERROR) {
    frog::croak(src, frog::diagnostic {
      frog::level::error,
      frog::message::unknown_builtin,
      frog::token_to_span(result),
    });
  }

  return result;
}

lexer::token lexer::number() {
  uint32_t start = idx;
  while (!lexer::is_eof() && src[idx] >= '0' && src[idx] <= '9')
    idx++;

  return lexer::bounds(start, lexer::token_type::NUMBER);
}

lexer::token lexer::string() {
  char delim = src[idx];
  uint32_t start = ++idx;

  /*
   * Consume all characters until reaching the
   * closing quote character. Knight does not
   * support escape sequences, so any matching
   * quote character will close the string.
   */
  while (!lexer::is_eof() && src[idx] != delim) idx++;
  lexer::token result = lexer::bounds(start, lexer::token_type::STRING);

  if (lexer::is_eof() || src[idx] != delim) {
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
   * 
   * But we must ensure that we have not reached the
   * end of file to prevent idx from overflowing.
   */
  if (!lexer::is_eof()) idx++;

  return result;
}

lexer::token_type lexer::operation() const {
  switch (src[idx]) {
    case '@': return lexer::token_type::ARRAY;
    case '!': return lexer::token_type::NOT;
    case '~': return lexer::token_type::NEGATE;
    case ',': return lexer::token_type::BOX;
    case '[': return lexer::token_type::HEAD;
    case ']': return lexer::token_type::TAIL;
    case '+': return lexer::token_type::ADD;
    case '-': return lexer::token_type::SUBTRACT;
    case '*': return lexer::token_type::MULTIPLY;
    case '/': return lexer::token_type::DIVIDE;
    case '%': return lexer::token_type::MOD;
    case '^': return lexer::token_type::POWER;
    case '>': return lexer::token_type::GREATER;
    case '<': return lexer::token_type::LESS;
    case '?': return lexer::token_type::COMPARE;
    case '&': return lexer::token_type::AND;
    case '|': return lexer::token_type::OR;
    case ';': return lexer::token_type::EXPR;
    case '=': return lexer::token_type::EQUAL;
    default: return lexer::token_type::NONE;
  }
}

lexer::token lexer::consume() {
  lexer::skip();

  // Check initial bounds
  if (lexer::is_eof()) return lexer::token { idx, 0, lexer::token_type::NONE };

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
  lexer::token_type op = lexer::operation();
  if (op != lexer::token_type::NONE)
    return token { idx++, 1, op };

  // Check bounds one last time
  if (lexer::is_eof()) return lexer::token { idx, 0, lexer::token_type::NONE };

  // Unknown token found, raise an error
  lexer::token err {
      idx, 1, lexer::token_type::ERROR,
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

lexer::token lexer::peek() {
  // Store the current state
  uint32_t prev_idx = idx;
  uint32_t prev_line = line;

  // Read token
  lexer::token t = lexer::consume();

  // Restore lexer state
  idx = prev_idx;
  line = prev_line;
  return t;
}

bool lexer::is_eof() const { return idx >= length; }
