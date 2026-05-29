#include "frog.hpp"

#include <iostream>
#include <string>
#include <format>

namespace frog {

std::string_view get_line(std::string_view src, diagnostic dg) {
    size_t pre = 0;
    size_t post = src.size();

    for (size_t i = 0; i < src.size(); i++) {
        if (i >= dg.span.start && src[i] == '\n') {
            post = i - 1;
            break;
        } else if (src[i] == '\n') {
            pre = i + 1;
        }
    }

    return src.substr(pre, post - pre);
}

position get_position(std::string_view src, diagnostic dg) {
    size_t pre = 0;

    uint32_t column = dg.span.start;
    uint32_t line = 0;

    for (size_t i = 0; i < src.size(); i++) {
        if (src[i] == '\n') {
            pre = i + 1;
            line++;
        }

        if (i == column) {
            column -= pre;
            break;
        }
    }

    return {
        .line = line,
        .column = column,
    };
} 

std::string line_gutter(uint32_t line) {
    return std::to_string(line) + " | ";
}

std::string empty_gutter(std::string_view gutter) {
    std::string empty(gutter);

    for (char& c : empty) {
        if (c != '|') c = ' ';
    }

    return empty;
}

std::string caret(std::string_view line, uint32_t column, uint32_t length) {
    std::string caret(column + length, ' ');

    for (uint32_t i = 0; i < column + length; i++) {
        if (line[i] == '\t') caret[i] = '\t';
        if (i >= column) caret[i] = '^';
    }

    return caret;
}

void format(std::string_view src, diagnostic dg) {
    uint32_t start = dg.span.start;
    uint32_t length = dg.span.length;

    std::string_view code = get_line(src, dg);
    position p = get_position(src, dg);

    uint32_t line = p.line;
    uint32_t column = p.column;

    std::string gutter = line_gutter(line);
    std::string empty = empty_gutter(gutter);

    std::string arrow = caret(code, column, length);
    std::string_view message = messages[static_cast<uint8_t>(dg.id)];
    std::string_view level = levels[static_cast<uint8_t>(dg.level)];

    std::cout << level << message;

    if (message[message.size() - 1] == ' ') {
        std::cout << '\'' << src.substr(start, length) << '\'';
    }

    std::cout << std::endl;

    std::cout << empty << std::endl;
    std::cout << gutter << code << std::endl;
    std::cout << empty << arrow << " - here" << std::endl;
}

void croak(std::string_view src, diagnostic dg) {
    format(src, dg);
}

void croak(std::string_view src, diagnostic d1, diagnostic d2) {
    croak(src, d1);
    format(src, d2);
}

}