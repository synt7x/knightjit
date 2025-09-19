#if defined(JIT_ON) && defined(X64)

#include <stdio.h>
#include <stdlib.h>

#include "reg.h"
#include "compile.h"

#include "value.h"
#include "vm.h"

#include "dasm_x86.h"

| .arch x64

int jit_truthy(ir_instruction_t* instr) {
    switch (instr->op) {
        case IR_CONST_STRING:
            v_string_t str = (v_string_t) (instr->constant.value & VALUE_MASK);
            return str->length > 0 ? 1 : 0;
        case IR_CONST_NUMBER:
            return (instr->constant.value >> 3) != 0 ? 1 : 0;
        case IR_CONST_BOOLEAN:
            return (instr->constant.value >> 3) != 0 ? 1 : 0;
        case IR_CONST_NULL:
            return 0;
        case IR_CONST_ARRAY:
            v_list_t list = (v_list_t) (instr->constant.value & VALUE_MASK);
            return list->length > 0 ? 1 : 0;
        default:
            return -1;
    }
}

v_t jit_prompt() {
    int capacity = 16;
    int length = 0;
    char* buffer = malloc(16);
    if (!buffer) panic("Failed to allocate memory for prompt buffer");

    char character;
    while ((character = getchar()) != EOF && character != '\n') {
        if (length + 1 >= capacity) {
            capacity *= 2;
            char* str = realloc(buffer, capacity);
            if (!str) {
                free(buffer);
                panic("Failed to reallocate memory for prompt buffer");
            }

            buffer = str;
        }
        buffer[length++] = character;
    }

    if (character == EOF && length == 0) {
        free(buffer);
        return (v_t)TYPE_NULL;
    }

    if (length > 0 && buffer[length - 1] == '\r') {
        length--;
    }

    buffer[length] = '\0';

    v_t result = v_create_string(buffer, length);
    free(buffer);
    return result;
}

void jit_add(dasm_State** Dst, ir_function_t* ir, ir_id_t left, ir_id_t right, ir_id_t result, regs_t* regs) {
    ir_instruction_t* l = jit_fetch(ir, left);
    ir_instruction_t* r = jit_fetch(ir, right);

    int reg = regs[result].reg;

    | mov64 Rq(reg), (r->constant.value >> 3)
    | add Rq(reg), (l->constant.value >> 3)
    | shl Rq(reg), 3
}

void jit_sub(dasm_State** Dst, ir_function_t* ir, ir_id_t left, ir_id_t right, ir_id_t result, regs_t* regs) {
    ir_instruction_t* l = jit_fetch(ir, left);
    ir_instruction_t* r = jit_fetch(ir, right);

    int reg = regs[result].reg;

    | mov64 Rq(reg), (l->constant.value >> 3)
    | sub Rq(reg), (r->constant.value >> 3)
    | shl Rq(reg), 3
}

