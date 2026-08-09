#include "ir.hpp"
#include "parser.hpp"
#include "arena.hpp"

#include <string_view>

ir::idx ir::emit_constant(int64_t num) {
    instruction instr {};
    instr.constant = constant(false, num);

    return emit(instr);
}

void ir::generate(parser::node& node) {
    switch (node.type) {
        case parser::node_type::STRING:
            std::string_view str = parser.fetch(node.range);
            strings.push_back(str);
            break;
    } 
}