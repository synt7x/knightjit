#pragma once

#include <string>
#include <string_view>

namespace file {

std::string read(std::string_view path);
bool exists(std::string_view path);

} // namespace file