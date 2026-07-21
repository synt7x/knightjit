#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace cli {

enum flags : uint32_t {
  none = 0,
  verbose = 1 << 0,
  jit = 1 << 1,
  file = 1 << 2,
  debug = 1 << 3,
};

inline constexpr bool flag(std::string_view arg, std::string_view s, std::string_view l) {
  return arg == s || arg == l;
}

inline constexpr std::string_view help_text =
    "Usage: knightjit [options] <input_file>\n"
    "\n"
    "Options:\n"
    "  -v, --version        Show version information\n"
    "  -h, --help           Show this help message\n"
    "  -V, --verbose        Enable verbose output\n"
    "  -e, --execute        Execute a string of Knight code\n"
    "  -j, --jit-off        Disable JIT compilation";

class config {
public:
  uint32_t flags = flags::jit | flags::file;
  std::vector<std::string_view> args;

  config() = default;

  void put(cli::flags flag) { flags |= flag; }
  void remove(cli::flags flag) { flags &= ~flag; }
  bool has(cli::flags flag) const { return (flags & flag) != 0; }
  void push(std::string_view arg) { args.push_back(arg); }
};

config parse(int argc, char **argv);

} // namespace cli