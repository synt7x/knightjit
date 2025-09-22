#ifndef COMPILE_H
#define COMPILE_H

#include "ir.h"
#include "opt.h"
#include "jit/reg.h"

#include "dasm_proto.h"

#if _WIN32
    #include <Windows.h>
    #define jalloc(size) VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)
    #define jprotect(ptr, size) {DWORD mprev; VirtualProtect(ptr, size, PAGE_EXECUTE_READ, &mprev); }
    #define ARG_REG 2
    #define CLOBBER_REG 1
    #define WIN_ABI
#else
    #include <sys/mman.h>
    #if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
        #define MAP_ANONYMOUS MAP_ANON
    #endif
    #define jalloc(size) mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
    #define jprotect(ptr, size) mprotect(ptr, size, PROT_READ | PROT_WRITE | PROT_EXEC);
    #define ARG_REG 7
    #define CLOBBER_REG 1
    #define POSIX_ABI
#endif

static inline ir_instruction_t* jit_fetch(ir_function_t* ir, ir_id_t id) {
    if (id < 0 || id >= ir->next_value_id) {
        panic("Bad instruction fetch (fetch %d)", id);
    }

    for (int b = 0; b < ir->block_count; b++) {
        ir_block_t* block = ir->blocks[b];
        for (int i = 0; i < block->instruction_count; i++) {
            if (block->instructions[i].result == id) {
                return &block->instructions[i];
            }
        }
    }
}

static unsigned int pc(dasm_State** Dst, unsigned int cpc, unsigned int apc) {
    if (apc <= cpc) {
        apc *= 2;
        dasm_growpc(Dst, apc);
    }
    
    return cpc;
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

void* compile(ir_function_t* ir, reg_info_t reg_info);

#endif