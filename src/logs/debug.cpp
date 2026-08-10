#include "debug.hpp"
#include "parser.hpp"
#include "ir.hpp"

#include <cstdint>
#include <string_view>
#include <iostream>

namespace debug {

void inspect(lexer lex) {

}

void inspect(parser* parse) {
    
}

void inspect(ir* ir) {
    for (std::size_t i = 0; i < ir->instructions.size(); i++) {
        const ir::instruction& instr = ir->instructions[i];
        std::cout << "v" << i << " = ";

        if (instr.compact.flag == ir::flags::COMPACT) {
            std::cout << "COMPACT\n";
        } else if (instr.extended.flag == ir::flags::EXTENDED) {
            std::cout << "EXTENDED\n";
        } else if (instr.constant.flag == static_cast<uint64_t>(ir::flags::CONSTANT)) {
            if (instr.constant.is_string) {
                vm::string* str = reinterpret_cast<vm::string*>(instr.constant.value << 3);

                std::cout << "\"" << str->view() << "\"\n";
            } else {
                std::cout << instr.constant.value << "\n";
            }
        }
    }
}

void inspect(parser* parse, parser::node node, const std::string& prefix, bool last) {
    std::cout << prefix;

    if (!prefix.empty()) {
        std::cout << (last ? "*- " : "|- ");
    }

    std::cout << inspect(node.type);

    switch (node.type) {
        case parser::node_type::NUMBER:
            std::cout << " " << parse->lex.src.substr(node.range.start, node.range.length);
            break;
        case parser::node_type::STRING:
            std::cout << " \"" << parse->lex.src.substr(node.range.start, node.range.length) << "\"";
            break;
        case parser::node_type::VARIABLE:
            std::cout << " " << parse->lex.src.substr(node.range.start, node.range.length);
            break;
        default:
            break;
    }

    std::cout << '\n';

    // for (size_t i = 0; i < 4; i++) {
    //     if (node.children[i] == 0) break;
    //     inspect(
    //         parse,
    //         parse->get(node.children[i]),
    //         prefix + (last ? "   " : "|  "),
    //         i == node.children.
    //     );
    // }
}

const std::string_view inspect(parser::node_type type) {
    switch (type) {
        case parser::node_type::VARIABLE: return "VARIABLE";
        case parser::node_type::NUMBER: return "NUMBER";
        case parser::node_type::STRING: return "STRING";

        case parser::node_type::TRUE: return "true";
        case parser::node_type::FALSE: return "false";
        case parser::node_type::NIL: return "null";
        case parser::node_type::ARRAY: return "@";

        case parser::node_type::EXPR: return ";";

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