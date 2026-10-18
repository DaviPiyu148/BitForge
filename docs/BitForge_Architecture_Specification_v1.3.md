# BitForge Architecture Specification v1.3: Rigorous Systems Baseline

**Project:** BitForge — An 8-Bit Computer Emulator, Assembler and Interactive Debugging Workbench  
**Document status:** Approved architectural baseline  
**Primary language:** Standard C++17 (Strict ISO standard, extensions disabled)  
**Target platforms:** Windows 10/11, Modern Mainstream Linux, macOS (Intel x86_64 & Apple Silicon arm64)  
**Initial target:** Headless desktop core and CLI; GUI workbench deferred to Phase 6  
**Project context:** Individual Algorithm and Data Structures mini project  
**Predecessor documents:**
- `BitForge_Architecture_Specification_v1.0.md` (preserved unchanged)
- `BitForge_Architecture_Specification_v1.1.md` (preserved unchanged)
- `BitForge_Architecture_Specification_v1.2.md` (preserved unchanged)

---

## 1. Purpose and product vision

BitForge is an educational software model of a small fictional computer. Users write programs in BitForge Assembly, assemble them into machine code, run them on an emulated CPU, and inspect execution through a debugger and visual workbench.

The signature demonstration is a Bubble Sort program written in BitForge Assembly and executed by the emulated processor. The application enables an observer to monitor the current instruction, CPU registers and flags, memory mutations, stack activity, and execution trace.

BitForge is an instruction-set simulator. It is **not** intended to simulate transistor-level behavior, electrical timing, or a physical processor's exact microarchitecture. Its CPU semantics are deterministic, cross-platform, and fully documented.

### 1.1 Primary goals

1. Define a small, internally consistent 8-bit instruction set.
2. Execute assembled machine code accurately and deterministically across all supported host platforms.
3. Provide a two-pass assembler with labels, directives, and structured source diagnostics.
4. Support single-stepping, breakpoints, tracing, and CPU/memory inspection.
5. Demonstrate ADS concepts through programs that execute on the simulated CPU.
6. Test core behavior independently from the graphical interface.
7. Support clean-checkout collaboration, continuous integration, and seamless cross-platform building.

### 1.2 Explicit non-goals for v1.3

- Cycle-accurate or transistor-level simulation.
- A pipelined or out-of-order CPU.
- A commercial C/C++ compiler or general-purpose macro assembler.
- Operating system simulation, hardware interrupts, multitasking, or privilege rings.
- Multiple emulated CPU architectures.
- Immediate packaging of installers or GUI bundles prior to core stabilization.

---

## 2. High-level components and dependency boundaries

BitForge is structured into decoupled modules with strict dependency boundaries:

```
+-------------------------------------------------------------------+
|               GUI Workbench (Phase 6, Optional/Decoupled)          |
+-------------------------------------------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
|             Debugger API & Headless CLI Test Runner               |
+-------------------------------------------------------------------+
         |                                           |
         v                                           v
+-----------------------+                 +-------------------------+
|   Assembler (2-Pass)  |                 |         Cpu Core        |
| - Lexer / Parser      |                 | - Decode & Fetch Cursor |
| - Label Resolution    |                 | - Pre-execution Checks  |
| - ProgramImage Gen    |                 | - Atomic State Commit   |
+-----------------------+                 +-------------------------+
                                                       |
                                                       v
                                          +-------------------------+
                                          |   Machine Bus (IBus)    |
                                          +-------------------------+
                                            /                     \
                                           v                       v
                         +--------------------+   +-----------------------+
                         |   Memory (64KiB)   |   | Virtual Devices (I/O) |
                         +--------------------+   +-----------------------+
```

1. **CPU core:** Instruction decoding, pre-execution validation, atomic architectural state commitment, registers, flags, machine states, and faults.
2. **Memory:** 65,536 byte-addressable locations (flat 64 KiB) and program-image loading.
3. **Machine bus (`IBus`):** Narrow interface connecting CPU to memory and virtual port I/O with side-effect-free permission queries.
4. **Virtual devices:** Deterministic input FIFO queue, text console, numeric stream, and virtual LEDs.
5. **Assembler:** Source parsing, label resolution, instruction encoding, directives, and diagnostics.
6. **Debugger:** Step/run/pause/reset execution controller, breakpoints, trace history, and source mapping.
7. **CLI runner:** Headless executable for automated testing and batch execution.
8. **GUI workbench:** Visual assembly editor, register/flag monitors, memory viewer, stack viewer, LED display, and trace visualization (Phase 6).
9. **Automated test suite:** Unit tests, integration tests, and ADS algorithmic benchmarks driven by CTest.

