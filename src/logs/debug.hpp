#pragma once

#include "debug.hpp"
#include "parser.hpp"
#include "ir.hpp"

#include <cstdint>
#include <string_view>
#include <iostream>

namespace debug {

void inspect(lexer lex);
void inspect(parser* parse);
void inspect(ir* ir);
void inspect(parser* parse, parser::node node,
             const std::string& prefix = "",
             bool is_last = true);
const std::string_view inspect(parser::node_type type);

}