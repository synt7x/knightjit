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

    int32_t anchor_offset = static_cast<int32_t>(length() - v1);
    instr.compact.anchor = static_cast<uint32_t>(anchor_offset);

    int32_t d2 = static_cast<int32_t>(v2) - static_cast<int32_t>(v1);
    int32_t d3 = static_cast<int32_t>(v3) - static_cast<int32_t>(v1);

    instr.compact.v2 = static_cast<uint16_t>(static_cast<int16_t>(d2));
    instr.compact.v3 = static_cast<uint16_t>(static_cast<int16_t>(d3));

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
    return emit_compact(op, 0, 0, 0);
}

ir::idx ir::emit_instruction(ir::opcode op, idx v1) {
    int64_t anchor = static_cast<int64_t>(length()) - static_cast<int64_t>(v1);
    bool fits = (anchor >= 0) && (anchor <= 0xFFFFFF);

    if (!fits) {
        return emit_extended(op, v1, 0, 0);
    } else {
        return emit_compact(op, v1, 0, 0);
    }
}

ir::idx ir::emit_instruction(ir::opcode op, idx v1, idx v2) {
    int64_t anchor = static_cast<int64_t>(length()) - static_cast<int64_t>(v1);
    int64_t d2 = static_cast<int64_t>(v2) - static_cast<int64_t>(v1);

    bool fits = ((anchor >= 0) && (anchor <= 0xFFFFFF))
        || ((d2 >= -32768) && (d2 <= 32767));

    if (!fits) {
        return emit_extended(op, v1, v2, 0);
    } else {
        return emit_compact(op, v1, v2, 0);
    }
}

ir::idx ir::emit_instruction(ir::opcode op, idx v1, idx v2, idx v3) {
    int64_t anchor = static_cast<int64_t>(length()) - static_cast<int64_t>(v1);

    int64_t d2 = static_cast<int64_t>(v2) - static_cast<int64_t>(v1);
    int64_t d3 = static_cast<int64_t>(v3) - static_cast<int64_t>(v1);

    bool fits =( (anchor >= 0) && (anchor <= 0xFFFFFF))
        || ((d2 >= -32768) && (d2 <= 32767))
        || ((d3 >= -32768) && (d3 <= 32767));

    if (!fits) {
        return emit_extended(op, v1, v2, v3);
    } else {
        return emit_compact(op, v1, v2, v3);
    }
}

ir::idx ir::patch(idx index, opcode op, idx v1) {
    int64_t anchor = static_cast<int64_t>(length()) - static_cast<int64_t>(v1);
    bool fits = (anchor >= 0) && (anchor <= 0xFFFFFF);
    instruction& instr = instructions[index];

    if (instr.compact.flag == flags::EXTENDED) {
        frog::croak(parser.lex.src, frog::diagnostic {
            frog::level::panic,
            frog::message::bug_extended,
            frog::span { 0, 0 }
        });
    }

    if (!fits) {
        instr.extended = extended(op, v1);
    } else {
        instr.compact = compact(op);
        instr.compact.anchor = static_cast<uint32_t>(anchor);
    }
}

ir::idx ir::patch(idx index, opcode op, idx v1, idx v2, idx v3) {
    instruction& instr = instructions[index];
    int64_t anchor = static_cast<int64_t>(length()) - static_cast<int64_t>(v1);

    int64_t d2 = static_cast<int64_t>(v2) - static_cast<int64_t>(v1);
    int64_t d3 = static_cast<int64_t>(v3) - static_cast<int64_t>(v1);

    bool fits =( (anchor >= 0) && (anchor <= 0xFFFFFF))
        || ((d2 >= -32768) && (d2 <= 32767))
        || ((d3 >= -32768) && (d3 <= 32767));

    if (!fits) {
        
    } else {
        
    }
}

ir::idx ir::emit_string(frog::span range) {
    std::string_view str = parser.fetch(range);
    vm::bump_id id = strings.allocate(str.size() + 8);

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

ir::idx ir::emit_block(idx block) {
    return emit_instruction(opcode::BLOCK, block);
}

ir::idx ir::generate_block(parser::node& node) {
    idx patch = emit_instruction(opcode::JMP, 0);

    return patch;
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
        case parser::node_type::BLOCK: {
            idx block = generate_block(parser.nodes.at(node.children[0]));
            patch(block, opcode::JMP, length());
            return emit_block(block + 1);
        }
        case parser::node_type::ASCII: {
            idx child = generate(parser.nodes.at(node.children[0]));
            return emit_instruction(opcode::ASCII, child);
        }
        case parser::node_type::QUIT: {
            idx child = generate(parser.nodes.at(node.children[0]));
            return emit_instruction(opcode::QUIT, child);
        }
        case parser::node_type::NOT: {
            idx child = generate(parser.nodes.at(node.children[0]));
            return emit_instruction(opcode::NOT, child);
        }
        case parser::node_type::NEGATE: {
            idx child = generate(parser.nodes.at(node.children[0]));
            return emit_instruction(opcode::NEGATE, child);
        }
        case parser::node_type::ADD: {
            idx left = generate(parser.nodes.at(node.children[0]));
            idx right = generate(parser.nodes.at(node.children[1]));

            idx coercion = emit_instruction(opcode::COERCE, right, left);
            
            return emit_instruction(opcode::ADD, left, coercion);
        }
        case parser::node_type::SUBTRACT: {
            idx left = generate(parser.nodes.at(node.children[0]));
            idx right = generate(parser.nodes.at(node.children[1]));

            idx coercion = emit_instruction(opcode::COERCE, right, left);

            return emit_instruction(opcode::SUB, left, coercion);
        }
        case parser::node_type::MULTIPLY: {
            idx left = generate(parser.nodes.at(node.children[0]));
            idx right = generate(parser.nodes.at(node.children[1]));

            idx coercion = emit_instruction(opcode::COERCE, right, left);

            return emit_instruction(opcode::MUL, left, coercion);
        }
        case parser::node_type::DIVIDE: {
            idx left = generate(parser.nodes.at(node.children[0]));
            idx right = generate(parser.nodes.at(node.children[1]));

            idx coercion = emit_instruction(opcode::COERCE, right, left);

            return emit_instruction(opcode::DIV, left, coercion);
        }
        case parser::node_type::MOD: {
            idx left = generate(parser.nodes.at(node.children[0]));
            idx right = generate(parser.nodes.at(node.children[1]));

            idx coercion = emit_instruction(opcode::COERCE, right, left);

            return emit_instruction(opcode::MOD, left, coercion);
        }
        case parser::node_type::POWER: {
            idx left = generate(parser.nodes.at(node.children[0]));
            idx right = generate(parser.nodes.at(node.children[1]));

            idx coercion = emit_instruction(opcode::COERCE, right, left);

            return emit_instruction(opcode::POW, left, coercion);
        }
        default:
            frog::croak(parser.lex.src, frog::diagnostic {
                frog::level::error,
                frog::message::bug_unimplemented,
                node.range
            });
            break;
    } 
}