The CPU core and assembler must have zero dependencies on Qt, OS GUI toolkits, or host terminal APIs.

---

## 3. Supported platforms and toolchains

### 3.1 Target operating systems vs CI-verified platforms

A rigorous distinction is maintained between intended design targets and platforms verified by automated CI builds:

| Operating System | Architectures | Target Status | CI Verification Job |
|---|---|---|---|
| **Windows 10/11** | x86_64 | Supported | `windows-latest` (MSVC 2022 + Ninja) |
| **Linux (Ubuntu)** | x86_64 | Supported | `ubuntu-latest` (GCC 11/13 + Ninja; Clang 14/16 + Ninja) |
| **Linux (Other distros)** | x86_64, aarch64 | Intended (Portable ISO C++17) | Unverified by default CI |
| **macOS (Apple Silicon)** | arm64 | Supported | `macos-14` / `macos-latest` (Apple Clang + Ninja) |
| **macOS (Intel)** | x86_64 | Supported | `macos-13` (Apple Clang + Ninja) |

Platform support claims are strictly gated by actual passing automated test runs on that specific OS/architecture configuration.

### 3.2 Supported compiler families and minimum versions

| Compiler Family | Minimum Version | Recommended Version | Standard Library |
|---|---|---|---|
| **GCC** | 9.3+ | 11.0+ / 14.0+ | `libstdc++` (GCC 9+ provides complete `<filesystem>`) |
| **LLVM Clang** | 10.0+ | 14.0+ / 18.0+ | `libc++` or `libstdc++` |
| **Apple Clang** | Xcode 12.0+ (Apple Clang 12.0+) | Xcode 14.0+ | Apple `libc++` |
| **MSVC** | Visual Studio 2019 (v16.10 / MSVC 19.28) | Visual Studio 2022 (v17.0+ / MSVC 19.30+) | Microsoft STL (`/std:c++17`, `/permissive-`) |

### 3.3 Build system requirements

- **CMake:** Minimum version **3.20** required. Version **3.25+** recommended for full preset schema v5 support.
- **Generators:** Ninja (single-config cross-platform), Visual Studio 16/17 (multi-config Windows), Unix Makefiles (single-config Linux/macOS), Xcode (multi-config macOS).

---

## 4. Portable implementation rules

### 4.1 Strict ISO C++17 standard
- C++17 is mandated across all targets.
- Extensions are explicitly disabled in CMake:
  ```cmake
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  ```
- No compiler-specific language extensions (`__attribute__`, `#pragma` extensions, `__declspec`) in core logic.

