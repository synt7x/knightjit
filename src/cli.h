#ifndef CLI_H
#define CLI_H

enum {
    CONFIG_VERBOSE = 1 << 0,
    CONFIG_JIT = 1 << 1,
    CONFIG_FILE = 1 << 2,
};

typedef int flags_t;

typedef struct {
    flags_t flags;
    char* input;
} cli_config_t;

void cli_help();
cli_config_t cli_parse(int argc, char* argv[]);
#endif