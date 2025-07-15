#ifndef CLI_H
#define CLI_H

typedef enum flags {
    CONFIG_VERBOSE = 1 << 0,
    CONFIG_JIT = 1 << 1,
    CONFIG_FILE = 1 << 2,
} flags_t;

typedef struct cli_config {
    flags_t flags;
    char* input;
} cli_config_t;

cli_config_t cli_parse(int argc, char* argv[]);

#endif