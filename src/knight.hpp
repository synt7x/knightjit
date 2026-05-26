#pragma once

#include <string_view>

namespace knight {

inline constexpr std::string_view name = "KnightJIT";
inline constexpr std::string_view version = "${VERSION}-${GIT_HASH}";

}