#include "ir.hpp"
#include "parser.hpp"
#include "arena.hpp"

#include <string_view>
#include <charconv>
#include <iostream>

ir::idx ir::emit_constant(int64_t num) {
    instruction instr {};
    instr.constant = constant(false, num);

    return emit(instr);
}

ir::idx ir::emit_constant(vm::string& str) {
    instruction instr {};
    instr.constant = constant(true, reinterpret_cast<uintptr_t>(&str) >> 3);
    
    return emit(instr);
}

ir::idx ir::emit_compact(ir::opcode op, idx v1, idx v2, idx v3) {
    instruction instr {};
    instr.compact = compact(op);
    instr.compact.anchor = length() - v1;
    instr.compact.v2 = (length() - v1) + v2;
    instr.compact.v3 = (length() - v1) +v3;

    return emit(instr);
}

ir::idx ir::emit_extended(ir::opcode op, idx v1, idx v2, idx v3) {
    instruction instr {};
    idx anchor = blocks.back().extended.size();

    blocks.back().extended.push_back(v1);
    blocks.back().extended.push_back(v2);
    blocks.back().extended.push_back(v3);

    instr.extended = extended(op, anchor);

    return emit(instr);
}

ir::idx ir::emit_instruction(ir::opcode op) {
    instruction instr {};

    return emit_compact(op, 0, 0, 0);
}

ir::idx ir::emit_instruction(ir::opcode op, idx v1) {
    instruction instr {};

    if (length() - v1 > (1 << 24) - 1) {
        return emit_extended(op, v1, 0, 0);
    } else {
        return emit_compact(op, v1, 0, 0);
    }
}

ir::idx ir::emit_instruction(ir::opcode op, idx v1, idx v2) {
    instruction instr {};

    if (length() - v1 > (1 << 24) - 1 || (length() - v1) + v2 > (1 << 16) - 1) {
        return emit_extended(op, v1, v2, 0);
    } else {
        return emit_compact(op, v1, v2, 0);
    }
}

ir::idx ir::emit_instruction(ir::opcode op, idx v1, idx v2, idx v3) {
    instruction instr {};

    if (length() - v1 > (1 << 24) - 1 || (length() - v1) + v2 > (1 << 16) - 1 || (length() - v1) + v3 > (1 << 16) - 1) {
        return emit_extended(op, v1, v2, v3);
    } else {
        return emit_compact(op, v1, v2, v3);
    }
}

ir::idx ir::emit_string(frog::span range) {
    std::string_view str = parser.fetch(range);
    vm::bump_id id = strings.allocate(str.size() + 2);

    vm::string& s = *reinterpret_cast<vm::string*>(strings.pointer_at(id));
    s.length = str.size();

    for (std::size_t i = 0; i < str.size(); i++) {
        s.content[i] = static_cast<std::byte>(str[i]);
    }
    
    return emit_constant(s);
}

ir::idx ir::emit_number(frog::span range) {
    std::string_view str = parser.fetch(range);
    
    int64_t num = 0;
    auto [ptr, err] = std::from_chars(str.data(), str.data() + str.size(), num);

    if (err != std::errc()) {
        frog::croak(parser.lex.src, frog::diagnostic {
            frog::level::error,
            frog::message::invalid_number,
            range
        });
    }

    return emit_constant(num);
}

ir::idx ir::generate(parser::node& node) {
    switch (node.type) {
        case parser::node_type::STRING:
            return emit_string(node.range);
        case parser::node_type::NUMBER:
            return emit_number(node.range);
        case parser::node_type::EXPR:
            generate(parser.nodes.at(node.children[0]));
            return generate(parser.nodes.at(node.children[1]));
    } 
}