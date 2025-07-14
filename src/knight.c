#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "cli.h"
#include "debug.h"

#include "lexer.h"

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

    lexer_t lexer = {};

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

    // lexer testing ^-^
    // do {
    //     lexer_load(&lexer.t, &lexer);
    //     if (lexer.t.type != TK_EOF) {
    //         info(config, "Token: %c (type: %d, length: %d, line: %d)",
    //              lexer.t.value ? *lexer.t.value : 'N',
    //              lexer.t.type, lexer.t.length, lexer.linenumber);
    //     }
    // } while (lexer.t.type != TK_EOF);
    
    return 0;
}