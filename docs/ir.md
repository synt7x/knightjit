# IR

One of the most important aspects of the KnightJIT v2 implementation is the structure of its intermediate representation (`ir`). KnightJIT v2 borrows some ideas from the earlier v1 implementation, but restructures some of the inherently naive aspects.

Before we begin the details of the IR, we must first discuss what it is trying to be, and what it is not. The KnightJIT v2 intermediate representation is intended to:

1. Serve as the intermediary between the semantics of Knight and its execution model.
2. Model the flow of data through the program by expanding on the information provided by the AST.
3. Provide a base for optimizations and analysis to be performed, being a valid mapping both back to the AST and to generated machine code.
4. Represent the execution of Knight in a way that adheres to the performance and execution model of native code.

However, this intermediate representation is not intended to:

1. Map **1 to 1** with the target machine code structures.
2. Map **1 to 1** with Knight's structures.
3. **Maintain coherence across versions**; this IR is volatile, and should not be a target for *your* interpreter! (Forks are welcome though :), maybe I'll consider making an `ir -> jit` library in the future!)

With all that out of the way, lets describe the

## Structure

Because all Knight operations are n-ary, and we can *mostly* map operations to a single IR instruction, we can say that all IR instructions will have an arity of `4`. This is because the operation with the highest arity of all Knight operations is `SET`, with 4 operands. However, set can be simplified to `SET arr idx value`

The structure of the IR instructions looks like this so far:

```cpp
struct instruction {
    uint32_t i1,
    uint32_t i2,
    uint32_t i3,
}
```

Since we obviously would like to distinguish between different instructions by the operation, we will create an `opcode` enum (with further information in the [instructions section](#instructions)).

```cpp
struct instruction {
    opcode op,

    uint32_t i1,
    uint32_t i2,
    uint32_t i3,
}
```

It would be preferable to minimize the size of our instructions to 64 bits, such as by using `uint16_t` instead of `uint32_t` However, this limits the amount of total instructions we can have in a program to ~32k, which is a lot, but low enough that some large programs may exhaust that space.

To counteract this problem, we will have `i1` be an anchor value and `i2`/`i3` will be offset from `i1`.

```cpp
struct instruction {
    opcode op,

    uint24_t i1,
    uint16_t i2,
    uint16_t i3,
}
```

What happens when we have more instructions than even 24 bits can hold? We will need an extended form of instruction, with some flag to specify.

```cpp
struct instruction {
    opcode op,
    flags flag,

    int24_t i1,
    int16_t i2,
    int16_t i3,
}
```

Since we can reasonably fit all instructions inside of 6 bits, we will have our `op` take up 6 bits in our instruction. This leaves us with 2 bits for the `flags`.

Let's make the flags determine if the instruction is `compact`, `extended`, an `immediate integer`, or a `referenced immediate`.

### Compact
`O` is the opcode. `A` is the anchor, referencing a specific instruction offset from the current instruction. `B` and `C` are offsets from the anchor. If `A`, `B`, or `C` overflow, all should be placed in the extended instruction table and the instruction should become an extended instruction.
```
[0b00-OOOOOO-AAAAAAAAAAAAAAAAAAAAAAAA-BBBBBBBBBBBBBBBB-CCCCCCCCCCCCCCCC]
```

### Extended
`O` is the opcode. `A`, `B`, and `C` are references into an extended instruction table, which maps far away instructions such as `extended_instructions[1] = 80293`. Note that the value in the table is not an offset, but the actual instruction id. These values go up to `uint32_t`. `R` are reserved bits.
```
[0b01-OOOOOO-AAAAAAAAAAAAAAAA-BBBBBBBBBBBBBBBB-CCCCCCCCCCCCCCCC-RRRRRRRR]
```

### Immediate Integer
`R` is a reserved bit
```
[0b10-R-XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX]
```