void jit_branch(dasm_State** Dst, ir_function_t* ir, ir_instruction_t* instr, regs_t* regs) {
    ir_block_t* truthy = instr->branch.truthy;
    ir_block_t* falsey = instr->branch.falsey;

    ir_id_t condition = instr->branch.condition;
    ir_instruction_t* cond = jit_fetch(ir, condition);
    // int reg = regs[condition].reg;
    int constant = jit_truthy(cond);
    if (constant != -1) {
        | jmp =>truthy->id
    } else if (!constant) {
        | jmp =>falsey->id
    }
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

void jit_dump(v_t value) {
    if (V_IS_LIST(value)) {
        v_list_t list = (v_list_t)(value & VALUE_MASK);

        printf("[");
        for (size_t i = 0; i < list->length; ++i) {
            if (i > 0) printf(",");
            if (V_IS_STRING(list->items[i])) {
                printf("'");
            }

            jit_dump(list->items[i]);

            if (V_IS_STRING(list->items[i])) {
                printf("'");
            }
        }
        printf("]");
    } else if (V_IS_NULL(value)) {
        printf("null");
    } else if (V_IS_BLOCK(value)) {
        printf("BLOCK (0x%llx)", (unsigned long long)(value & VALUE_MASK));
    } else {
        v_t string = v_coerce_to_string(value);
        v_string_t str = (v_string_t) (string & VALUE_MASK);
        
        if (V_IS_NULL(value)) {
            printf("null");
        } else {
            printf("%s", str->data);
        }
    }
}

void* compile(ir_function_t* ir, reg_info_t reg_info) {
    dasm_State* d;
    unsigned int apc = 8;
    unsigned int cpc = 0;

    regs_t* regs = reg_info.regs;

    | .define arg, Rq(ARG_REG)
    | .define hldr, Rq(CLOBBER_REG)
    | .define temp, Rq(6)

    | .macro prelude
    | push rbp
    | push arg
    | push hldr
    | sub rsp, 40
    | .endmacro

    | .macro epilogue
    | add rsp, 40
    | pop hldr
    | pop arg
    | pop rbp
    | .endmacro

    | .macro foreign, fn
    | mov rbp, rsp
    | mov64 rax, (uint64_t) fn
    | call rax
    | .endmacro

    | .macro ldr, r, v
    || if (r.reg != -1) {
        || if (r.reg > 3) {
        | mov Rq(r.reg + 3), v
        ||} else {
        | mov Rq(r.reg), v
        ||}
    ||} else if (r.slot != -1) {
    | mov temp, v
    | mov [rbp - (r.slot * 8)], temp
    ||}
    | .endmacro

    | .macro rdr, t, r
    ||if (r.reg != -1) {
    || if (r.reg > 3) {
    | mov t, Rq(r.reg + 3)
    ||} else {
    | mov t, Rq(r.reg)
    ||}
    ||} else if (r.slot != -1) {
    | mov t, [rbp - (r.slot * 8)]
    ||}
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

        if (cpc == 0) {
            | lea rbp, [rsp - 8]
            | sub rsp, (8 + reg_info.max_slot * 8)
        }

        cpc++;
        for (int i = 0; i < block->instruction_count; i++) {
            ir_instruction_t instr = block->instructions[i];

            regs_t reg = regs[instr.result];

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
                    | lea temp, [<2]
                    | or temp, TYPE_STRING
                    | ldr reg, temp
                    break;
                case IR_CONST_ARRAY:
                    | .constants
                    | .align 8
                    v_list_t list = (v_list_t) (instr.constant.value & VALUE_MASK);

                    | 2:
                    | .qword list->length
                    | .qword list->capacity
                    | .qword >1
                    | 1:
                    for (size_t i = 0; i < list->capacity; i++) {
                        | .qword list->items[i]
                    }

                    | .code
                    // | lea Rq(reg), [<2]
                    // | or Rq(reg), TYPE_LIST
                    break;
                case IR_PROMPT:
                    | prelude
                    | foreign jit_prompt
                    | epilogue
                    break;
                case IR_ADD:
                    jit_add(Dst, ir, instr.generic.operands[0], instr.generic.operands[1], instr.result, regs);
                    break;
                case IR_SUB:
                    jit_sub(Dst, ir, instr.generic.operands[0], instr.generic.operands[1], instr.result, regs);
                    break;
                case IR_OUTPUT:
                    ir_instruction_t* value = jit_fetch(ir, instr.generic.operands[0]);
                    
                    | prelude

                    switch (value->op) {
                        case IR_CONST_NUMBER: case IR_CONST_BOOLEAN: case IR_CONST_NULL:
                            | mov64 arg, value->constant.value
                            break;
                        default:
                            | rdr arg, regs[instr.generic.operands[0]]
                            break;
                    }

                    
                    | foreign jit_output
                    | epilogue
                    | ldr reg, TYPE_NULL
                    break;
                case IR_DUMP:
                    value = jit_fetch(ir, instr.generic.operands[0]);
                    
                    | prelude

                    switch (value->op) {
                        case IR_CONST_NUMBER: case IR_CONST_BOOLEAN: case IR_CONST_NULL:
                            | mov64 arg, value->constant.value
                            break;
                        default:
                            | rdr arg, regs[instr.generic.operands[0]]
                            break;
                    }

                    | foreign jit_dump

                    | ldr reg, arg
                    | epilogue
                    break;
                case IR_JUMP:
                    if (instr.jump.block->id != block->id) {
                        | jmp =>instr.jump.block->id
                    }
                    break;
                case IR_BRANCH:
                    jit_branch(Dst, ir, &instr, regs);
                    break;
                case IR_SAVE:
                    for (int i = 0; i < instr.generic.operand_count; i++) {
                        ir_id_t operand = instr.generic.operands[i];
                        ir_instruction_t* op = jit_fetch(ir, operand);
                        int reg = regs[operand].reg;

                        //| push Rq(reg)
                    }
                    break;
                case IR_RESTORE:
                    for (int i = instr.generic.operand_count - 1; i >= 0; i--) {
                        ir_id_t operand = instr.generic.operands[i];
                        ir_instruction_t* op = jit_fetch(ir, operand);
                        int reg = regs[operand].reg;

                        //| pop Rq(reg)
                    }
                    break;
                default:
                    continue;
            }
        }
        | add rsp, (8 + reg_info.max_slot * 8)
        | ret
    }

    void* program = link(&d) + dasm_getpclabel(&d, 0);
    dasm_free(&d);

    return (void*) program;
}

#endif
