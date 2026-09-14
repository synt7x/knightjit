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
    idx start = 0;
    idx length = 0;
    idx successor = 0;

    std::vector<idx> extended = {};
  };

  std::vector<block> blocks = { 
    block { 0, 0, 0, {} } // The root block, length is updated once it is finished.
  };

  /// @brief Current SSA compilation unit.
  idx unit_index = 0;

  block& unit() {
    return blocks[unit_index];
  }

  /**
   * @brief Starts a new block
   * 
   * @return idx 
   */
  idx bnter() {
    unit().length = instructions.size() - unit().start;

    blocks.push_back(block {
      /*.start = */ static_cast<idx>(instructions.size()),
      /*.length = */ 0,
      /*.successor = */ 0,
      /*.extended = */ {},
    });

    unit_index = blocks.size() - 1;
    return unit_index;
  }

  /**
   * @brief Ends the current block
   * 
   * @return idx 
   */
  idx bxit() {
    idx finished = unit_index;
    unit().length = instructions.size() - unit().start;

    blocks.push_back(block {
      /*.start = */ static_cast<idx>(instructions.size()),
      /*.length = */ 0,
      /*.successor = */ 0,
      /*.extended = */ {},
    });

    unit_index = blocks.size() - 1;
    return finished;
  }

  /**
   * @brief Generates the SSA instructions from the
   * provided AST node in the form of a block.
   */
  idx generate_block(parser::node& node);


  /**
   * @brief Generates the SSA instructions from the
   * provided AST node.
   */
  idx generate(parser::node& node, bool tail = false);

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
   * 
   * @note Coercions are all explicit using the opcode::COERCE instruction,
   * @note even when specified that the operation is coercive.
   */
  enum class opcode : idx {
    /// @brief Panics with an error corresponding to `vm::error::error_name`.
    PANIC,

    /// @brief Performs no operation. This instruction is *never* compiled in JIT mode.
    NOP,

    /// @brief Points to the beginning of a block of instructions, which may be jumped to.
    BLOCK,

    /**
     * @brief Converts the first argument to and from ASCII.
     * 
     * @note Type specific behavior:
     * @note - int: Converts the first argument to a string containing its ASCII character.
     * @note - string: Converts the first argument to an int containing its ASCII value (of the first character).
     * @note - array: Results in `vm::error::ascii_array`.
     * @note - bool: Results in `vm::error::ascii_boolean`.
     * @note - null: Results in `vm::error::ascii_null`.
     */
    ASCII,

    /// @brief Calls the first argument, panics if the first argument is not a block.
    CALL,

    /**
     * @brief Retrieves the length of the first argument.
     * 
     * @note Type specific behavior:
     * @note - int: Returns the number of digits.
     * @note - string: Returns the number of characters.
     * @note - array: Returns the number of elements.
     * @note - bool: Results in `vm::error::length_boolean`.
     * @note - null: Returns 0.
     */
    LENGTH,
    OUTPUT,

    QUIT,
    NOT,
    NEGATE,


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

    /**
     * @brief Multiplication operation, coerces the second argument to the first.
     * 
     * @note Type specific behavior:
     * @note - int: Multiplies the first argument by the second.
     * @note - string: Repeats the first argument by the second argument number of times.
     * @note - array: Repeats the first argument by the second argument number of times.
     * @note - bool: Results in `vm::error::multiply_boolean`.
     * @note - null: Results in `vm::error::multiply_null`.
     */
    MUL,

    /**
     * @brief Division operation, coerces the second argument to the first.
     * 
     * @note Type specific behavior:
     * @note - int: Divides the first argument by the second.
     * @note - string: Results in `vm::error::divide_string`.
     * @note - array: Results in `vm::error::divide_array`.
     * @note - bool: Results in `vm::error::divide_boolean`.
     * @note - null: Results in `vm::error::divide_null`.
     */
    DIV,

    /**
     * @brief Modulo operation, coerces the second argument to the first.
     * 
     * @note Type specific behavior:
     * @note - int: Computes the remainder of the first argument divided by the second.
     * @note - string: Results in `vm::error::modulo_string`.
     * @note - array: Results in `vm::error::modulo_array`.
     * @note - bool: Results in `vm::error::modulo_boolean`.
     * @note - null: Results in `vm::error::modulo_null`.
     */
    MOD,

    /**
     * @brief Exponentiation operation, coerces the second argument to the first.
     * 
     * @note Type specific behavior:
     * @note - int: Raises the first argument to the power of the second.
     * @note - string: Results in `vm::error::power_string`.
     * @note - array: Joins elements in the first argument with the second argument as a separator.
     * @note - bool: Results in `vm::error::power_boolean`.
     * @note - null: Results in `vm::error::power_null`.
     */
    POW,

    /**
     * @brief Coerces the first argument to the type of the second argument.
     * 
     */
    COERCE,

    RETURN,

    /// @brief Unconditional jump to the instruction at the specified index.
    JMP,
    
    JZ,
    JNZ
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

    /**
     * @brief Unpacks the constant value as a string.
     * 
     * @return vm::string* 
     */
    vm::string* unpack() const {
      if (!is_string) return nullptr;
      return reinterpret_cast<vm::string*>(value << 3);
    }
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
  idx emit_block(idx block);
  idx emit_return(idx value, bool tail);

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