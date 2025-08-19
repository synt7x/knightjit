#if defined(JIT_ON) && defined(X64)

#include <stdio.h>
#include <stdlib.h>

#include "reg.h"
#include "compile.h"

#include "value.h"
#include "vm.h"

#include "dasm_x86.h"

| .arch x64

void jit_add(dasm_State** Dst, ir_function_t* ir, ir_id_t left, ir_id_t right, ir_id_t result, regs_t* regs) {
    ir_instruction_t* l = jit_fetch(ir, left);
    ir_instruction_t* r = jit_fetch(ir, right);

    int reg = regs[result].reg;

    | mov64 Rq(reg), (r->constant.value >> 3)
    | add Rq(reg), (l->constant.value >> 3)
    | shl Rq(reg), 3
}

void jit_output(v_t value) {
    v_t string = v_coerce_to_string(value);
    v_string_t str = (v_string_t) (string & VALUE_MASK);
    if (str->length > 0 && str->data[str->length - 1] == '\\') {
        printf("%.*s", (int)(str->length - 1), str->data);
        return;
    }

    puts(str->data);
    return;
}

void* compile(ir_function_t* ir, regs_t* regs) {
    dasm_State* d;
    unsigned int apc = 8;
    unsigned int cpc = 0;

    | .define arg, Rq(ARG_REG)
    | .define hldr, Rq(CLOBBER_REG)

    | .macro prelude
    | push arg
    | push hldr
    | .endmacro

    | .macro epilogue
    | pop hldr
    | pop arg
    | .endmacro

    | .macro foreign, fn
    | mov64 rax, (uint64_t) fn
    | call rax
    | .endmacro

    | .section code, constants, variables
    dasm_init(&d, DASM_MAXSECTION);

    | .globals label_
    void* labels[label__MAX];
    dasm_setupglobal(&d, labels, label__MAX);

    | .actionlist kn_actions
    dasm_setup(&d, kn_actions);

    dasm_State** Dst = &d;
    dasm_growpc(Dst, apc);

    | .variables
    | ->variables:
    for (int i = 0; i < ir->var_id; i++) {
        | .qword 0
    }

    | .code
    for (int b = 0; b < ir->block_count; b++) {
        ir_block_t* block = ir->blocks[b];

        block->id = cpc;
        | =>pc(&d, cpc, apc):
        for (int i = 0; i < block->instruction_count; i++) {
            ir_instruction_t instr = block->instructions[i];

            int reg = regs[instr.result].reg;

            switch (instr.op) {
                case IR_CONST_STRING:
                    | .constants
                    | .align 8
                    v_string_t str = (v_string_t) (instr.constant.value & VALUE_MASK);

                    | 2:
                    | .qword str->length
                    | .qword >1
                    | 1:
                    for (size_t i = 0; i < str->length; i++) {
                        | .byte str->data[i]
                    }

                    | .code
                    | lea Rq(reg), [<2]
                    | or Rq(reg), TYPE_STRING
                    break;
                case IR_ADD:
                    jit_add(Dst, ir, instr.generic.operands[0], instr.generic.operands[1], instr.result, regs);
                    break;
                case IR_OUTPUT:
                    | mov rcx, Rq(regs[instr.generic.operands[0]].reg)

                    | prelude

                    #ifdef WIN_ABI
                    | sub rsp, 40
                    | foreign jit_output
                    | add rsp, 40
                    #else
                    | foreign jit_output
                    #endif

                    | epilogue
                    break;
                case IR_PUSH:
                    for (int i = 0; i < instr.generic.operand_count; i++) {
                        ir_id_t operand = instr.generic.operands[i];
                        ir_instruction_t* op = jit_fetch(ir, operand);
                        int reg = regs[operand].reg;

                        | push Rq(reg)
                    }
                case IR_POP:
                    for (int i = instr.generic.operand_count - 1; i >= 0; i--) {
                        ir_id_t operand = instr.generic.operands[i];
                        ir_instruction_t* op = jit_fetch(ir, operand);
                        int reg = regs[operand].reg;

                        | pop Rq(reg)
                    }
                default:
                    continue;
            }
        }
        | ret
    }

    void* program = link(&d) + dasm_getpclabel(&d, 0);
    dasm_free(&d);

    return (void*) program;
}

#endif