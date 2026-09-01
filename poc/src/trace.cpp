#include <cstddef>
#include <cstdint>
#include <iostream>
#include <atomic>

#include "shared.hpp"

using pool = uint64_t*;

void tracer(pool trace_pool, void(*program)()) {
    uint8_t* code = reinterpret_cast<uint8_t*>(program);
    uint8_t patch = 0;

    std::cout << "[Tracer] Tracing thread started." << std::endl;

    while (true) {
        uint64_t slot1 = trace_pool[0];
        uint64_t slot2 = trace_pool[1];

        if (patch == 0 && slot1 > 10000) {
            std::cout << "[Tracer] Integer phase solidification detected at count: " << slot1 << std::endl;

            #if _WIN32
            FlushInstructionCache(GetCurrentProcess(), code, 64000);
            #else
            __builtin___clear_cache(reinterpret_cast<char*>(code_base), 
                                    reinterpret_cast<char*>(code_base + 64000));
            #endif

            patch = 1;
            std::cout << "[Tracer] Specialized integer trace patched successfully." << std::endl;
        } else if (patch == 1 && slot2 > 10000) {
            std::cout << "[Tracer] String phase solidification detected at count: " << slot2 << std::endl;

            #if _WIN32
            FlushInstructionCache(GetCurrentProcess(), code, 64000);
            #else
            __builtin___clear_cache(reinterpret_cast<char*>(code_base), 
                                    reinterpret_cast<char*>(code_base + 64000));
            #endif

            patch = 1;
            std::cout << "[Tracer] Specialized string trace patched successfully." << std::endl;
            break;
        }
    }
}