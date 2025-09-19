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

typedef struct reg_info {
    regs_t* regs;
    int max_slot;
} reg_info_t;

reg_info_t reg_allocate(ir_function_t* ir, opt_liveness_t* liveness);

#endif