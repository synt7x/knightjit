#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "arena.h"
#include "cli.h"
#include "debug.h"
#include "map.h"

#include "lexer.h"
#include "parser.h"
#include "ir.h"
#include "opt.h"
#include "vm.h"

#include "jit/value.h"
#include "jit/reg.h"

#ifndef JIT_OFF
#include "jit/compile.h"
#endif

int main(int argc, char* argv[]) {
    cli_config_t config = cli_parse(argc, argv);

    info(config, "KnightJIT v1.0.0");
    info(
        config,
        "FLAGS 0x%x (%s%s%s)",
        config.flags,
        config.flags & CONFIG_VERBOSE ? "VERBOSE " : "",
        config.flags & CONFIG_FILE ? "FILE " : "INLINE ",
        
        #ifdef JIT_OFF
        "JIT-OFF"
        #else
        config.flags & CONFIG_JIT ? "JIT" : "JIT-OFF"
        #endif
    );

    lexer_t lexer;

    if (config.flags & CONFIG_FILE && config.input) {
        info(config, "Reading input file: %s", config.input);
        size_t size;
        char* input = file_read(config.input, &size);

        if (!input) {
            panic("Failed to read input file: %s", config.input);
        }

        info(config, "Read %zu bytes from file: %s", size, config.input);
        l_init(&lexer, input, size);
    } else if (config.input) {
        info(config, "Executing inline code");

        l_init(&lexer, config.input, strlen(config.input));
    } else {
        panic("No input provided. Use -h for help.");
    }

    arena_t* ast_arena = arena_create(512);
    ast_node_t* tree = parse(&lexer, ast_arena);
    
    info(config, "Parsed AST of size %zu", ast_arena->size);
    map_t* symbol_table = map_create(8);

    arena_t* arena = arena_create(512);
    ir_function_t* ir = ir_create(tree, arena, symbol_table, &config);

    arena_free(ast_arena);

    info(config, "Created IR with %d blocks, total size of %zu bytes", ir->block_count, arena->size);

    opt_liveness_t* liveness = ir_optimize(ir);
    regs_t* regs = reg_allocate(ir, liveness);

    if (config.flags & CONFIG_IR) {
        ir_function_t* function = ir;
        printf("IR (%p):\n", function);
        for (int i = 0; i < function->block_count; ++i) {
            ir_block_t* block = function->blocks[i];
            printf("  Block %d (id=%d %p):\n", i, block->id, block);
            for (int j = 0; j < block->instruction_count; ++j) {
                ir_instruction_t* instr = &block->instructions[j];
                printf("    [%d] op=%d", instr->result, instr->op);

                switch (instr->op) {
                    case IR_CONST_NUMBER:
                        printf(" NUMBER CONST value=%ld", (long)instr->constant.value >> 3);
                        break;
                    case IR_CONST_BOOLEAN:
                        printf(" BOOLEAN CONST value=%ld", (long)instr->constant.value >> 3);
                        break;
                    case IR_CONST_STRING:
                        printf(" STRING CONST");
                        break;
                    case IR_CONST_NULL:
                        printf(" NULL CONST");
                        break;
                    case IR_CONST_ARRAY:
                        printf(" @");
                        break;
                    case IR_LOAD:
                        printf(" LOAD var_id=%d", instr->var.var_id);
                        break;
                    case IR_STORE:
                        printf(" STORE var_id=%d value=%d", instr->var.var_id, instr->var.value);
                        break;
                    case IR_BLOCK:
                        printf(" BLOCK function=%p result_id=%d", (void*)(intptr_t)instr->block.function, instr->block.result_id);
                        break;
                    case IR_BRANCH:
                        printf(" BRANCH true=%d false=%d condition=%d", instr->branch.truthy->id, instr->branch.falsey->id, instr->branch.condition);
                        break;
                    case IR_JUMP:
                        printf(" JUMP block_id=%d", instr->jump.block->id);
                        break;
                    case IR_PHI:
                        printf(" PHI phi_count=%d", instr->phi.phi_count);
                        printf(" values=");
                        for (int k = 0; k < instr->phi.phi_count; ++k) {
                            printf("%d ", instr->phi.phi_values[k]);
                        }
                        printf(" blocks=");
                        for (int k = 0; k < instr->phi.phi_count; ++k) {
                            printf("%p ", instr->phi.phi_blocks[k]);
                        }
                        break;
                    case IR_RANDOM:
                        printf(" RANDOM");
                        break;
                    case IR_PROMPT:
                        printf(" PROMPT");
                        break;
                    default:
                        printf(" %s operands=", debug_ir_op_string(instr->op));
                        for (int k = 0; k < instr->generic.operand_count; ++k) {
                            printf("%d ", instr->generic.operands[k]);
                        }
                        break;
                    
                }

                printf(" REGISTER=%d STACK=%d", 
                       regs[instr->result].reg,
                       regs[instr->result].slot
                );

                printf("\n");
            }
        }
    }

    #ifndef JIT_OFF
    if ((config.flags & CONFIG_JIT) == 0) {
    #endif
        vm_run(ir, arena);
    #ifndef JIT_OFF
    } else {
        //opt_liveness_t* liveness = ir_optimize(ir);
        void (*program)() = compile(ir, regs);
        program();
    }
    #endif

    arena_free(arena);

    return 0;
}