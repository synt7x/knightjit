#pragma once

#include <vector>

#include "../logs/frog.hpp"

#include "lexer.hpp"

/**
 * @brief An AST node, representing a single token
 * and its children in the tree.
 */
struct node {
  node_type type;
  frog::span range;
  std::vector<node> children;
};


/// @brief An alias for the root node of the AST.
using ast = node;

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
  parser(lexer l) : lex(l), allocator() {}

  /**
   * @brief Parses the input source string and builds an AST. 
   * 
   * @return The root node of the AST
   */
  ast parse();
private:
  /// @brief The lexer instance used to tokenize the input.
  lexer lex;

  /// @brief The arena allocator used to store the AST
  arena allocator;

  /**
   * @brief Parses a single nullary expression.
   * 
   * @return The resultant `node`
   */
  node nullary();

  /**
   * @brief Parses a single unary expression.
   * 
   * @return The resultant `node`
   */
  node unary();

  /**
   * @brief Parses a single binary expression.
   * 
   * @return The resultant `node`
   */
  node binary();

  /**
   * @brief Parses a single ternary expression.
   * 
   * @return The resultant `node`
   */
  node ternary();

  /**
   * @brief Parses a single quarternary expression.
   * 
   * @return The resultant `node`
   */
  node quarternary();
};