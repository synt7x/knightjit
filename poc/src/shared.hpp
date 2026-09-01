#pragma once

#include <cstdint>
#include <atomic>

#if _WIN32
    #include <Windows.h>
    #define jalloc(size) VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)
    #define jprotect(ptr, size) {DWORD mprev; VirtualProtect(ptr, size, PAGE_EXECUTE_READWRITE, &mprev); }
#else
    #include <sys/mman.h>
    #if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
        #define MAP_ANONYMOUS MAP_ANON
    #endif
    #define jalloc(size) mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
    #define jprotect(ptr, size) mprotect(ptr, size, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif

inline static void flush(void* code) {
#if _WIN32
    FlushInstructionCache(GetCurrentProcess(), code, 64000);
#else
    __builtin___clear_cache(
        reinterpret_cast<char*>(code_base), 
        reinterpret_cast<char*>(code_base + 64000)
    );
    #endif
}

using pool = uint64_t*;
void* stage0(pool trace_pool);
void tracer(pool trace_pool, void(*program)());