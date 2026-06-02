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

constexpr flags operator|(flags lhs, flags rhs) {
  return static_cast<flags>(static_cast<uint32_t>(lhs) |
                            static_cast<uint32_t>(rhs));
}

constexpr flags operator&(flags lhs, flags rhs) {
  return static_cast<flags>(static_cast<uint32_t>(lhs) &
                            static_cast<uint32_t>(rhs));
}

constexpr flags operator~(flags f) {
  return static_cast<flags>(~static_cast<uint32_t>(f));
}

inline flags &operator|=(flags &lhs, flags rhs) {
  lhs = lhs | rhs;
  return lhs;
}

inline flags &operator&=(flags &lhs, flags rhs) {
  lhs = lhs & rhs;
  return lhs;
}

inline constexpr std::string_view help_text =
    "Usage: knightjit [options] <input_file>\n"
    "Options:\n"
    "  -v, --version        Show version information\n"
    "  -h, --help           Show this help message\n"
    "  -V, --verbose        Enable verbose output\n"
    "  -e, --execute        Execute a string of Knight code\n"
    "  -j, --jit-off        Disable JIT compilation";

struct config {
  uint32_t flags = flags::jit | flags::file;
  std::vector<std::string_view> args;
};

config parse(int argc, char **argv);

} // namespace cli