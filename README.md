# BitForge

**An 8-Bit Computer Emulator, Assembler, and Interactive Debugging Workbench**

BitForge is an educational software model of a fictional 8-bit microprocessor and workbench environment. It is designed to demonstrate computer architecture and algorithm & data structures (ADS) concepts—such as Bubble Sort, Linear Search, and recursion—through deterministic simulation and interactive inspection.

---

## Current Status: Phase 4 Complete (CLI Assembler & Execution Runner)

BitForge is developed incrementally in strict adherence to [BitForge Architecture Specification v1.7](BitForge_Architecture_Specification_v1.7.md).

Implemented capabilities include:
- **Build Infrastructure:** Portable CMake 3.20+ build system with C++17 enforcement, compiler warning configurations, unified `CMakePresets.json`, and multi-platform CI workflow.
- **Architectural Types & Constants:** 8-bit/16-bit fixed-width types, big-endian arithmetic helpers, and all 31 canonical instruction opcodes.
- **Memory Model:** Flat 64 KiB byte-addressable memory with big-endian 16-bit read/write, safe 16-bit boundary wrapping, and segment loading bounds enforcement.
- **Pure Functional ALU:** Stateless arithmetic and logic unit computing `ADD`, `SUB`, `CMP`, `INC`, `DEC`, `AND`, `OR`, `XOR`, and `NOT`, with exact flag calculation (`Z`, `C`, `N`, `V`).
- **CPU Execution Engine:** Full 31-mnemonic fetch-decode-execute engine with atomic state commitment, state machine (`Ready`, `Running`, `Paused`, `Halted`, `Faulted`), and architectural fault records.
- **Stack, Subroutines & Virtual I/O:** Downward-growing stack (`SP = 0xFFFF` empty sentinel), big-endian `CALL`/`RET` return addresses, virtual FIFO input (Port 0x00), text console (Port 0x01), numeric stream (Port 0x02), and 8-bit LED register (Port 0x10).
- **Two-Pass Assembler:** Lexer, parser, symbol table, expression evaluation, multi-segment program image generation, explicit entry-point validation, and source diagnostics.
- **Headless CLI Executable (`bitforge`):** Command-line assembler and execution runner supporting `help`, `assemble`, and `run` subcommands with execution step budgeting, register dumping, virtual I/O presentation, and stable numeric exit codes.
- **Automated Test Suite:** 136 passing tests covering units, edge cases, integration scenarios, and CLI operations in both Debug and strict CI mode (`-DBITFORGE_WARNINGS_AS_ERRORS=ON`).

---

## Architectural Highlights

- **Data & Address Width:** 8-bit data, 16-bit address space (`0x0000`–`0xFFFF`, 65,536 bytes).
- **Registers:** Four 8-bit general-purpose registers (`R0`–`R3`), 16-bit index register (`IX`), 16-bit program counter (`PC`), and 16-bit stack pointer (`SP`).
- **Flags:** Zero (`Z`), Carry/no-borrow (`C`), Negative (`N`), and Signed Overflow (`V`).
- **Instruction Set:** Exactly 31 canonical instruction mnemonics across 4 functional groups.
- **Endianness:** Strictly big-endian across 16-bit addresses and immediate words.
- **Dependencies:** Strictly standard ISO C++17. Zero third-party library dependencies for core, assembler, CLI, and tests.

---

## Command-Line Interface (`bitforge`)

### Usage

```bash
bitforge help | -h | --help
bitforge assemble <source.asm>
bitforge run <source.asm> [--max-steps N] [--dump-regs]
```

### Commands

| Command | Description |
|---|---|
| `help` | Display command-line usage, options, and exit codes. |
| `assemble <source.asm>` | Assemble an assembly source file and display entry point and emitted segment layout. |
| `run <source.asm>` | Assemble, load into emulated memory, and execute the program on the virtual CPU. |

### Options for `run`

| Option | Default | Description |
|---|---|---|
| `--max-steps <N>` | `1000000` | Safety execution budget. Terminate if CPU steps exceed `<N>` instructions. |
| `--dump-regs` | Disabled | Output architectural register and flag state upon termination. |

### Exit Codes

| Code | Meaning | Description |
|---|---|---|
| `0` | Success | Program halted normally via `HALT` or assembly succeeded. |
| `1` | Usage Error | Invalid command-line arguments, unknown options, or missing file arguments. |
| `2` | Assembly / File Error | Source file cannot be opened or assembly diagnostics encountered. |
| `3` | CPU Fault | Architectural execution fault (e.g., `InvalidOpcode`, `StackUnderflow`). |
| `4` | Budget Exceeded | Execution exceeded the step limit before reaching `HALT` or faulting. |

### Examples

#### Assembling a Program
```bash
bitforge assemble examples/01_arithmetic.asm
```
Output:
```text
Assembly successful: examples/01_arithmetic.asm
Entry point:  0x0200
Total bytes:  14
Segments (1):
  [0] Start: 0x0200, Size: 14 bytes (0x000E)
```

#### Executing with Register Dump
```bash
bitforge run examples/01_arithmetic.asm --dump-regs
```
Output:
```text
==================================================
BitForge Register Dump
==================================================
State:          Halted
Stop Reason:    Halted
Steps Executed: 5
PC: 0x020E   SP: 0xFFFF   IX: 0x0000
R0: 0x1E     R1: 0x14     R2: 0x00     R3: 0x00
Flags: [Z=0, C=0, N=0, V=0]
LEDs:  0x00 (0b00000000)
==================================================
```

#### Virtual I/O Demonstration
```bash
bitforge run examples/02_io_demo.asm
```
Output:
```text
--- Text Output (Port 0x01) ---
HI!
--- Numeric Output (Port 0x02) ---
42
LED State (Port 0x10): 0x55 (0b01010101)
```

---

## Quickstart

### Prerequisites
A modern C++17 compliant compiler and CMake 3.20+:
- **Windows:** MSVC (Visual Studio 2019/2022) or MinGW-w64 (GCC 9.3+ / Clang 10+).
- **Linux:** GCC 9.3+ or Clang 10+.
- **macOS:** Apple Clang (Xcode 12+) on Apple Silicon or Intel.

For detailed platform setup and toolchain installation commands, see [BUILDING.md](BUILDING.md).

### Building with CMake Presets (Recommended)

```bash
# Configure debug build
cmake --preset debug

# Compile bitforge executable and unit test suite
cmake --build --preset debug

# Run automated tests via CTest
ctest --preset debug
```

### Building Manually (Single-Config / Ninja or Make)

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Test
ctest --test-dir build --output-on-failure
```

---

## Specification History

Historical architectural specifications are preserved at the repository root:
- [v1.0](BitForge_Architecture_Specification_v1.0.md) — Initial architecture specification.
- [v1.1](BitForge_Architecture_Specification_v1.1.md) — Clarified opcode counts, registers, and fault contracts.
- [v1.2](BitForge_Architecture_Specification_v1.2.md) — Added cross-platform and CI engineering guidelines.
- [v1.3](BitForge_Architecture_Specification_v1.3.md) — Refined runner configurations and compiler version requirements.
- [v1.4](BitForge_Architecture_Specification_v1.4.md) — Canonical 31-instruction audit and macOS runner selection.
- [v1.5](BitForge_Architecture_Specification_v1.5.md) — Full CI workflow specification and runner lifecycles.
- [v1.6](BitForge_Architecture_Specification_v1.6.md) — Linear search memory contract and ADS benchmark budgets.
- [v1.7](BitForge_Architecture_Specification_v1.7.md) — **Active Architectural Baseline**.
