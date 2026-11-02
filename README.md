# RISC201 Processor Design

ISA, two-pass assembler/disassembler, ALU, FPU, microprogrammed control unit and a
CPU simulator with an interactive debugger.

## Build and run

```
cmake -S . -B build
cmake --build build -j
ctest --test-dir build

build/bin/assembler programs/examples/factorial5_loop.asm out.hex
build/bin/disassembler out.hex
build/bin/processor programs/examples/factorial5_loop.asm      # debugger (type "help")
build/bin/processor --run --perf programs/examples/factorial.asm    # run + 4-/6-stage timing table (add --fwd for forwarding)
build/bin/isa_sim programs/examples/factorial.asm [-f]           # team's standalone timing tool (GCC/Clang only)
```

`docs/simulator.html` is a standalone in-browser simulator (open it in any browser).
`docs/simulator.md` describes the memory model, the stack, the debugger commands
and the 4-/6-stage views.

## Layout (src/)

| Folder | Contents |
|---|---|
| `ISA/` | register numbers, hex/bin program files |
| `Assembler/` | `risc201_common.h/.cpp`, assembler and disassembler tools |
| `ALU/alu/`, `ALU/fpu/` | ALU and FPU |
| `datapath/` | register file, data memory, instruction memory |
| `Control/` | control unit, microcode builder, `*_microcode/` folders, `tests/` |
| `Simulator/` | CPU, decoder, debugger, micro-mode executor, `main.cpp` |

Headers are included relative to `src/` (for example `"datapath/memory.h"`); the
control-unit files and tests use bare names resolved through `src/Control`, which
CMake puts on the include path.
