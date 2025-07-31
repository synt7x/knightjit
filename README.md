# KnightJIT

Knight implementation with an optimizing register based VM that supports[*](#Work-In-Progress) JIT compilation. Generally outperforms other interpreters while maintaining correctness.

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

## Contributing

Information regarding contributions can be found in the [CONTRIBUTING.md](https://github.com/synt7x/knightjit/blob/master/CONTRIBUTING.md). Feel free to open a pull request at any time!