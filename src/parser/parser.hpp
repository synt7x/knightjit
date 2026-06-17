#pragma once

#include "lexer.hpp"

/**
 * @brief 
 * 
 */
class parser {
public:
  parser(lexer l) : lex(l) {}

private:
  lexer lex;
};