#pragma once

#include <cstdint>
#include <string_view>

namespace vm {

struct string {
    std::uint64_t length;
    std::byte content[];
};

}