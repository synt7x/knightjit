#include "cli.hpp"

#include "parser/lexer.hpp"
#include "parser/parser.hpp"

int main(int argc, char **argv) {
  cli::config cfg = cli::parse(argc, argv);

  return 0;
}
