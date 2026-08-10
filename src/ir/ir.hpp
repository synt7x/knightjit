#pragma once

#include <cstdint>
#include <vector>

#include "parser.hpp"
#include "arena.hpp"
#include "bump.hpp"
#include "string.hpp"
#include "array.hpp"

/**
 * @brief SSA based IR that is generated
 * from an AST, which can be optimized.
 */
class ir {
public:
  /**
   * @brief Type alias representing the maximum
   * index into various IR related arrays.
   * 
   * Indexes for all arrays must fit into at
   * most this value.
   */
  using idx = uint32_t;

  /**
   * @brief Instance of the SSA IR, which builds an
   * an array of instructions using the provided AST.
   * 
   * @param ast A `parser::ast` instance 
   * @param nodes A `vm::arena<parser::node>` instance
   */
  ir(parser::ast& ast, parser& parse) : strings(), arrays(), parser(parse), instructions() {
    generate(ast);
  }

  
  /**
   * @brief A block of SSA instructions.
   * 
   * Represented as a span over the continuous
   * array of instructions.
   * 
   */
  struct block {
    idx start;
    idx length;

    std::vector<idx> extended;
  };

  std::vector<block> blocks = { 
    { 0, 0, {} } // The root block, length is updated once it is finished.
  };


  /**
   * @brief Generates the SSA instructions from the
   * provided AST node.
   */
  idx generate(parser::node& node);

  /**
   * @brief A pool of strings allocated for the IR,
   * will be cloned into the data section when generating 
   * machine code.
   */
  vm::bump strings;

  /**
   * @brief A pool of arrays allocated for the IR,
   * will be cloned into the data section when generating 
   * machine code.
   */
  vm::arena<vm::array> arrays;

  /**
   * @brief The AST nodes used to generate SSA instructions.
   */
  parser& parser;

  /**
   * @brief Operation to be performed by a specific IR instruction.
   */
  enum class opcode : idx {
    /// @brief Panics with an error corresponding to `vm::error::error_name`.
    PANIC,

    /// @brief Performs no operation. This instruction is *never* compiled in JIT mode.
    NOP,

    /**
     * @brief Addition operation, coerces the second argument to the first.
     * 
     * @note Type specific behavior:
     * @note - int: Adds the second argument to the first.
     * @note - string: Appends the second argument to the first.
     * @note - array: Appends the second argument to the first.
     * @note - bool: Results in `vm::error::add_boolean`.
     * @note - null: Results in `vm::error::add_null`.
     */
    ADD,

    /**
     * @brief Subtraction operation, coerces the second argument to the first.
     * 
     * @note Type specific behavior:
     * @note - int: Subtracts the second argument from the first.
     * @note - string: Results in `vm::error::subtract_string`.
     * @note - array: Results in `vm::error::subtract_array`.
     * @note - bool: Results in `vm::error::subtract_boolean`.
     * @note - null: Results in `vm::error::subtract_null`.
     */
    SUB,
  };

  enum class flags {
    COMPACT,
    EXTENDED,
    CONSTANT,
  };

  /**
   * @brief Compact form of an SSA instruction.
   * 
   * `anchor` is a unsigned, negative offset from the current
   * instruction referencing `v1`, the first argument of the
   * instruction.
   * 
   * `anchor` may be signed if specified by the corresponding
   * `opcode`. The offset represents the number
   * of instructions preceding the current instruction.
   * 
   * `v2` and `v3` are signed 16-bit offsets from `anchor`,
   * with a positive offset representing instructions above
   * the anchor instruction.
   * 
   * Should `anchor`, `v1`, or `v2` not fit, you should use
   * the extended instruction form.
   * 
   * @note Flag value will always match `ir::flags::COMPACT`.
   */
  struct compact {
    flags flag : 2;
    opcode op : 6;
    idx anchor : 24;
    idx v2 : 16;
    idx v3 : 16;

    compact(opcode op) : flag(flags::COMPACT), op(op), anchor(0), v2(0), v3(0) {}
  };

  /**
   * @brief Extended form of an SSA instruction.
   * 
   * `e_anchor` is a 32-bit index into the extended instruction array
   * associated with each IR block.
   * 
   * @note This form is used when the compact form is not sufficient.
   * @note Flag value will always match `ir::flags::EXTENDED`.
   */
  struct extended {
    flags flag : 2;
    opcode op : 6;
    idx e_anchor : 32;

    extended(opcode op, idx v1) : flag(flags::EXTENDED), op(op), e_anchor(v1) {}
  };

  /**
   * @brief Constant form of an SSA instruction.
   * 
   * `is_string` indicates whether the constant value is a string.
   * `value` is the constant value itself.
   * 
   * `value` with either be a 61-bit integer or the top 61 bits of a
   * pointer (e.g. a tagged pointer).
   * 
   * @note This form is used when the instruction's value is a constant.
   * @note Flag value will always match `ir::flags::CONSTANT`.
   */
  struct constant {
    /// @note Refers to `ir::flags`.
    uint64_t flag : 2;
    uint64_t is_string : 1;
    uint64_t value : 61;

    constant(bool is_string, uintptr_t value) : flag(static_cast<uint64_t>(flags::CONSTANT)), is_string(is_string), value(value) {}
  };

  /**
   * @brief An individual SSA instruction.
   * 
   * This is a union of the three possible forms of an SSA instruction.
   * SSA instructions should fit within a single 64-bit word.
   */
  union instruction {
    compact compact;
    extended extended;
    constant constant;

    instruction() : compact(opcode::NOP) {}
  };

  /**
   * @brief The array of SSA instructions.
   * 
   * @note SSA blocks are simply spans over the
   * continuous array of instructions.
   */
  std::vector<instruction> instructions;

  idx length() const {
    return instructions.size();
  }

  idx emit_constant(vm::string& str);
  idx emit_constant(vm::array& arr);
  idx emit_constant(int64_t num);

  idx emit_compact(ir::opcode op, idx v1 = 0, idx v2 = 0, idx v3 = 0);
  idx emit_extended(ir::opcode op, idx v1, idx v2 = 0, idx v3 = 0);

  idx emit_instruction(opcode op);
  idx emit_instruction(opcode op, idx v1);
  idx emit_instruction(opcode op, idx v1, idx v2);
  idx emit_instruction(opcode op, idx v1, idx v2, idx v3);

  idx emit_string(frog::span range);
  idx emit_number(frog::span range);


  idx emit(instruction instr) {
    instructions.emplace_back(instr);
    return length() - 1;
  }

  // Ensure that individual format fits within 8 bytes.
  static_assert(sizeof(compact) == 8);
  static_assert(sizeof(extended) == 8);
  static_assert(sizeof(constant) == 8);

  // Ensure that instructions fit within 8 bytes.
  static_assert(sizeof(instruction) == 8);
};