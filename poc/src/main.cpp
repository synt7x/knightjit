#include <cstddef>
#include <iostream>
#include <thread>

#include "shared.hpp"

int main() {
    uint64_t trace_pool[2] = {0, 0};

    std::cout << "[JIT] Compiling stage0." << std::endl;
    void(*program)() = (void(*)()) stage0(trace_pool);
    std::cout << "[JIT] Successfully compiled stage0." << std::endl;
    std::thread tracing(tracer, trace_pool, program);

    std::cout << "[Runtime] Executing stage0." << std::endl;
    program();
    std::cout << "[Runtime] Successfully executed stage0." << std::endl;

    tracing.detach();
    
    return 0;
}