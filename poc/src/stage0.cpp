#include <stdio.h>
#include <stdlib.h>

#include "dasm_proto.h"
#include "dasm_x86.h"

#if _WIN32
    #include <Windows.h>
    #define jalloc(size) VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)
    #define jprotect(ptr, size) {DWORD mprev; VirtualProtect(ptr, size, PAGE_EXECUTE_READWRITE, &mprev); }
    #define ARG_REG 1
    #define WIN_ABI
#else
    #include <sys/mman.h>
    #if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
        #define MAP_ANONYMOUS MAP_ANON
    #endif
    #define jalloc(size) mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
    #define jprotect(ptr, size) mprotect(ptr, size, PROT_READ | PROT_WRITE | PROT_EXEC);
    #define ARG_REG 7
    #define POSIX_ABI
#endif

void* link(dasm_State** d) {
    size_t size;
    void* ptr;

    dasm_link(d, &size);
    ptr = jalloc(size);
    
    dasm_encode(d, ptr);
    jprotect(ptr, size);

    return ptr;
}

| .arch x64