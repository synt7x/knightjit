#include "lexer.hpp"

#include "../logs/frog.hpp"

#include <limits>

lexer::lexer(std::string_view source) {
    src = source;
    length = source.size();
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
                while (src[idx] != '\n') idx++;
                break;
            default:
                return;
        }
    }
}

token lexer::consume_identifier() {
    uint32_t start = idx;

    while (src[idx] >= 'a' && src[idx] <= 'z' || src[idx] == '_') {
        idx++;
    }

    if (idx - start > std::numeric_limits<uint16_t>::max()) {
        token err = {
            .start = start,
            .length = 1,
            .type = token_type::ERROR,
        };

        frog::croak(
            src, {
                .level = frog::level::error,
                .id = frog::message::identifier_too_long,
                .span = frog::token_to_span(err),
            }
        );

        return {
            .start = start,
            .length = 1,
            .type = token_type::ERROR,
        };
    }

    return {
        .start = start,
        .length = (uint16_t) (idx - start),
    };
}

token lexer::consume() {
    lexer::skip();

    if (idx >= length) return {
        .start = idx,
        .length = 0,
        .type = token_type::NONE,
    };

    if (src[idx] >= 'a' && src[idx] <= 'z' || src[idx] == '_') {
        return lexer::consume_identifier();
    }

    return {
        .start = idx,
        .length = 0,
        .type = token_type::NONE,
    };
}