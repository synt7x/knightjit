#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "debug.h"

void cli_help() {
    printf("Usage: knight [options] <input_file>\n");
    printf("Options:\n");
    printf("  -v, --verbose        Enable verbose output\n");
    printf("  -h, --help           Show this help message\n");
    printf("  -e, --execute        Execute a string of Knight code\n");
    printf("  -j, --jit-off        Disable JIT compilation\n");
    printf("  -d, --debug          View debug output for IR\n");
}

cli_config_t cli_parse(int argc, char* argv[]) {
    if (argc < 2 || !argv) {
        cli_help();
        exit(0);
    }

    cli_config_t config = {0};
    config.flags = CONFIG_JIT | CONFIG_FILE;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            config.flags |= CONFIG_VERBOSE;
        } else if (strcmp(argv[i], "-j") == 0 || strcmp(argv[i], "--jit-off") == 0) {
            config.flags &= ~CONFIG_JIT;
        } else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--execute") == 0) {
            config.flags &= ~CONFIG_FILE;
            if (i + 1 < argc) {
                config.input = argv[++i];
            } else {
                panic("No string specified for -e");
                exit(1);
            }
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            config.flags |= CONFIG_IR;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            cli_help();
            exit(0);
        } else {
            config.flags |= CONFIG_FILE;
            config.input = argv[i];
        }
    }

    return config;
}