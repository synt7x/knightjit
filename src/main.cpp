#include "cli.hpp"
#include "file.hpp"

#include "parser/lexer.hpp"

#include <string>

int main(int argc, char **argv) {
  cli::config cfg = cli::parse(argc, argv);

  auto input = file::read("examples/99_long_string.kn");

  lexer lex(input);

  lex.consume();
  lex.consume();

  return 0;
}
