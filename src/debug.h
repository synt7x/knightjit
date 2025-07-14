#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdlib.h>
#include "cli.h"

#define dbg_verbose(config, fmt, ...) \
    config.flags & CONFIG_VERBOSE ? fprintf(stderr, fmt "\n", ##__VA_ARGS__) : (void)0

#define debug(config, fmt, ...) \
    dbg_verbose(config, "[DEBUG] " fmt, ##__VA_ARGS__)

#define info(config, fmt, ...) \
    dbg_verbose(config, "[INFO] " fmt, ##__VA_ARGS__)

#define warn(config, fmt, ...) \
    dbg_verbose(config, "[WARN] " fmt, ##__VA_ARGS__)

#define error(config, fmt, ...) \
    fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

#define panic(fmt, ...) \
    do { \
        fprintf(stderr, "[PANIC] " fmt "\n", ##__VA_ARGS__); \
        exit(1); \
    } while (0)

#endif