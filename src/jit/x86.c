/*
 * This file is currently a work in progress.
 *
 * Look at x86_64.c for the JIT that is being
 * actively developed.
 * 
 * x86.c will be developed once x86_64.c is finished.
 */

#if defined(JIT_ON) && defined(X86)

#include <stdio.h>
#include <stdlib.h>

#include "reg.h"
#include "compile.h"

#include "dasm_x86.h"

void* compile(ir_function_t* ir, opt_liveness_t* liveness) {
    | .arch x86

    dasm_State* d;
    unsigned npc = 8;
    unsigned nextpc = 0;

    | .section code
    dasm_init(&d, DASM_MAXSECTION);

    | .globals label_
    void* labels[label__MAX];
    dasm_setupglobal(&d, labels, label__MAX);

    | .actionlist kn_actions
    dasm_setup(&d, kn_actions);

    dasm_growpc(&d, npc);


}

#endif