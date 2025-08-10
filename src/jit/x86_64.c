
#include <stdio.h>
#include <stdlib.h>

#include "ir.h"
#include "reg.h"

#include "dynasm/dasm_proto.h"
#include "dynasm/dasm_x86.h"

void* compile(ir_function_t* ir) {
    | .arch x64

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