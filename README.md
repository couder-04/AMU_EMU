# IITPSx Assembler and Emulator

A two-pass assembler (`asm`) and a cycle-accurate emulator (`emu`) for the
IITPSx processor, written in portable C++11 with no dependencies beyond the
standard library. Built for the CS2102 IITPSx Processor mini-project.

```sh
make                      # builds ./asm and ./emu
./asm bub.asm             # -> bub.o (binary) and bub.lst (bit-string listing)
./emu -trace -dump bub.o  # run with per-instruction trace and a memory dump
```

**Contents**

- [Features](#features)
- [Build](#build)
- [Quick start](#quick-start)
- [The IITPSx machine](#the-iitpsx-machine)
- [Instruction set](#instruction-set)
- [Assembly language](#assembly-language)
- [Instruction encoding](#instruction-encoding)
- [Assembler reference](#assembler-reference)
- [Emulator reference](#emulator-reference)
- [Tests and examples](#tests-and-examples)
- [Project layout](#project-layout)

## Features

- **Two-pass assembler** with labels, forward references, PC-relative branches
  and calls, and the `data` and `SET` directives.
- **Clear diagnostics.** Every error carries a `file:line:` prefix. Warnings
  flag unused labels and programs with no `halt`. Nothing is written if any
  error is found.
- **Two output formats.** A little-endian binary object file (`.o`) and a
  human-readable listing (`.lst`) with one 32-bit word per line. The emulator
  loads either one.
- **Emulator with tracing, memory dumps, and a disassembler.** Memory size and
  instruction limit are configurable, so runaway programs stop cleanly.
- **Runtime checks** for out-of-range PC, out-of-range memory access, unknown
  opcodes, division by zero, and instruction-limit exhaustion. Each produces a
  distinct message, the register state, and exit code `2`.
- **Extended ISA.** Implements the 19 standard instructions plus `mul` and
  signed `div`.
- **Ten test programs** in [`Tests/`](Tests/) with recorded assembler and
  emulator output.

## Build

Requirements: a C++11 compiler and `make`.

```sh
make
```

This produces `asm` and `emu` in the project root. The default flags are
`-std=c++11 -pedantic -Wall -Wextra`. Override the compiler or flags as needed:

```sh
make CXX=clang++ CXXFLAGS="-std=c++11 -O2 -Wall"
```

## Quick start

Assemble and run the bubble-sort example that sorts five values in place:

```console
$ ./asm bub.asm
Assembly OK: 72 word(s), 0 warning(s). Wrote bub.o and bub.lst

$ ./emu -dump bub.o
HALT reached after 484 instruction(s)
Registers: A=0 (0x00000000)  B=4 (0x00000004)  PC=67  SP=4090
Memory dump (address: words)
00000000: 00100000 0000000B FFFFFA0A 00000500
...
00000044: 00000002 00000003 00000004 00000005
Non-zero words outside the loaded image:
00000FFA: 00000004 (4)
...
```

The last row of the dump is the `values` array, now sorted `1 2 3 4 5`
(the `1` is the final word of the previous row). The words at `0xFFA`
onward are the stack frame the program built below `SP = 0x1000`.

To watch each instruction execute, add `-trace`:

```console
$ ./asm Tests/test08.asm && ./emu -trace Tests/test08.o
00000000: ldc    6    [A=0 B=0 SP=65536]
00000001: ldc    7    [A=6 B=0 SP=65536]
00000002: mul       [A=7 B=6 SP=65536]
00000003: ldc    2    [A=42 B=6 SP=65536]
00000004: div       [A=2 B=42 SP=65536]
00000005: halt      [A=21 B=42 SP=65536]
HALT reached after 6 instruction(s)
Registers: A=21 (0x00000015)  B=42 (0x0000002A)  PC=6  SP=65536
```

Each trace line shows the register state *before* the instruction runs.

## The IITPSx machine

IITPSx is a tiny accumulator machine with a two-entry register stack.

| Register | Role |
| --- | --- |
| `A` | Top of the register stack. Almost every instruction reads or writes `A`. |
| `B` | Second register. Loading a value into `A` pushes the old `A` into `B`. |
| `SP` | Stack pointer. Starts at the memory size (one past the last word) and grows downward. `ldl`/`stl` address memory relative to it. |
| `PC` | Program counter. Incremented *before* an instruction executes, so branch offsets are relative to the next instruction. |

- Memory is an array of 32-bit words, addressed by word (default 65536 words).
  There are no bytes.
- Program execution starts at address `0`. Code and data share one address
  space; a `data` word placed in the path of execution is decoded as an
  instruction.
- Binary operations (`add`, `sub`, `shl`, `shr`, `mul`, `div`) compute
  `B op A`, store the result in `A`, and leave `B` unchanged.
- Arithmetic is 32-bit two's complement and wraps on overflow.

## Instruction set

`v` is a signed 24-bit operand. `off` is a signed 24-bit PC-relative offset
that the assembler computes for you when you write a label.

| Opcode | Mnemonic | Operand | Effect |
| ---: | --- | --- | --- |
| 0 | `ldc` | `v` | `B = A; A = v` |
| 1 | `adc` | `v` | `A = A + v` |
| 2 | `ldl` | `v` | `B = A; A = mem[SP + v]` |
| 3 | `stl` | `v` | `mem[SP + v] = A; A = B` |
| 4 | `ldnl` | `v` | `A = mem[A + v]` |
| 5 | `stnl` | `v` | `mem[A + v] = B` |
| 6 | `add` | | `A = B + A` |
| 7 | `sub` | | `A = B - A` |
| 8 | `shl` | | `A = B << A` |
| 9 | `shr` | | `A = B >> A` (arithmetic, sign-filling) |
| 10 | `adj` | `v` | `SP = SP + v` |
| 11 | `a2sp` | | `SP = A; A = B` |
| 12 | `sp2a` | | `B = A; A = SP` |
| 13 | `call` | `off` | `B = A; A = PC; PC = PC + off` (A holds the return address) |
| 14 | `return` | | `PC = A; A = B` |
| 15 | `brz` | `off` | `if (A == 0) PC = PC + off` |
| 16 | `brlz` | `off` | `if (A < 0) PC = PC + off` |
| 17 | `br` | `off` | `PC = PC + off` |
| 18 | `halt` | | Stop. Prints the instruction count and registers. |
| 19 | `mul` | | `A = B * A` |
| 20 | `div` | | `A = B / A` (signed, truncates toward zero; `A == 0` is a runtime error) |

Shift counts outside `0..31` do not wrap: `shl` yields `0`, and `shr` yields
`0` for a non-negative `B` or `-1` for a negative `B`.

`./emu -isa` prints this table from the emulator itself.

### Common idioms

```asm
        ldc 0x1000      ; set up a stack: SP = 0x1000
        a2sp
        adj -3          ; reserve three local words at SP+0, SP+1, SP+2

        ldc 7           ; local[0] = 7
        stl 0
        ldl 0           ; A = local[0]

        ldc arr         ; A = &arr[i]   (arr is a label, so its value is an address)
        ldl 1           ;   (i is in local[1]; ldl pushes &arr into B)
        add
        ldnl 0          ; A = arr[i]

        call fn         ; A = return address, jump to fn
        ...
fn:     ...             ; body must keep the return address in A (or save it with stl)
        return
```

## Assembly language

- One statement per line: `[label:] [mnemonic [operand]] [; comment]`.
- Mnemonics are case-insensitive. `HALT` and `halt` are the same.
- A **label** is a letter followed by letters or digits, ending in `:`. It may
  stand alone on its line, in which case it names the next emitted word.
  Labels are case-sensitive. One label per line.
- **Numeric operands** may be decimal (`10`), hexadecimal (`0x1F`), or octal
  with a leading zero (`017`). A leading `+` or `-` is allowed.
- **Label operands** evaluate to the label's address. For `br`, `brz`, `brlz`,
  and `call`, the assembler converts the address to an offset relative to the
  following instruction. A numeric operand to a branch is used as a raw offset.
- `data v` emits the 32-bit word `v` as-is. Accepts `-2^31 .. 2^32 - 1`.
- `name: SET v` gives `name` the constant value `v` without emitting a word.
  It needs a label on the same line and a numeric value, not another label.
- Instruction operands must fit in a signed 24-bit field,
  `-8388608 .. 8388607`.

## Instruction encoding

Every instruction is one 32-bit word. The opcode sits in the low 8 bits and
the operand in the upper 24 bits, stored as a signed value.

```text
 31                                    8 7              0
+---------------------------------------+----------------+
|          operand (24 bits, signed)     | opcode (8 bits)|
+---------------------------------------+----------------+
```

Examples:

| Source | Word | Why |
| --- | --- | --- |
| `ldc 5` | `0x00000500` | operand `5` in bits 31..8, opcode `0` |
| `adj -6` | `0xFFFFFA0A` | operand `-6` (`0xFFFFFA`), opcode `10` (`0x0A`) |
| `br -45` | `0xFFFFD311` | operand `-45`, opcode `17` (`0x11`) |
| `data 70` | `0x00000046` | the whole word is the value |

Object files store words in little-endian byte order. Listing files store the
same words as 32-character strings of `0` and `1`, one per line, in address
order.

## Assembler reference

```text
./asm program.asm
./asm -version
```

For `program.asm`, the assembler writes `program.o` and `program.lst` next to
the source. Exit code `0` on success and `1` if the file cannot be opened or
any error is reported. On error, no output files are written.

Diagnostics go to standard error. The assembler reports:

| Message | Cause |
| --- | --- |
| `duplicate label 'x'` | A label defined twice. |
| `no such label 'x'` | An operand names a label that is never defined. |
| `bogus label name 'x'` | Label does not match `[A-Za-z][A-Za-z0-9]*`. |
| `only one label is allowed per line` | Two `name:` prefixes on one line. |
| `unknown mnemonic 'x'` | Not an instruction or directive. |
| `invalid number 'x'` / `invalid operand 'x'` | Operand is not a number or a valid label. |
| `missing operand for 'x'` / `unexpected operand for 'x'` | Operand count does not match the mnemonic. |
| `extra text at end of line` | More than one operand. |
| `operand out of range: n` | Value does not fit the 24-bit field (or 32 bits for `data`/`SET`). |
| `SET requires a label on the same line` / `SET needs a numeric value` | Misused `SET`. |
| `WARNING: label 'x' defined but never used` | Unreferenced label. |
| `WARNING: program contains no HALT instruction` | The program can only stop via a runtime error or the instruction limit. |

See [`Tests/test02.log`](Tests/test02.log) for a run that exercises most of
these.

## Emulator reference

```text
./emu [options] program.o|program.lst
```

The input format is chosen by extension: `.lst` is parsed as binary text,
anything else is read as a little-endian binary object file.

| Option | Description |
| --- | --- |
| `-trace` | Print each instruction with the register state before it executes. |
| `-before` | Dump the loaded image before execution. |
| `-dump` | Dump memory after execution, including any non-zero words outside the loaded image (the stack, stored results, and so on). |
| `-disasm` | Disassemble the loaded image and exit without running it. |
| `-isa` | Print the instruction set and exit. |
| `-mem N` | Memory size in 32-bit words. Default `65536`. Also sets the initial `SP`. |
| `-limit N` | Maximum number of instructions to execute. Default `10000000`. |
| `-version` | Print the emulator version and exit. |

`N` accepts decimal, `0x` hex, or leading-zero octal.

**Exit codes**

| Code | Meaning |
| ---: | --- |
| `0` | The program reached `halt`. |
| `1` | Usage error, or the file could not be opened or parsed. |
| `2` | Runtime error. The message, instruction count, and registers are printed. |

**Runtime errors**

- `PC out of range (errant program)`
- `ldl`/`stl`/`ldnl`/`stnl` `address out of range`
- `unknown opcode N at PC=P` (typically execution ran into a `data` word)
- `division by zero`
- `instruction limit exceeded (probable infinite loop)`

Use `-limit` whenever you run a program that might not terminate:

```sh
./emu -limit 30 -dump Tests/test01.o
```

## Tests and examples

Every `Tests/testNN.asm` has a matching `.log` recording the exact assembler
and emulator commands and their output. Tests that assemble successfully also
include their `.o` and `.lst`. Tests that are supposed to fail have a short
`.lst` explaining why no listing exists.

| Test | What it exercises |
| --- | --- |
| `test01` | Valid syntax, labels, branches, `data`, and an infinite loop (run with `-limit`). |
| `test02` | Malformed input. Triggers nine distinct assembler errors. |
| `test03` | The `SET` directive and its use in execution. |
| `test04` | Recursive triangle-number computation with `call`/`return`. |
| `test05` | Sum of the integers 1 through 10. |
| `test06` | Unused-label warnings and another infinite loop. |
| `test07` | More malformed input, focused on `SET` and `data` diagnostics. |
| `test08` | `n*(n+1)/2` using `mul` and `div`. |
| `test09` | Division by zero caught by the emulator. |
| `test10` | Bubble sort of five values (same program as `bub.asm`). |

There is no automated test runner. To re-run a test and compare with its log:

```sh
make
./asm Tests/test04.asm
./emu -dump Tests/test04.o
diff <(./emu -dump Tests/test04.o) <(sed -n '/^Emulator command/,/^Emulator exit/p' Tests/test04.log | sed '1d;$d')
```

[`Tests/compile.txt`](Tests/compile.txt) lists the commands used to build and
run the examples.

## Project layout

```text
assembler/asm.cpp   Two-pass assembler (single file, ~400 lines)
emulator/emu.cpp    Emulator, disassembler, and ISA printer (~280 lines)
Makefile            Builds ./asm and ./emu
bub.asm             Bubble-sort example used in the quick start
Tests/              Test programs with recorded output
claims              Summary of implemented features for the project submission
```

## Design notes

- **Pass 1** of the assembler records label addresses and reports syntax
  errors. **Pass 2** generates code and reports undefined labels. Both passes
  share one line-processing routine; a flag controls which pass reports a
  given error so nothing is printed twice.
- Instruction definitions live in a single table of `{mnemonic, opcode,
  operand kind}`. Adding an instruction means adding one row there and one
  `case` in the emulator.
- The emulator fetches, bumps `PC`, then executes. This is why `call` leaves
  the address of the *next* instruction in `A`, and why the assembler computes
  branch offsets as `target - (PC + 1)`.
- `div` special-cases a divisor of `-1` so that `INT_MIN / -1` wraps instead
  of raising a hardware exception on the host.

## License

No license is specified. Add one before redistributing or accepting
contributions.
