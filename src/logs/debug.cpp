#include "debug.hpp"
#include "parser.hpp"

#include <cstdint>
#include <string_view>
#include <iostream>

namespace debug {

void inspect(lexer lex) {

}

void inspect(parser* parse) {

}

void inspect(parser::node node, parser* parse, uint32_t depth) {
    std::cout << std::string(depth * 2, ' ') << inspect(node.type);
    if (node.type == parser::node_type::NUMBER) {
        std::cout << parse->lex.src.substr(node.range.start, node.range.length);
    } else if (node.type == parser::node_type::STRING) {
        std::cout << "\"" << parse->lex.src.substr(node.range.start, node.range.length) << "\"";
    } else if (node.type == parser::node_type::VARIABLE) {
        std::cout << parse->lex.src.substr(node.range.start, node.range.length);
    }

    if (node.type != parser::node_type::EXPR) {
        std::cout << std::endl;
    } else {
        std::cout << " ";
    }

    for (const auto &id : node.children) {
        parser::node child = parse->get(id);
        inspect(child, parse, node.type == parser::node_type::EXPR ? depth : depth + 1);
    }
}

const std::string_view inspect(parser::node_type type) {
    switch (type) {
        case parser::node_type::VARIABLE: return "";
        case parser::node_type::NUMBER: return "";
        case parser::node_type::STRING: return "";

        case parser::node_type::TRUE: return "true";
        case parser::node_type::FALSE: return "false";
        case parser::node_type::NIL: return "null";
        case parser::node_type::ARRAY: return "@";


        case parser::node_type::ADD: return "+";
        case parser::node_type::AND: return "&";
        case parser::node_type::ARGS: return "ARGS";
        case parser::node_type::ASCII: return "ASCII";
        case parser::node_type::BLOCK: return "BLOCK";
        case parser::node_type::BOX: return "BOX";
        case parser::node_type::CALL: return "CALL";
        case parser::node_type::COMPARE: return "==";
        case parser::node_type::DIVIDE: return "/";
        case parser::node_type::DUMP: return "DUMP";
        case parser::node_type::EQUAL: return "=";
        case parser::node_type::ERROR: return "ERROR";
        case parser::node_type::EXPR: return ";";
        case parser::node_type::GET: return "GET";
        case parser::node_type::GREATER: return ">";
        case parser::node_type::HEAD: return "HEAD";
        case parser::node_type::IF: return "IF";
        case parser::node_type::LENGTH: return "LENGTH";
        case parser::node_type::LESS: return "<";
        case parser::node_type::MOD: return "%";
        case parser::node_type::MULTIPLY: return "*";
        case parser::node_type::NEGATE: return "-";
        case parser::node_type::NOT: return "!";
        case parser::node_type::OR: return "|";
        case parser::node_type::OUTPUT: return "OUTPUT";
        case parser::node_type::POWER: return "^";
        case parser::node_type::PROMPT: return "PROMPT";
        case parser::node_type::QUIT: return "QUIT";
        case parser::node_type::RANDOM: return "RANDOM";
        case parser::node_type::SET: return "SET";
        case parser::node_type::SUBTRACT: return "-";
        case parser::node_type::TAIL: return "TAIL";
        case parser::node_type::WHILE: return "WHILE";
        default: return "UNKNOWN";
    }
}

}