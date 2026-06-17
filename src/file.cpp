#include "file.hpp"

#include <fstream>
#include <string>
#include <string_view>

namespace file {

std::string read(std::string_view path) {
  std::ifstream file(std::string{path}, std::ios::binary);

  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  file.seekg(0, ::std::ios::beg);

  std::string buffer(size, '\0');
  file.read(buffer.data(), size);

  return buffer;
}

bool exists(std::string_view path) {
  std::ifstream file(std::string{path}, std::ios::binary);
  return file.good();
}

} // namespace file