### 4.2 Cross-platform filesystem and path handling
- **No hardcoded paths or drive letters:** Never assume `C:\`, `/home`, or specific directory layouts.
- **Path abstraction:** Use standard `std::filesystem::path` for all file operations.
- **Path normalization:** Accept both forward slashes (`/`) and backslashes (`\`) gracefully.
- **Working directory independence:** Do not assume code is run from the project root or binary directory; use relative paths passed via command line or resolve paths relative to input files.

### 4.3 Deterministic text file and newline handling
- Source files are treated as UTF-8 encoded text.
- The assembler lexer must transparently handle and normalize:
  - Windows CRLF (`\r\n`)
  - Unix/macOS LF (`\n`)
  - Legacy CR (`\r`)
- Line numbers and column indices in error diagnostics must match the user's visual source lines regardless of host line ending.

### 4.4 Fixed-width integers and arithmetic guarantees
- All machine data, addresses, registers, and memory must use explicit types from `<cstdint>`:
  - `byte_t` = `uint8_t` (exactly 8 bits unsigned)
  - `word_t` = `uint16_t` (exactly 16 bits unsigned)
  - `sbyte_t` = `int8_t` (exactly 8 bits signed two's complement)
  - `sword_t` = `int16_t` (exactly 16 bits signed two's complement)
  - `reg_id_t` = `uint8_t`
  - `port_t` = `uint8_t`
- Never use naked `char`, `short`, `int`, or `long` for emulated machine storage, as their widths and signedness vary across host platforms and compilers.

### 4.5 Explicit big-endian byte ordering
- BitForge machine words and 16-bit addresses are strictly **big-endian**.
- All encoding and decoding must be explicit bit shifts:
  ```cpp
  // Decode big-endian 16-bit address
  word_t addr = (static_cast<word_t>(hi_byte) << 8) | static_cast<word_t>(lo_byte);

  // Encode big-endian 16-bit address
  byte_t hi_byte = static_cast<byte_t>((addr >> 8) & 0xFF);
  byte_t lo_byte = static_cast<byte_t>(addr & 0xFF);
  ```
- **Never** use pointer casts (`*(uint16_t*)&memory[addr]`) or `memcpy` to interpret machine words, as this produces host-endian dependent bugs between x86 (little-endian) and big-endian systems.

### 4.6 Avoidance of undefined and implementation-defined behavior
- Bit-shifts must not exceed operand width (shift count strictly `< 8` for 8-bit, `< 16` for 16-bit).
- Arithmetic overflow in the CPU model uses unsigned wrapping arithmetic modulo 256 or 65,536, preventing signed host integer overflow (which is undefined behavior in C++).
- Flag calculations use standard boolean logic on explicit bit positions.

### 4.7 Compiler warning flags and configuration
Warnings are conditioned per compiler family in CMake, with warnings-as-errors controlled via a dedicated build option:
- **CMake option:** `option(BITFORGE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)`
- **GCC / Clang:** `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor` (appends `-Werror` when enabled)
- **MSVC:** `/W4 /permissive-` (appends `/WX` when enabled)

### 4.8 Zero-overhead dependency minimization
- The core emulator, memory, bus, assembler, debugger, and CLI runner rely exclusively on the **C++17 Standard Library**.
- The unit testing harness is self-contained and dependency-free (`tests/test_harness.hpp`), requiring no external package managers.

---

## 5. Machine architecture and address wrapping

### 5.1 Data model

| Property | Definition |
|---|---|
| Data width | 8 bits (one byte) |
| Address width | 16 bits |
| Address space | `0x0000` through `0xFFFF` inclusive |
| Addressable memory | 65,536 bytes (64 KiB) |
| General registers | Four 8-bit registers: `R0`, `R1`, `R2`, `R3` |
| Index register | One 16-bit register: `IX` |
| Program counter | 16-bit `PC` |
| Stack pointer | 16-bit `SP` |
| Status flags | `Z`, `C`, `N`, `V` |
| Instruction execution model | One complete instruction per step |
| Interrupts | Not implemented in v1.3 |

### 5.2 Registers

- **R0–R3:** General-purpose 8-bit registers.
- **IX:** 16-bit index register for indirect byte-array access. `LDI16`, `LDX`, `STX`, `INX`, and `DEX` operate on it.
- **PC:** Address of the next instruction to fetch between completed instructions.
- **SP:** Stack pointer. Points to the next free stack location; stack grows toward lower addresses.
- **Z (Zero):** Set when the operation's 8-bit result is zero.
- **C (Carry/no-borrow):** Set to carry-out for addition; set when subtraction/comparison requires no unsigned borrow (`left >= right`).
- **N (Negative):** Copies bit 7 of the operation's 8-bit result.
- **V (Signed overflow):** Set when signed two's-complement arithmetic overflows the range `-128..127`.

### 5.3 Initial/reset state

On a full machine reset:
- `R0..R3 = 0`
- `IX = 0`
- `PC = program entry address` (from `.ENTRY`, or default `0x0000`)
- `SP = 0xFFFF` (empty-stack sentinel / next free byte)
- `Z = C = N = V = 0`
- machine state is `Ready`
- trace, console output, LED state, and fault state are cleared
- memory is initialized to zero before the assembled image is loaded

### 5.4 Address wraparound rules

The 16-bit address space wraps strictly modulo 65,536:
1. **Sequential PC advancement:** Advancing `PC` wraps from `0xFFFF` to `0x0000`: `PC = static_cast<word_t>((PC + 1) & 0xFFFF)`.
2. **Multi-byte instruction fetch across boundary:** If an instruction begins at `0xFFFE` or `0xFFFF`:
   - Opcode byte fetched from `PC`.
   - Operand bytes fetched at `(PC + 1) & 0xFFFF`, `(PC + 2) & 0xFFFF`, wrapping safely into low memory without out-of-bounds array access.
3. **Index register arithmetic:** `INX` wraps `0xFFFF -> 0x0000`; `DEX` wraps `0x0000 -> 0xFFFF`.
4. **Data memory reads and writes:** `LD`, `ST`, `LDX`, `STX` access `memory[addr]` where `addr` is an unsigned 16-bit integer `0x0000..0xFFFF`. Out-of-bounds indexing is mathematically impossible because the storage array has exactly 65,536 entries.
5. **Stack boundary exemption:** The only addresses exempted from cyclic wraparound are stack push/pop/call/ret operations, which trigger `StackOverflow` or `StackUnderflow` upon reaching reserved stack bounds rather than wrapping.

### 5.5 Memory conventions

| Address range | Convention |
|---|---|
| `0x0000–0x7FFF` | Program code and small constants |
| `0x8000–0xEFFF` | Static data and arrays |
| `0xF000–0xFFFE` | Reserved stack storage |
| `0xFFFF` | Stack-pointer empty sentinel; not available to assembler-emitted program data |

Assembler-emitted bytes at `0xF000` or above are rejected to prevent normal program images from overwriting the stack region. Runtime CPU operations address the flat 16-bit space without memory protection.

### 5.6 Stack conventions and pre-validation

The stack grows toward lower addresses within `0xF000`–`0xFFFE`. `0xFFFF` is the empty sentinel. All stack operations must validate boundaries **before** committing state:

1. **`PUSH Rs`:** Requires `SP > 0xF000`. If `SP <= 0xF000`, immediate `StackOverflow` fault. When valid: decrement `SP`, store byte at `memory[SP]`.
2. **`POP Rd`:** Requires `SP < 0xFFFF`. If `SP >= 0xFFFF`, immediate `StackUnderflow` fault. When valid: load byte from `memory[SP]`, increment `SP`.
3. **`CALL addr16`:** Requires `SP >= 0xF002`. If `SP < 0xF002`, immediate `StackOverflow` fault. When valid: store `ret_hi` at `memory[SP - 2]`, `ret_lo` at `memory[SP - 1]`, `SP = SP - 2`, `PC = addr16`.
4. **`RET`:** Requires `SP <= 0xFFFD`. If `SP > 0xFFFD`, immediate `StackUnderflow` fault. When valid: pop `ret_hi` from `[SP]`, increment `SP`, pop `ret_lo` from `[SP]`, increment `SP`, `PC = (ret_hi << 8) | ret_lo`.
5. **Atomicity:** Any stack fault leaves `SP`, `PC`, registers, flags, and memory untouched.

---

## 6. Instruction set and encoding

### 6.1 Opcode table

| Opcode | Mnemonic and assembly form | Length | Operand-byte encoding | Purpose |
|---:|---|---:|---|---|
| `0x00` | `NOP` | 1 | — | No operation |
| `0x01` | `HALT` | 1 | — | Stop execution normally |
| `0x10` | `LDI Rd, imm8` | 3 | `reg, imm8` | Load an 8-bit constant into a register |
| `0x11` | `MOV Rd, Rs` | 3 | `dst, src` | Copy one register to another |
| `0x12` | `LD Rd, [addr16]` | 4 | `reg, addr_hi, addr_lo` | Load a byte from absolute memory |
| `0x13` | `ST [addr16], Rs` | 4 | `addr_hi, addr_lo, reg` | Store a byte to absolute memory |
| `0x14` | `LDI16 IX, addr16` | 3 | `addr_hi, addr_lo` | Load an address into `IX` |
| `0x15` | `LDX Rd, [IX]` | 2 | `reg` | Load a byte from memory addressed by `IX` |
| `0x16` | `STX [IX], Rs` | 2 | `reg` | Store a byte to memory addressed by `IX` |
| `0x17` | `INX` | 1 | — | Increment `IX` modulo 65,536 |
| `0x18` | `DEX` | 1 | — | Decrement `IX` modulo 65,536 |
| `0x20` | `ADD Rd, Rs` | 3 | `dst, src` | Add source to destination register |
| `0x21` | `SUB Rd, Rs` | 3 | `dst, src` | Subtract source from destination register |
| `0x22` | `INC Rd` | 2 | `reg` | Increment an 8-bit register |
| `0x23` | `DEC Rd` | 2 | `reg` | Decrement an 8-bit register |
| `0x30` | `AND Rd, Rs` | 3 | `dst, src` | Bitwise AND |
| `0x31` | `OR Rd, Rs` | 3 | `dst, src` | Bitwise OR |
| `0x32` | `XOR Rd, Rs` | 3 | `dst, src` | Bitwise XOR |
| `0x33` | `NOT Rd` | 2 | `reg` | Bitwise complement |
| `0x40` | `CMP Ra, Rb` | 3 | `left, right` | Set subtraction-comparison flags without storing a result |
| `0x50` | `JMP addr16` | 3 | `addr_hi, addr_lo` | Unconditional absolute jump |
| `0x51` | `JZ addr16` | 3 | `addr_hi, addr_lo` | Jump if `Z = 1` |
| `0x52` | `JNZ addr16` | 3 | `addr_hi, addr_lo` | Jump if `Z = 0` |
| `0x53` | `JC addr16` | 3 | `addr_hi, addr_lo` | Jump if `C = 1` |
| `0x54` | `JNC addr16` | 3 | `addr_hi, addr_lo` | Jump if `C = 0` |
| `0x60` | `CALL addr16` | 3 | `addr_hi, addr_lo` | Push return address and call absolute target |
| `0x61` | `RET` | 1 | — | Return using the stack |
| `0x62` | `PUSH Rs` | 2 | `reg` | Push one register byte |
| `0x63` | `POP Rd` | 2 | `reg` | Pop one byte into a register |
| `0x70` | `IN Rd, port8` | 3 | `reg, port` | Read a byte from a virtual input port |
| `0x71` | `OUT port8, Rs` | 3 | `port, reg` | Write a register byte to a virtual output port |

Register IDs: `R0 = 0`, `R1 = 1`, `R2 = 2`, `R3 = 3`. Any other ID triggers `InvalidRegisterEncoding`.

### 6.2 Flag semantics

- **ADD:** `r = (a + b) & 0xFF`, `Z = (r == 0)`, `N = (r & 0x80) != 0`, `C = (a + b > 0xFF)`, `V = ((~(a ^ b) & (a ^ r) & 0x80) != 0)`.
- **SUB / CMP:** `r = (a - b) & 0xFF`, `Z = (r == 0)`, `N = (r & 0x80) != 0`, `C = (a >= b)`, `V = (((a ^ b) & (a ^ r) & 0x80) != 0)`.
- **INC / DEC:** update `Z`, `N`, `V` (`V` set on `0x7F -> 0x80` for INC, `0x80 -> 0x7F` for DEC). `C` is unchanged.
- **AND, OR, XOR, NOT:** update `Z`, `N`; clear `C = 0, V = 0`.
- **Other instructions:** leave flags unchanged.

---

## 7. Machine bus, virtual devices, and atomicity contracts

### 7.1 Virtual port table

| Port | Direction | Behavior |
|---:|---|---|
| `0x00` | Input | Read next byte from preloaded FIFO queue. If empty: returns `0x00`, logs `InputUnderflow`, does not fault. |
| `0x01` | Output | Append byte as ASCII char to text console (non-printable bytes as `\xNN`). |
| `0x02` | Output | Append byte as unsigned decimal (`0..255`) to numeric console stream. |
| `0x10` | Output | Set eight virtual LEDs to written byte. |

### 7.2 Strict I/O atomicity contracts

To ensure failed instructions commit zero partial mutations:
1. **Side-effect-free permission validation:**
   - `IBus::can_read_io(port)` and `IBus::can_write_io(port)` are strictly `const noexcept` methods. They query port registration and direction without altering FIFO queues, text streams, or LED states.
2. **Two-phase FIFO consumption:**
   - Reading the input FIFO on Port `0x00` pops the head element **only** when `IBus::read_io(0x00)` is invoked during a valid instruction commit.
   - If an instruction faults (e.g., due to an `InvalidRegisterEncoding` in `IN R9, 0x00`), the port is never read and the FIFO head remains intact.
3. **Guaranteed output device integrity:**
   - Output devices (text stream, numeric stream, LEDs) are written only after all instruction validations succeed. If an instruction faults (e.g., `OUT 0x01, R9`), `IBus::write_io` is never invoked.
4. **Port directionality enforcement:**
   - Direction mismatches (`OUT 0x00` or `IN 0x01..0x10`) or access to unmapped ports trigger an `InvalidPort` CPU fault.

### 7.3 Instruction commit ordering

Every instruction step follows a strict eight-stage pipeline:
1. **Fetch:** Read opcode and operand bytes from memory using a transient address cursor `uint32_t cursor = PC;`. `PC` remains unaltered.
2. **Pre-validation:**
   - Validate opcode against defined table.
   - Validate register operands (`0..3`).
   - Validate stack boundaries against current `SP`.
   - Validate I/O port permissions using side-effect-free `can_read_io` / `can_write_io`.
3. **Fault abort (if validation failed):**
   - Record `Fault` with `instruction_address = PC`.
   - Transition machine state to `Faulted`.
   - Record fault in trace history.
   - Return immediately. Registers, flags, memory, `SP`, `PC`, and devices remain identical to their pre-instruction state.
4. **Pure computation:** Compute ALU result, effective addresses, or branch targets in local temporary variables.
5. **Memory and I/O commitment:** Perform memory writes via `write_byte` or I/O reads/writes via `read_io` / `write_io`.
6. **Register commitment:** Update destination registers.
7. **Flag commitment:** Update status flags (`Z`, `C`, `N`, `V`).
8. **PC advancement:** Advance `PC` sequentially or load branch/call/return target. If `HALT`, advance `PC` by 1 and transition to `Halted`. Generate trace entry.

---

## 8. Assembly language specification

- **Immediate ranges:**
  - `imm8` / `.BYTE`: `-128` through `255` (negative values encoded via 8-bit two's complement).
  - `.WORD`: `-32768` through `65535` (negative values encoded via 16-bit two's complement big-endian).
- **Directives:**
  - `.ORG address` (`0x0000..0xEFFF`)
  - `.ENTRY label` (resolves to `< 0xF000`)
  - `.BYTE val, ...`
  - `.WORD val`
  - `.ASCII "text"` (escapes `\n`, `\t`, `\r`, `\\`, `\"`, `\0` supported)
