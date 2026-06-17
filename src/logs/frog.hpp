#pragma once

#include <cstdint>
#include <string_view>

#include "../parser/lexer.hpp"

namespace frog {

/**
 * @brief Diagnostic levels for error reporting, used to indicate the severity of an issue.
 * Used to generate strings to be output when calling `croak()`.
 */
constexpr std::string_view levels[] = {
    "note: ", "warning: ", "error: ", "panic: "
};

/**
 * @brief Specify the diagnostic level of the emitted message
 */
enum class level : uint8_t {
  /// @note Used to attach a note to an error message
  note = 0,
  /// @note Used for non-critical issues that do not prevent the program from running
  warning = 1,
  /// @note Used for critical issues that prevent the program from running correctly
  error = 2,
  /// @note Used for unrecoverable issues that cause the program to terminate immediately
  panic = 3
};

// Ensure the levels array has the same number of entries as the level enum
static_assert(
  sizeof(levels) / sizeof(levels[0]) == static_cast<size_t>(level::panic) + 1,
  "Diagnostic levels array must match the number of levels in the enum"
);

/**
 * @brief Diagnostic messages for error reporting, used to indicate the specific issue encountered.
 * Used to generate strings to be output when calling `croak()`.
 */
constexpr std::string_view messages[] = {
    "unable to open file ",
    "file/input too large",
    "unexpected token ",
    "expected expression, found end of file",
    "unknown identifier ",
    "unknown builtin function ",
    "when parsing expression for ",
    "token is too long starting at ",
    "unterminated string"
};

/**
 * @brief Specify the diagnostic messages for error reporting.
 */
enum class message : uint8_t {
  unavailable_file = 0, unexpected_token = 1,
  input_too_large = 2, expected_expression = 3,
  unknown_identifier = 4, unknown_builtin = 5,
  when_parsing = 6, token_too_long = 7,
  unterminated_string = 8
};

// Ensure the messages array has the same number of entries as the message enum
static_assert(
  sizeof(messages) / sizeof(messages[0]) == static_cast<size_t>(message::unterminated_string) + 1,
  "Diagnostic messages array must match the number of messages in the enum"
);

/**
 * @brief A frog span (which differs from the span of a token),
 * used to represent a range of characters in the source code for error reporting.
 * @note Conforms to the same size limits as tokens.
 */
struct span {
  uint32_t start;
  uint16_t length;
};

/**
 * @brief Stores the vertical and horizontal position of a 
 * character from the source string.
 * 
 * @note Differs from `span` in that it contains the
 * line number aswell, rather than just the start and
 * length of a range of characters.
 */
struct position {
  uint32_t line;
  uint32_t column;
};

/**
 * @brief A diagnostic message, contains all required information
 * to generate a formatted error message with source code context.
 */
struct diagnostic {
  /// @brief Level of error to be raised
  level level;

  /// @brief The type of message to be shown
  message id;

  /// @brief The span of the relevant source code
  span span;
};

/**
 * @brief Helper method to convert a token to a span for error reporting.
 * @note The span of a `token` is different from the `span` struct used in a
 * `diagnostic`.
 * 
 * @param `token` contained in the source string
 * @return raw `span` that contains the start and length
 */
static inline span token_to_span(token tk) {
  return { tk.start, tk.length };
}

/**
 * @brief Shows a diagnostic message with a simple level and message ID.
 * 
 * @param lvl Level of error to be raised
 * @param id The type of message to be shown
 * @return `true` if the message ends with a space,
 * @return `false` if it does not
 */
bool croak(level lvl, message id);

/**
 * @brief Shows a short diagnostic message, appending
 * the given string if the message ends with a space character.
 * 
 * @param lvl Level of error to be raised
 * @param id The type of message to be shown
 * @param str Embedded string
 */
void croak(level lvl, message id, std::string_view str);

/**
 * @brief Shows a diagnostic message generated
 * from the given `diagnostic`.
 * 
 * @param src Represents the source string
 * @param dg Is the diagnostic message
 */
void croak(std::string_view src, diagnostic dg);

/**
 * @brief Show a diagnostic message with two diagnostic parts,
 * usually used to showa a `note` attached to the first message.
 * 
 * @param src Represents the source string
 * @param d1 Is the top diagnostic message
 * @param d2 Is the bottom diagnostic message
 */
void croak(std::string_view src, diagnostic d1, diagnostic d2);

} // namespace frog