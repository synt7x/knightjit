#include "ir.hpp"
#include "parser.hpp"
#include "arena.hpp"

ir::idx ir::emit_constant(vm::string& str) {
    instruction instr {};
    instr.constant = constant(true, reinterpret_cast<uintptr_t>(&str));

    return emit(instr);
}

ir::idx ir::emit_constant(int64_t num) {
    instruction instr {};
    instr.constant = constant(false, num);

    return emit(instr);
}

void ir::generate(parser::node& node) {
    switch (node.type) {
        case parser::node_type::STRING:
            break;
    } 
}