- Emitted bytes at or beyond `0xF000` are rejected as assembly errors.

---

## 9. CPU fault model and atomicity

Defined faults: `InvalidOpcode`, `InvalidRegisterEncoding`, `InvalidPort`, `StackOverflow`, `StackUnderflow`.

**Universal PC preservation rule:**
- All CPU faults without exception preserve architectural state: `R0..R3`, `IX`, `SP`, flags, and memory remain unchanged.
- `PC` remains at the instruction-start address for **all** faults.
- Machine transitions to `Faulted` and records a fault record with trace history.

---

## 10. Clean-checkout build and test experience

### 10.1 Multi-config vs single-config generator handling

CMake generators are categorized into two models, both supported:

#### Single-Configuration Generators (Ninja, Unix Makefiles)
The build configuration is selected at **configure time**:
```bash
# Configure
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Test
ctest --test-dir build --output-on-failure
```

#### Multi-Configuration Generators (Visual Studio, Xcode)
The build configuration is selected at **build and test time**:
```bash
# Configure
cmake -B build

# Build Debug
cmake --build build --config Debug

# Test Debug
ctest --test-dir build -C Debug --output-on-failure
```

#### Unified Preset Workflow (Recommended across all platforms)
Using `CMakePresets.json`:
```bash
# Configure
cmake --preset debug

# Build
cmake --build --preset debug

# Test
ctest --preset debug
```

