#pragma once

#ifndef KNIGHT_VERSION
#define KNIGHT_VERSION "dev"
#endif

#ifndef KNIGHT_GIT_HASH
#define KNIGHT_GIT_HASH "unknown"
#endif

#include <string_view>

namespace knight {

inline constexpr std::string_view name = "KnightJIT";
inline constexpr std::string_view version = KNIGHT_VERSION;
inline constexpr std::string_view git_hash = KNIGHT_GIT_HASH;

} // namespace knight