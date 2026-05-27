#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace cli {

enum config_flags : uint32_t {
  none = 0,
  verbose = 1 << 0,
  jit = 1 << 1,
  file = 1 << 2,
  debug = 1 << 3,
};

constexpr config_flags operator|(config_flags lhs, config_flags rhs) {
  return static_cast<config_flags>(static_cast<uint32_t>(lhs) |
                                   static_cast<uint32_t>(rhs));
}

constexpr config_flags operator&(config_flags lhs, config_flags rhs) {
  return static_cast<config_flags>(static_cast<uint32_t>(lhs) &
                                   static_cast<uint32_t>(rhs));
}

constexpr config_flags operator~(config_flags f) {
  return static_cast<config_flags>(~static_cast<uint32_t>(f));
}

inline config_flags &operator|=(config_flags &lhs, config_flags rhs) {
  lhs = lhs | rhs;
  return lhs;
}

inline config_flags &operator&=(config_flags &lhs, config_flags rhs) {
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
  uint32_t flags = config_flags::jit | config_flags::file;

  std::string_view input;
  std::vector<std::string_view> args;
};

config parse(int argc, char **argv);

} // namespace cli