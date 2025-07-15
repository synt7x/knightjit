# KnightJIT

## Project Layout

```rb
knightjit/
├── src/             # Source code for the KnightJIT interpreter
│   ├── knight.c     # Entry point
│   ├── lexer.c      # Generates tokens
│   ├── parser.c     # Generates the AST
│   ├── ir.c         # Generates IR
│   ├── ...          # Other utilities
│   └── jit/
│       ├── ...      # Sources for JIT compilation
│       └── value.h  # Handling Knight values
├── target/          # Build output directory
│   ├── artifacts/   # Build artifacts
│   ├── luajit/      # Dependencies from LuaJIT
│   └── knight       # Compiled binary
├── Makefile
├── README.md        # Project documentation
└── LICENSE          # License file
```

## Building

The only requirements for building the KnightJIT interpreter is a C compiler (e.g. `clang` or `gcc`), and an installation of `git` and `make`. Once these prerequisites are installed, simply run `make` in the parent directory to build the knight binary as `target/knight`.
