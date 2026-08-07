#pragma once

#include <cstdint>
#include <string_view>

namespace vm {

struct string {
    uint32_t length;
    uint32_t hash;

    char content[];

    std::string_view view() const {
        return { content, length };
    }
};

}