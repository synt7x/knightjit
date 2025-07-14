#include <stdio.h>
#include <stdlib.h>

#include "cli.h"
#include "debug.h"

int main(int argc, char* argv[]) {
    cli_config_t config = cli_parse(argc, argv);

    info(config, "KnightJIT v1.0.0");
    info(
        config,
        "FLAGS 0x%x (%s%s%s)",
        config.flags,
        config.flags & CONFIG_VERBOSE ? "VERBOSE" : "",
        config.flags & CONFIG_FILE ? " FILE" : " INLINE",
        config.flags & CONFIG_JIT ? " JIT" : " JIT-OFF"
    );

    return 0;
}