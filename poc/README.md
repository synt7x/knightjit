# Proof-of-Concept
This proof-of-concept demonstrates that the tracing, optimizing, and patching workflow is viable for high performance applications for the `x86_64` architecture.

This example is based around the following Knight pseudo-code:
```knight
; = list ...75,000 ints... ... 25,000 strings ...
; = i 0
; = result 0
WHILE > #list i
  ; = result + result GET list i 1
  = i + i 1
OUTPUT result
```

## What this is and is not
This proof-of-concept **is**:
- Showing the viability of tracking a machine-code-first runtime using instructions such as `inc [rX + instruction_offset + type/branch_offset]`.
- Patching machine code at runtime using i-cache invalidation and atomic writes to executable memory from a separate thread.
- Generating new stubs for additional, required deoptimization code.

This proof-of-concept **does not**:
- Generate dynamic machine code from source code; that process is already explored in [KnightJIT v1](https://github.com/synt7x/knightjit).
- Use SSA IR/Metadata to generate patched code; this is an implementation detail, not the focus of the PoC of the runtime architecture.

## Building

The only requirements for building this proof-of-concept is a C++ compiler (e.g. `clang++` or `g++`), and an installation of `git` and `make`. Once these prerequisites are installed, simply run `make` in this directory to build the poc binary as `target/poc`.
```sh
make
```