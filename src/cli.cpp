#include <iostream>

#include "knight.hpp"
#include "cli.hpp"

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
                std::cout
                    << knight::name
                    << " v"
                    << knight::version
                    << '\n';

                std::exit(0);
            } else if (flag(arg, "-V", "--verbose")) {

            }
        }
    }

    return cfg;
}

} // namespace cli