---

## 11. Continuous integration (CI) design

### 11.1 Proposed GitHub Actions workflow design

Automated verification is driven by GitHub Actions on every push and pull request. CI explicitly selects the compiler, generator, and shell for each job:

```yaml
name: BitForge CI

on:
  push:
    branches: [ master, main ]
  pull_request:
    branches: [ master, main ]

jobs:
  build-and-test:
    name: ${{ matrix.name }}
    runs-on: ${{ matrix.os }}
    strategy:
      fail-fast: false
      matrix:
        include:
          # Windows MSVC 2022 (Ninja) - Debug
          - name: "Windows (MSVC 2022) [Debug]"
            os: windows-latest
            compiler: msvc
            build_type: Debug
            preset: windows-msvc-debug

          # Windows MSVC 2022 (Ninja) - Release
          - name: "Windows (MSVC 2022) [Release]"
            os: windows-latest
            compiler: msvc
            build_type: Release
            preset: windows-msvc-release

          # Linux GCC 13 (Ninja) - Debug
          - name: "Linux (GCC) [Debug]"
            os: ubuntu-latest
            compiler: gcc
            cc: gcc
            cxx: g++
            build_type: Debug

          # Linux Clang 16 (Ninja) - Debug
          - name: "Linux (Clang) [Debug]"
            os: ubuntu-latest
            compiler: clang
            cc: clang
            cxx: clang++
            build_type: Debug

          # macOS Apple Silicon (arm64) - Debug
          - name: "macOS Apple Silicon (arm64) [Debug]"
            os: macos-14
            compiler: apple-clang
            build_type: Debug

          # macOS Intel (x86_64) - Debug
          - name: "macOS Intel (x86_64) [Debug]"
            os: macos-13
            compiler: apple-clang
            build_type: Debug

    defaults:
      run:
        shell: bash

    steps:
      - name: Checkout repository
        uses: actions/checkout@v4

      - name: Set up MSVC environment (Windows)
        if: matrix.compiler == 'msvc'
        uses: ilammy/msvc-dev-cmd@v1

      - name: Set up Ninja
        uses: ninja-build/setup-ninja@v1

      - name: Install Linux build dependencies
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y gcc g++ clang

      - name: Print compiler information
        run: |
          if [ "${{ matrix.compiler }}" = "msvc" ]; then
            cl.exe || true
          elif [ -n "${{ matrix.cxx }}" ]; then
            ${{ matrix.cxx }} --version
          else
            c++ --version
          fi
          cmake --version
          ninja --version

      - name: Configure CMake (Linux / macOS)
        if: matrix.compiler != 'msvc'
        run: |
          cmake -B build -G Ninja \
            -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
            -DCMAKE_C_COMPILER=${{ matrix.cc || 'cc' }} \
            -DCMAKE_CXX_COMPILER=${{ matrix.cxx || 'c++' }} \
            -DBITFORGE_WARNINGS_AS_ERRORS=ON

      - name: Configure CMake (Windows MSVC)
        if: matrix.compiler == 'msvc'
        run: |
          cmake -B build -G Ninja \
            -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
            -DCMAKE_C_COMPILER=cl \
            -DCMAKE_CXX_COMPILER=cl \
            -DBITFORGE_WARNINGS_AS_ERRORS=ON

      - name: Build
        run: cmake --build build

      - name: Run CTest
        run: ctest --test-dir build --output-on-failure
```

