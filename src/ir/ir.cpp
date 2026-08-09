#include "ir.hpp"
#include "parser.hpp"
#include "arena.hpp"

#include <string_view>

ir::idx ir::emit_constant(int64_t num) {
    instruction instr {};
    instr.constant = constant(false, num);

    return emit(instr);
}

ir::idx ir::emit_constant(vm::string& str) {
    instruction instr {};
    instr.constant = constant(true, reinterpret_cast<uintptr_t>(&str));
    
    return emit(instr);
}

ir::idx ir::emit_string(frog::span range) {
    std::string_view str = parser.fetch(range);
    vm::bump_id id = strings.allocate(str.size() + 2);
    vm::string& s = *reinterpret_cast<vm::string*>(strings.pointer_at(id));
    s.length = str.size();

    std::memcpy(strings.pointer_at(id) + 2, str.data(), str.size());

    return emit_constant(s);
}

void ir::generate(parser::node& node) {
    switch (node.type) {
        case parser::node_type::STRING:
            emit_string(node.range);
            break;
    } 
}