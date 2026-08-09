#pragma once

#include <cstdint>

#include "value.hpp"

namespace vm {

struct array {
    std::uint64_t length;
    value content[];
};

}