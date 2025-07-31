# Contributing

Contributions are welcome! Feel free to make a pull-request to propose any changes you see fit. Generally, it is recommended to follow the C style present in the pre-existing files in the project.

## Todo

KnightJIT is still being actively developed, and is continually improving.

The current order of priorities are:
    * `opt.c`, adding more optimizations and liveness analysis.
    * `x86_64.c`, begin emitting generated JIT code.
    * `gc.c`, write a fast minimally invasive garbage collector.
    * `vm.c`, increase general VM performance.

## Building

The only requirements for building the KnightJIT interpreter is a C compiler (e.g. `clang` or `gcc`), and an installation of `git` and `make`. Once these prerequisites are installed, simply run `make` in the parent directory to build the knight binary as `target/knight`.

```sh
$ make
```

KnightJIT requires the LuaJIT source code for dynasm aswell as preprocessing. When running `make`, the LuaJIT repository should be automatically cloned into the `target/luajit/` directory.

To modify the build process, the following environment variables can be used:
    * `EXECUTABLE` specifies the name of the compiled binary.
    * `ARCH` specifies the architecture used by dynasm. Generally, this shouldn't be changed.
    * `JIT_ENABLED` specifies whether JIT compilation can be enabled.
    * `STANDARD` specifies whether the interpreter strictly follows the standard, or allows extensions (in this case, command line arguments).
    * `COMPILER` specifies which C compiler is used to build the project.
    * `LUA` specifies which Lua binary to use during compilation. This is not required, as the Makefile will default to the bundled Lua interpreter (minilua) provided by LuaJIT.

It should be noted that if you have changed the `LUA` environment variable to a Lua binary that is not **minilua** or **LuaJIT**, files that utilize dynasm (primarily JIT files) will not be able to be compiled.

## Testing

To run the full test suite, the bundled `minilua` that comes with LuaJIT does not suffice. You will need to install a seperate [Lua](https://lua.org) executable to run the tests. Once you have a Lua installation, configure the `LUA` environment variable to your Lua executable.

```sh
$ make test
```
or
```sh
$ make strict
```
to not stop on the first error.

If you are updating core VM logic, it is recommended to use `make increment` to run tests after compilation.

Tests may behave weirdly depending on how your operating system handles newlines, but generally (atleast in my testing) the tests should pass on most platforms.

## Project Layout

The interpreter starts in `src/knight.c` and branches from there. All C files using dynasm should be placed in the `src/jit/` directory. All header files should be placed in the same directory of their parent C file, or whichever directory seems fit if the header has no corresponding C file. Headers in `src/jit/` should be focused on low-level value operations (e.g. `value.h` fits because it deals with creating and coercing values).

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
├── tests/           # Test suite
│   └── ...          # Various test files
├── target/          # Build output directory
│   ├── artifacts/   # Build artifacts
│   ├── luajit/      # Dependencies from LuaJIT
│   └── knight       # Compiled binary
├── Makefile
├── README.md        # Project documentation
└── LICENSE.md       # License file
```