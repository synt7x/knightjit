#pragma once

#include <vector>

#include "arena.hpp"
#include "frog.hpp"

#include "lexer.hpp"

/**
 * @brief A parser that builds an AST from
 * the provided lexer.
 */
class parser {
public:
  /**
   * @brief Instance of the parser, which builds an
   * AST using the provided lexer.
   * 
   * @param l A `lexer` instance referring to the source string
   * @note Builds an AST following a single call of `parse()`
   */
  parser(lexer& l) : lex(l), nodes() {}

  /// @brief An alias for indexing nodes
  using node_id = vm::arena_id;

  /// @brief An alias for the type of nodes
  using node_type = lexer::token_type;

  /**
   * @brief An AST node, representing a single token
   * and its children in the tree.
   */
  struct node {
    node_type type;
    frog::span range;
    std::vector<node_id> children;
  };

  /// @brief An alias for the root node of the AST.
  using ast = node;

  /// @brief Flag indicating whether parsing failed due to an error.
  bool failed = false;

  /// @brief The lexer instance used to tokenize the input.
  lexer& lex;

  /**
   * @brief Retrieves a node from the AST by its ID.
   * 
   * @param id The ID of the node to retrieve
   * @return The node with the specified ID
   */
  node get(node_id id) {
    return nodes.at(id);
  }

  /**
   * @brief Parses the input source string and builds an AST. 
   * 
   * @return The root node of the AST
   */
  ast parse();
private:
  /// @brief The arena allocator used to store the AST
  vm::arena<node> nodes;

  /**
   * @brief Parses a single n-arity expression
   * 
   * @return `node_id` of the resultant `node`
   */
  node_id expression();

  /**
   * @brief Parses a single nullary expression.
   * 
   * @return `node_id` of the resultant `node`
   */
  [[nodiscard]]
  node_id nullary();

  /**
   * @brief Parses a single unary expression.
   * 
   * @return `node_id` of the resultant `node`
   */
  [[nodiscard]]
  node_id unary();

  /**
   * @brief Parses a single binary expression.
   * 
   * @return `node_id` of the resultant `node`
   */
  [[nodiscard]]
  node_id binary();

  /**
   * @brief Parses a single ternary expression.
   * 
   * @return `node_id` of the resultant `node`
   */
  [[nodiscard]]
  node_id ternary();

  /**
   * @brief Parses a single quaternary expression.
   * 
   * @return `node_id` of the resultant `node`
   */
  [[nodiscard]]
  node_id quaternary();
};