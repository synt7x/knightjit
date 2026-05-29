#include "file.hpp"

#include <fstream>
#include <string>

namespace file {

std::string read(const char* path) {
    std::ifstream file(path, std::ios::binary);

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, ::std::ios::beg);

    std::string buffer(size, '\0');
    file.read(buffer.data(), size);

    return buffer;
}

}