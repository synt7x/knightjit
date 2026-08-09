#pragma once

#include <cstdint>
#include <string_view>

namespace vm {

struct string {
    std::uint64_t length;
    std::byte content[];

    const std::string_view view() const {
        return std::string_view(reinterpret_cast<const char*>(content), length);
    }
};

}