#pragma once

#include "debug.hpp"
#include "parser.hpp"

#include <cstdint>
#include <string_view>
#include <iostream>

namespace debug {

void inspect(lexer lex);
void inspect(parser* parse);
void inspect(parser::node node, parser* parse, uint32_t depth = 0);
const std::string_view inspect(parser::node_type type);

}