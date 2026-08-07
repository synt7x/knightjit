#pragma once

#include <cstdint>

namespace vm {

enum class tag {
    number,
    string,
    array,
    boolean,
    null,
    block,

    mask = 0b111
};

class alignas(8) value {
    uint64_t raw;

    tag type() const {
        return static_cast<tag>(raw & static_cast<uint64_t>(tag::mask));
    }
};

}