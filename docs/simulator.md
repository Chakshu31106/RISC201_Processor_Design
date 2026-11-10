# Simulator (CPU + debugger)

## Running

```
build/bin/processor prog.asm            # assemble and open the debugger
build/bin/processor prog.hex            # load a hex file and open the debugger
build/bin/processor --run prog.asm      # run to completion, print registers
build/bin/processor --run --trace prog.asm
```

## Flow

```
.asm --assm201--> .hex --> InstructionMemory
                               |
     CPU::step():  fetch(ip) -> decode -> execute (ALU + RegisterFile) -> update ip
```

Every instruction runs through the microprogrammed control unit: the simulator fetches the instruction, hands it to the control unit, which decodes it and runs its microinstructions on the CPU's registers, flags and data memory.
The pipeline is shown, not simulated: the `stage` debugger command walks the next
instruction through the stages one at a time (instructions do not overlap). Stage
names follow the project documents; pick a view with `pipeline 4|6`:

| View | Stages |
|---|---|
| 4-stage | IF (fetch), OF (decode + read operands), EX (ALU, address, branch), MA+RW (memory access then register write-back) |
| 6-stage | IF, ID (decode), RR (read registers), EX, MA (memory access), RW (register write-back) |

EX shows the result using a throw-away copy; the instruction really commits on the
last stage (MA+RW or RW). The browser simulator has the same 4/6-stage selector.

## CPU conventions

- Every instruction is 8 bytes. `ip` (r29) is a **byte address**.
- While an instruction executes, `ip` is the address of that instruction. Branch
  target = `ip + offset`.
- An instruction that writes r29 directly (for example `mov r29, 16`) makes that
  value the next `ip`, so it acts as a jump.
- When `ip` is aligned but past the last instruction, the CPU halts normally
  (`end of program`). A misaligned `ip` is an error.
- On an error the `ip` stays on the faulting instruction.
- `cmp` sets the flags in the register file; `beq` / `bgt` / `bst` read them.
- `mov` immediates are 39 bits wide but registers are 32 bits, so they are truncated.
- Decoded field names follow the encoder. For `st` and `vst`, `rd` is the **base**
  register and `rs1` is the **value** register.

## Memory model

