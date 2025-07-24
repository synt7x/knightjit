#ifndef VM_H
#define VM_H

#include "ir.h"
#include "jit/value.h"

typedef struct vm {
    ir_block_t* block;
    ir_function_t* function;

    v_t* variables;
    v_t* registers;

    int index;
} vm_t;

vm_t* vm_run(ir_function_t* function, arena_t* arena);

#endif