#include <iostream>

#include "cli.hpp"
#include "knight.hpp"

namespace cli {

#define flag(arg, s, l) arg == s || arg == l

config parse(int argc, char **argv) {
  config cfg;

  if (argc < 2) {
    std::cout << help_text << std::endl;
    std::exit(0);
  }

  for (int i = 1; i < argc; i++) {
    std::string_view arg = argv[i];

    if (cfg.input.empty()) {
      if (flag(arg, "-v", "--version")) {
        std::cout << knight::name << " v" << knight::version << " ("
                  << knight::git_hash << ')' << '\n';

        std::exit(0);
      } else if (flag(arg, "-h", "--help")) {
        std::cout << help_text << std::endl;
        std::exit(0);
      } else if (flag(arg, "-V", "--verbose")) {
        cfg.flags |= config_flags::verbose;
      } else if (flag(arg, "-j", "--jit-off")) {
        cfg.flags &= ~config_flags::jit;
      } else if (flag(arg, "-e", "--execute")) {
        cfg.flags &= ~config_flags::file;
      }
    } else {
        for (; i < argc; ++i) cfg.args.push_back(argv[i]);
        break;
    }
  }

  return cfg;
}

} // namespace cli