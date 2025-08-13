#ifndef REG_H
#define REG_H

#include "compile.h"

typedef struct reg_info {
    int reg;
    int stack_slot;
    int start, end;
} reg_info_t;

typedef reg_info_t* regs_t;
regs_t reg_allocate(ir_function_t* ir, opt_liveness_t* liveness);

#endif