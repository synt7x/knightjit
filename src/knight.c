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
#include "vm.h"

#include "jit/value.h"

int main(int argc, char* argv[]) {
    cli_config_t config = cli_parse(argc, argv);

    info(config, "KnightJIT v1.0.0");
    info(
        config,
        "FLAGS 0x%x (%s%s%s)",
        config.flags,
        config.flags & CONFIG_VERBOSE ? "VERBOSE " : "",
        config.flags & CONFIG_FILE ? "FILE " : "INLINE ",
        config.flags & CONFIG_JIT ? "JIT" : "JIT-OFF"
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
        lexer_init(&lexer, input, size);
    } else if (config.input) {
        info(config, "Executing inline code");

        lexer_init(&lexer, config.input, strlen(config.input));
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

    #ifndef JIT_OFF
    if ((config.flags & CONFIG_JIT) == 0) {
    #endif

        vm_t* vm = vm_run(ir, arena);

    #ifndef JIT_OFF
    }
    #endif

    arena_free(arena);

    return 0;
}