#include "cli.hpp"
#include "file.hpp"

#include "frog.hpp"

#include "lexer.hpp"
#include "parser.hpp"
#include "debug.hpp"

#include <string>
#include <iostream>

int execute(std::string_view input) {
  lexer lex(input);
  parser parse(lex);

  parser::ast ast = parse.parse();
  if (parse.failed) return 1;

  debug::inspect(ast, &parse);

  return 0;
}

int main(int argc, char **argv) {
  cli::config cfg = cli::parse(argc, argv);

  if (cfg.has(cli::flags::file)) {
    for (std::string_view name : cfg.args) {
      if (!file::exists(name))
        frog::croak(frog::level::error, frog::message::unavailable_file, name);
      std::string input = file::read(name);

      execute(input);
    }
  } else {
    for (std::string_view input : cfg.args)
      execute(input);
  }

  return 0;
}