- **Separate memories:** instruction memory (8-byte instructions) and data memory.
- **Data memory:** 64 KB (the control unit's size), big-endian, 4-byte words. A word
  access must be at an address divisible by 4; a misaligned or out-of-range
  `ld`/`st` is an **error** that stops the CPU.
- **Stack:** at the top of data memory, growing **down**. `sp`
  (r30) starts at 65536 (empty) and always points at the newest word.
  - `push r`: `sp -= 4; mem[sp] = r`   `pop r`: `r = mem[sp]; sp += 4`
  - `peek r`: `r = mem[sp]`   `mpop #n`: `sp += 4n`   `clr`: `sp = 65536`
  - Overflow (no room below `sp`) and underflow (too few words for `pop`, `peek`,
    `mpop` or `ret`) are errors with a clear message.
- **Calls:** `call label` pushes the caller's `ra`, then the return address
  (`ip + 8`), sets `ra` to the return address and jumps. `ret` pops the return
  address into `ip` and then restores the caller's `ra`. Nested calls need no manual
  saving; see `programs/examples/call_ret_test.asm`.
- `ld rd, off[base]`: `rd = mem[base + off]`.  `st rs, off[base]`: `mem[base + off] = rs`.

## What runs today

| Works | Stops with `not implemented` |
|---|---|
| add sub mul div mod pow, and or nor nand xor not, lsl lsr asr rotl rotr, pcnt tog sbit cbit, incr decr abs, cmp, b beq bgt bst, mov (mode 0), nop, ld st push pop peek mpop clr call ret, **fadd fsub fmul fdiv fcmp sqrt fabs sin cos** (registers hold raw IEEE-754 bit patterns; `fcmp` sets E/GT/ST) | mov modes 1/2. Vector instructions (vadd vsub vdot vcrs vmag vang vld vst) run through the control unit's microcode like every other instruction; `acos` is only reached through `vang` |

A valid instruction whose behaviour is not decided yet stops the CPU with the
`NotImplemented` reason and the instruction's address, instead of guessing.

Stack macros are done in the assembler: `pushm r1, r2, r3` becomes `push r1 / push r2 / push r3`, and `popm r1, r2, r3` becomes `pop r3 / pop r2 / pop r1` (reverse order, so it undoes `pushm`). `push r1, r2` and `pop r1, r2` are also accepted and expand in the order written. `not` takes `not rd, rs1` (the one-operand form `not rd` still works). Still open: error-handling details, floating-point details (NaN/infinity/rounding rules), error-handling details beyond the stop-with-message default. `div` and `mod` by zero
currently return 0 (ignored for now).

## Debugger commands

| Command | Action |
|---|---|
| `load <file>` | load a `.asm` (assembled) or `.hex` file |
| `step [n]`, `s` | execute n instructions (default 1) |
| `run [max]`, `r`, `continue`, `c` | run until halt, breakpoint, or step limit (default 1,000,000) |
| `stage`, `sg` | advance one stage of the next instruction |
| `pipeline 4\|6` | choose the 4-stage or 6-stage view |
| `perf [fwd]` | pipeline statistics for the instructions executed so far: stalls, flushes, CPI, execution time and the 4-stage vs 6-stage speedup. `perf fwd` assumes operand forwarding |
| `micro`, `upc` | preview the next instruction through the microprogrammed control unit, one microinstruction at a time (uPC, type, fields, internal registers changed). A preview on a private copy; nothing is executed. Every opcode has microcode. A vector operand is written as the first register of the vector: r20 (v1 = r20-r22), r23 (v2 = r23-r25) or r26 (v3 = r26-r28); the assembler encodes them as the vector ids 0, 1, 2 that the microcode expects, and the simulator reports any other register used as a vector operand as an error |
| `stack` | draw the stack (starts at the top of data memory, grows down; newest word on top) |
| `mem <addr> [n]` | show n data-memory words (addr multiple of 4) |
| `fmt int\|float\|both` | how registers, memory, the stack and the trace are shown. Every register and memory word is a raw 32-bit pattern: integer instructions read it as an int, floating-point and vector instructions read the same bits as an IEEE-754 float. Default is `both`. The same setting is available as `--fmt` on the command line |
| `regs` / `reg <n>` | show all registers and flags / one register |
| `list [n]`, `l` | disassemble n instructions from `ip`; `=>` marks `ip`, `*` marks breakpoints |
| `break <addr>` / `unbreak <addr>` / `breaks` | set, remove, list breakpoints (byte address, decimal or 0x hex) |
| `trace on` / `trace off` | print each executed instruction and the registers it changed |
| `reset` | reset registers and `ip` (program and breakpoints are kept) |
| `help`, `quit` | |

Resuming with `run` from a breakpoint steps past it first, so it does not stop on
the same instruction again.


## Pipeline statistics (`perf`, `--perf`)

The pipelines are not emulated. Instead the real execution trace (each instruction's source and destination
registers, loads, and whether a branch was taken) is fed to the timing model from `src/Simulator/isa_pipeline_sim.cpp`
(also implemented in `pipeline_model.cpp`). The model counts, for the 4-stage (IF OF EX MA+RW, 4 ns clock) and 6-stage
(IF ID RR EX MA RW, 3 ns clock) pipelines:

* data-hazard stall cycles: without forwarding the operand-read stage waits for the producer's write-back; with forwarding EX waits for the ALU result (or for MA after a load);
* branch flushes: static predict-not-taken, resolved in EX; a taken `beq/bgt/bst`, `b`, `call` and `ret` flush the instructions fetched before EX;
* cycles, CPI, execution time (cycles x clock period) and the speedup of the 6-stage over the 4-stage pipeline.

```
./processor --run --perf program.asm          # table printed after the run
./processor --run --perf --fwd program.asm    # same, with forwarding
./processor --run --fmt float program.asm     # show register values as floats (int | float | both)
risc201> perf        (or: perf fwd)           # inside the debugger, after run / step
```

On programs without stack or call instructions the table is identical to `isa_sim`
(`./isa_sim program.asm [-f]`, built from the same file; it has its own parser and syntax, so it only understands the
simple form of each instruction). The model also treats `push`, `pop`, `call` and `ret` as reading and writing `sp` (and `ra`), because they do.

## Floating-point values

A literal with a decimal point is a single-precision float. The assembler stores its 32-bit IEEE-754 pattern,
so `mov r1, #1.5` loads `0x3FC00000`, and `fadd`, `fmul`, `sqrt` and the vector instructions read it as 1.5.
`programs/examples/float_literal_test.asm` shows this; run it with `regs` or `--fmt both` to see both views.
