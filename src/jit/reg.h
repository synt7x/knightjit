#ifndef REG_H
#define REG_H

#include <stddef.h>
#include "ir.h"
#include "opt.h"

/*
 * regs_t (high bits -> stack slot)(low bits -> register number)
 */
typedef struct regs {
    int reg;
    int slot;
} regs_t;

regs_t* reg_allocate(ir_function_t* ir, opt_liveness_t* liveness);

#endif