#include <cstddef>
#include <cstdint>
#include <iostream>

#include "shared.hpp"

using pool = uint64_t*;

constexpr uint8_t STUB1[256] = {

};

constexpr uint8_t STUB2[256] = {

};

constexpr uint8_t ADD_INT[100] = {
    // <GUARD>:
    0x48, 0x89, 0xca, // mov rdx, rcx
    0x48, 0x83, 0xe2, 0x07, // and rdx, 0x7
    0x74, 0x06, // jz <SKIP>
    0xff, 0x25, 0x00, 0x00, 0x00, 0x00, // jmp <STUB>
    
    // <SKIP>:
    0x49, 0xff, 0x06, // inc qword [trace_slot_1]
    0x48, 0x01, 0xcb, // add rbx, rcx
};

constexpr uint8_t ADD_STR[100] = {
     // <GUARD>:
    0x48, 0x89, 0xca, // mov rdx, rcx
    0x48, 0x83, 0xe2, 0x07, // and rdx, 0x7
    0x48, 0x83, 0xfa, 0x03, // cmp rdx, 3
    0x74, 0x06, // jz <SKIP>
    0xff, 0x25, 0x00, 0x00, 0x00, 0x00, // jmp <STUB>
    
    // <SKIP>:
    0x49, 0xff, 0x46, 0x08, // inc qword [trace_slot_2]

    0x48, 0x83, 0xe1, 0xf8, // and rcx, 0xfffffffffffffff8
    0x4c, 0x8b, 0x01, // mov r8, QWORD PTR [rcx]
    0x49, 0xc7, 0xc1, 0x00, 0x00, 0x00, 0x00, // mov r9, 0
    0x49, 0xc7, 0xc2, 0x00, 0x00, 0x00, 0x00, // mov r10, 0

    0x48, 0x01, 0xcb, // add rbx, rcx
};

void tracer(pool trace_pool, void(*program)()) {
    uint8_t* code = reinterpret_cast<uint8_t*>(program);
    uint8_t patch = 0;

    uint8_t* target_address = code + 52;
    uint8_t* stub = code + 184;

    std::cout << "[Tracer] Stub pool located " << (stub - target_address) << " bytes from the target address." << std::endl;
    std::cout << "[Tracer] Tracing thread started." << std::endl;

    while (true) {
        uint64_t slot1 = trace_pool[0];
        uint64_t slot2 = trace_pool[1];

        if (patch == 0 && slot1 > 10000) {
            std::cout << "[Tracer] Integer phase solidification detected at count: " << slot1 << std::endl;

            flush(code);

            patch = 1;
            std::cout << "[Tracer] Specialized integer trace patched successfully." << std::endl;
        } else if (patch == 1 && slot2 > 10000) {
            std::cout << "[Tracer] String phase solidification detected at count: " << slot2 << std::endl;

            flush(code);

            patch = 1;
            std::cout << "[Tracer] Specialized string trace patched successfully." << std::endl;
            break;
        }
    }
}