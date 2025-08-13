#ifndef COMPILE_H
#define COMPILE_H

#include "ir.h"
#include "opt.h"

#include "dasm_proto.h"

#if _WIN32
    #include <Windows.h>
    #define jalloc(size) VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)
    #define jprotect(ptr, size) {DWORD mprev; VirtualProtect(ptr, size, PAGE_EXECUTE_READ, &mprev); }
#else
    #include <sys/mman.h>
    #if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
        #define MAP_ANONYMOUS MAP_ANON
    #endif
    #define jalloc(size) mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
    #define jprotect(ptr, size) mprotect(ptr, size, PROT_READ | PROT_EXEC);
#endif

static unsigned int pc(dasm_State** Dst, unsigned int cpc, unsigned int apc) {
    if (apc <= cpc) {
        apc *= 2;
        dasm_growpc(Dst, apc);
    }
    
    return cpc++;
}

static void* link(dasm_State** d) {
    size_t size;
    void* ptr;

    dasm_link(d, &size);
    ptr = jalloc(size);
    
    dasm_encode(d, ptr);
    jprotect(ptr, size);

    return ptr;
}

void* compile(ir_function_t* ir, opt_liveness_t* liveness);

#endif