#include "cli.hpp"
#include "file.hpp"

#include "logs/frog.hpp"

#include "parser/lexer.hpp"
#include "parser/parser.hpp"

#include <string>

int execute(std::string_view input) {
  lexer lex(input);
  parser parse(lex);

  return 0;
}

int main(int argc, char **argv) {
  cli::config cfg = cli::parse(argc, argv);

  if (cfg.flags & cli::flags::file) {
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