*Note: This workflow is a design specification. Actual CI results will only be reported once the workflow is committed and executed on GitHub.*

---

## 12. Measurable release acceptance criteria

To avoid ambiguity, acceptance is defined by verifiable, bounded criteria:

### 12.1 ISA and Opcode Conformance Gate
- 100% of defined opcodes (all 30 opcodes) have dedicated automated unit tests verifying standard execution and boundary behavior.
- Every arithmetic flag (`Z`, `C`, `N`, `V`) is verified on edge cases (zero result, carry-out, signed two's complement overflow `0x7F + 1 = 0x80`, underflow `0x80 - 1 = 0x7F`, unsigned borrow).
- All 5 defined faults (`InvalidOpcode`, `InvalidRegisterEncoding`, `InvalidPort`, `StackOverflow`, `StackUnderflow`) are verified to abort atomically, preserving `PC` at the instruction start address.
- 16-bit address wraparound is verified: sequential PC rollover (`0xFFFF -> 0x0000`), multi-byte instruction straddling `0xFFFF`, and boundary reads/writes at `0x0000` and `0xFFFF`.

### 12.2 ADS Demonstration Test Suite Gate
The example programs are verified against explicit, documented input test vectors:
1. **Bubble Sort (`03_bubble_sort.asm`):**
   - Vector 1: Empty array ($N=0$).
   - Vector 2: Single element ($N=1$, `[42]`).
   - Vector 3: Two elements sorted ($N=2$, `[10, 20]`).
   - Vector 4: Two elements reversed ($N=2$, `[20, 10]`).
   - Vector 5: Duplicate elements ($N=5$, `[7, 7, 7, 7, 7]`).
   - Vector 6: Pre-sorted array ($N=8$, `[1, 2, 3, 4, 5, 6, 7, 8]`).
   - Vector 7: Reverse-sorted array ($N=8$, `[8, 7, 6, 5, 4, 3, 2, 1]`).
   - Vector 8: Pseudorandom array ($N=7$, `[64, 34, 25, 12, 22, 11, 90]`).
   - Bounded exhaustive permutation test: All $4! = 24$ permutations of multiset $\{1, 2, 3, 4\}$ ($N=4$) verified to yield sorted output `[1, 2, 3, 4]`.
2. **Linear Search (`02_linear_search.asm`):**
   - Key at first element ($index = 0$).
   - Key at middle element ($index = N/2$).
   - Key at last element ($index = N - 1$).
   - Key absent (returns sentinel `0xFF`).
3. **Factorial Subroutine (`04_factorial.asm`):**
   - $0! = 1$
   - $1! = 1$
   - $5! = 120$ (`0x78`), with `SP` verified to return exactly to its initial sentinel value.
4. **Arithmetic Baseline (`01_arithmetic.asm`):**
   - Program computes `10 + 20` and stores `0x1E` at address `0x8000`.

### 12.3 Multi-Platform CI Gate
- Clean compilation and 100% CTest pass rate across all matrix jobs: Windows MSVC (Debug & Release), Linux GCC, Linux Clang, macOS Intel (macos-13), and macOS Apple Silicon (macos-14).

---

## 13. Repository structure and specifications location

The repository structure preserves historical specifications at the repository root:

```text
bitforge/
├── .github/workflows/ci.yml           # Multi-platform CI workflow
├── CMakeLists.txt                      # Root CMake build definition
├── CMakePresets.json                   # Standardized build/test presets
├── .gitignore                          # Git ignore rules
├── README.md                           # Project overview & quickstart
├── BUILDING.md                         # Multi-platform build manual
├── BitForge_Architecture_Specification_v1.0.md  # Preserved historical root spec
├── BitForge_Architecture_Specification_v1.1.md  # Preserved historical root spec
├── BitForge_Architecture_Specification_v1.2.md  # Preserved historical root spec
├── BitForge_Architecture_Specification_v1.3.md  # Approved active specification
├── include/bitforge/                   # Public decoupled C++ headers
│   ├── core/                           # CPU, ALU, registers, flags, faults, types
│   ├── memory/                         # 64 KiB memory model
│   ├── bus/                            # IBus interface & SystemBus
│   ├── devices/                        # Virtual FIFO, text/numeric console, LEDs
│   ├── assembler/                      # Lexer, parser, codegen, diagnostics
│   └── debugger/                       # Execution controller, breakpoints, trace
├── src/                                # Modular implementation files
│   ├── core/
│   ├── memory/
│   ├── bus/
│   ├── devices/
│   ├── assembler/
│   ├── debugger/
│   └── cli/                            # Headless CLI main executable
├── tests/                              # CTest suite
│   ├── test_harness.hpp                # Dependency-free test assertion framework
│   ├── test_main.cpp                   # CTest test runner entry point
│   ├── unit/                           # Unit tests (ALU, CPU, memory, stack, I/O)
│   └── integration/                    # Integration tests (Arithmetic, Bubble Sort)
└── examples/                           # Verified BitForge assembly programs
    ├── 01_arithmetic.asm
    ├── 02_linear_search.asm
    ├── 03_bubble_sort.asm
    ├── 04_factorial.asm
    └── 05_io_devices.asm
```

---

## 14. Specification Changelog: v1.2 to v1.3

| Change ID | Affected Section | Semantic Decision Recorded |
|---|---|---|
| **CL-16** | Section 3.1 | **Target vs CI-Verified Delineation:** Separated broad target OS aspirations from explicitly verified CI platforms; added explicit separate verification for macOS Intel (`macos-13`) and macOS Apple Silicon (`macos-14`). |
| **CL-17** | Section 4.7 | **Configurable Warnings:** Added CMake option `BITFORGE_WARNINGS_AS_ERRORS` (default `ON`), with added `-Wshadow` and `-Wnon-virtual-dtor` flags. |
| **CL-18** | Section 5.4 | **Address Wraparound Specification:** Defined 16-bit modulo 65,536 address wraparound for sequential PC advancement (`0xFFFF -> 0x0000`), multi-byte instruction fetches straddling `0xFFFF`, and boundary memory reads/writes. |
| **CL-19** | Section 7.2, 7.3 | **Strict Bus & I/O Atomicity Contracts:** Defined side-effect-free I/O permission queries (`can_read_io`/`can_write_io`), two-phase FIFO consumption, and an explicit eight-stage instruction commit pipeline guaranteeing zero partial mutations on fault. |
| **CL-20** | Section 10.1 | **Multi-Config Generator Support:** Documented distinct workflows for single-config (Ninja/Make) and multi-config (Visual Studio/Xcode) CMake generators, alongside unified presets. |
| **CL-21** | Section 11.1 | **Explicit CI Compiler Selection:** Corrected GitHub Actions workflow to explicitly set `CC`/`CXX` for GCC vs Clang on Linux, initialize MSVC via `ilammy/msvc-dev-cmd@v1` on Windows, print compiler versions in logs, and separate Debug and Release jobs. |
| **CL-22** | Section 12 | **Measurable Acceptance Criteria:** Replaced ambiguous coverage statements with bounded, testable criteria including 8 explicit Bubble Sort test vectors, a 24-permutation exhaustive test set, and explicit Linear Search, Factorial, and Arithmetic test vectors. |
| **CL-23** | Section 13 | **Specification File Location Resolution:** Confirmed permanent preservation of historical specifications at the repository root without deletion or moving. |
