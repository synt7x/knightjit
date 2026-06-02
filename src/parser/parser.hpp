#pragma once

#include "lexer.hpp"

class parser {
public:
  parser(lexer lex);

private:
  lexer lex;
};