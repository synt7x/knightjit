#ifndef VM_H
#define VM_H

#include "ir.h"
#include "jit/value.h"

typedef struct vm_stack_item {
    ir_block_t* callee;
    ir_block_t* previous;
    int index;
    ir_id_t result;
    v_t* registers; // <-- add this line
} vm_stack_item_t;

typedef struct vm_stack {
    vm_stack_item_t* items;
    int size;
    int capacity;
} vm_stack_t;

typedef struct vm {
    ir_block_t* block;
    ir_function_t* function;

    v_t* variables;
    v_t* registers;

    int index;
} vm_t;

vm_t* vm_run(ir_function_t* function, arena_t* arena);

#endif