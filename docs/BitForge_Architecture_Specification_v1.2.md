# BitForge Architecture Specification v1.2: Portability and Shareability

**Project:** BitForge — An 8-Bit Computer Emulator, Assembler and Interactive Debugging Workbench  
**Document status:** Approved architectural baseline  
**Primary language:** Standard C++17 (Strict ISO standard, extensions disabled)  
**Target platforms:** Windows 10/11, Modern Mainstream Linux, macOS (Intel & Apple Silicon)  
**Initial target:** Headless desktop core and CLI; GUI workbench deferred to Phase 6  
**Project context:** Individual Algorithm and Data Structures mini project  
**Predecessor documents:**
- `BitForge_Architecture_Specification_v1.0.md` (preserved unchanged)
- `BitForge_Architecture_Specification_v1.1.md` (preserved unchanged)

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

### 1.2 Explicit non-goals for v1.2

- Cycle-accurate or transistor-level simulation.
- A pipelined or out-of-order CPU.
- A commercial C/C++ compiler or general-purpose macro assembler.
- Operating system simulation, hardware interrupts, multitasking, or privilege rings.
- Multiple emulated CPU architectures.
- Immediate packaging of installers or GUI bundles prior to core stabilization.

---

## 2. High-level components

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
3. **Machine bus (`IBus`):** Narrow interface connecting CPU to memory and virtual port I/O.
4. **Virtual devices:** Deterministic input FIFO queue, text console, numeric stream, and virtual LEDs.
5. **Assembler:** Source parsing, label resolution, instruction encoding, directives, and diagnostics.
6. **Debugger:** Step/run/pause/reset execution controller, breakpoints, trace history, and source mapping.
7. **CLI runner:** Headless executable for automated testing and batch execution.
8. **GUI workbench:** Visual assembly editor, register/flag monitors, memory viewer, stack viewer, LED display, and trace visualization (Phase 6).
9. **Automated test suite:** Unit tests, integration tests, and ADS algorithmic benchmarks driven by CTest.

The CPU core and assembler must have zero dependencies on Qt, OS GUI toolkits, or host terminal APIs.

---

## 3. Supported platforms and toolchains

BitForge is engineered for cross-platform portability across Windows, Linux, and macOS. Platform claims are strictly gated by passing automated builds and test suites on each environment.

### 3.1 Target operating systems

| Operating System | Architectures | Target Baseline |
|---|---|---|
| **Windows** | x86_64 | Windows 10 (version 1809+) / Windows 11 |
| **Linux** | x86_64, aarch64 | Modern mainstream distributions (Ubuntu 22.04/24.04 LTS, Debian 12+, Fedora 39+) |
| **macOS** | Apple Silicon (arm64), Intel (x86_64) | macOS 12 (Monterey) or newer |

### 3.2 Supported compiler families and minimum versions

To guarantee strict C++17 conformance (including `<string_view>`, `<optional>`, `<variant>`, and `<filesystem>`) and clean CMake integration:

| Compiler Family | Minimum Version | Recommended Version | Standard Library |
|---|---|---|---|
| **GCC** | 9.3+ | 11.0+ / 14.0+ | `libstdc++` (GCC 9+ provides complete `<filesystem>`) |
| **LLVM Clang** | 10.0+ | 14.0+ / 18.0+ | `libc++` or `libstdc++` |
| **Apple Clang** | Xcode 12.0+ (Apple Clang 12.0+) | Xcode 14.0+ | Apple `libc++` |
| **MSVC** | Visual Studio 2019 (v16.10 / MSVC 19.28) | Visual Studio 2022 (v17.0+ / MSVC 19.30+) | Microsoft STL (`/std:c++17`, `/permissive-`) |

### 3.3 Build system requirements

- **CMake:** Minimum version **3.20** required (supports `CMakePresets.json` schema v2/v3). Version **3.25+** recommended.
- **Generators:** Ninja (cross-platform recommended), Visual Studio 16/17 (Windows), Unix Makefiles (Linux/macOS).

---

## 4. Portable implementation rules

All source code in the headless core, assembler, debugger, CLI, and test suite must comply with the following portable engineering standards:

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

### 4.7 Compiler warning flags
Warnings must be treated as errors in CI builds, conditioned per compiler family in CMake:
- **GCC / Clang:** `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror`
- **MSVC:** `/W4 /WX /permissive-`

### 4.8 Zero-overhead dependency minimization
- The core emulator, memory, assembler, debugger, and CLI runner must rely exclusively on the **C++17 Standard Library**.
- No third-party utility libraries (Boost, Abseil, etc.) are permitted in the core.
- The unit testing harness must be lightweight and dependency-free (or self-contained header-only) so checkout requires no package managers.

---

## 5. Machine architecture specification (Unchanged from v1.1)

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
| Interrupts | Not implemented in v1.2 |

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

### 5.4 Memory conventions

| Address range | Convention |
|---|---|
| `0x0000–0x7FFF` | Program code and small constants |
| `0x8000–0xEFFF` | Static data and arrays |
| `0xF000–0xFFFE` | Reserved stack storage |
| `0xFFFF` | Stack-pointer empty sentinel; not available to assembler-emitted program data |

Assembler-emitted bytes at `0xF000` or above are rejected to prevent normal program images from overwriting the stack region. Runtime CPU operations address the flat 16-bit space without memory protection.

### 5.5 Stack conventions and pre-validation

The stack grows toward lower addresses within `0xF000`–`0xFFFE`. `0xFFFF` is the empty sentinel. All stack operations must validate boundaries **before** committing state:

1. **`PUSH Rs`:** Requires `SP > 0xF000`. If `SP <= 0xF000`, immediate `StackOverflow` fault. When valid: decrement `SP`, store byte at `memory[SP]`.
2. **`POP Rd`:** Requires `SP < 0xFFFF`. If `SP >= 0xFFFF`, immediate `StackUnderflow` fault. When valid: load byte from `memory[SP]`, increment `SP`.
3. **`CALL addr16`:** Requires `SP >= 0xF002`. If `SP < 0xF002`, immediate `StackOverflow` fault. When valid: store `ret_hi` at `memory[SP - 2]`, `ret_lo` at `memory[SP - 1]`, `SP = SP - 2`, `PC = addr16`.
4. **`RET`:** Requires `SP <= 0xFFFD`. If `SP > 0xFFFD`, immediate `StackUnderflow` fault. When valid: pop `ret_hi` from `[SP]`, increment `SP`, pop `ret_lo` from `[SP]`, increment `SP`, `PC = (ret_hi << 8) | ret_lo`.
5. **Atomicity:** Any stack fault leaves `SP`, `PC`, registers, flags, and memory untouched.

---

## 6. Instruction set and encoding (Unchanged from v1.1)

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

## 7. Virtual input/output devices (Unchanged from v1.1)

| Port | Direction | Behavior |
|---:|---|---|
| `0x00` | Input | Read next byte from preloaded FIFO queue. If empty: returns `0x00`, logs `InputUnderflow`, does not fault. |
| `0x01` | Output | Append byte as ASCII char to text console (non-printable bytes as `\xNN`). |
| `0x02` | Output | Append byte as unsigned decimal (`0..255`) to numeric console stream. |
| `0x10` | Output | Set eight virtual LEDs to written byte. |

Direction mismatch (`OUT 0x00` or `IN 0x01..0x10`) or unmapped port access triggers `InvalidPort` CPU fault.

---

## 8. Assembly language specification (Unchanged from v1.1)

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

## 9. CPU fault model and atomicity (Unchanged from v1.1)

Defined faults: `InvalidOpcode`, `InvalidRegisterEncoding`, `InvalidPort`, `StackOverflow`, `StackUnderflow`.

**Strict fault atomicity rule:**
- All CPU faults preserve architectural state: `R0..R3`, `IX`, `SP`, flags, and memory remain unchanged.
- `PC` remains at the instruction-start address for **all** faults without exception.
- Machine transitions to `Faulted` and records a fault record with trace history.

---

## 10. Build and test experience

The repository must provide a clean-checkout workflow that works out of the box on Windows, Linux, and macOS without requiring any specific IDE.

### 10.1 Required repository documentation files

1. **`README.md`:**
   - Project overview, architectural goals, feature matrix.
   - Prerequisites list (compiler, CMake, Git).
   - Quick-start build and test commands.
   - Example assembly code walkthrough (Arithmetic and Bubble Sort).
   - Project status and roadmap.
2. **`BUILDING.md`:**
   - Detailed build instructions tailored for Windows, Linux, and macOS.
   - Instructions for CLI-based development (Ninja, Make) and optional IDE integration (VS Code, Visual Studio, CLion, Xcode).
   - Troubleshooting common configuration errors.
3. **`CMakePresets.json`:**
   - Standardized configuration, build, and test presets:
     - `debug`: Debug build with warnings, debug info, assertions enabled.
     - `release`: Optimized build with assertions disabled.
     - `test`: Automated test preset running CTest with verbose failure logging.

### 10.2 Standard command sequence

Every developer and CI runner follows the same universal three-step workflow:

```bash
# 1. Configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# 2. Build
cmake --build build

# 3. Test
ctest --test-dir build --output-on-failure
```

---

## 11. Continuous integration (CI) design

Automated verification is driven by GitHub Actions on every push and pull request. CI verifies the headless core, assembler, debugger, CLI, and test suite across all three supported operating systems.

### 11.1 CI matrix specification

```yaml
name: BitForge CI

on:
  push:
    branches: [ master, main ]
  pull_request:
    branches: [ master, main ]

jobs:
  build-and-test:
    name: ${{ matrix.os }} - ${{ matrix.compiler }} (${{ matrix.build_type }})
    runs-on: ${{ matrix.os }}
    strategy:
      fail-fast: false
      matrix:
        include:
          # Windows MSVC
          - os: windows-latest
            compiler: msvc
            build_type: Debug
          # Linux GCC
          - os: ubuntu-latest
            compiler: gcc
            build_type: Debug
          # Linux Clang
          - os: ubuntu-latest
            compiler: clang
            build_type: Debug
          # macOS Apple Clang
          - os: macos-latest
            compiler: apple-clang
            build_type: Debug

    steps:
      - name: Checkout repository
        uses: actions/checkout@v4

      - name: Set up Linux dependencies
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y ninja-build

      - name: Set up Ninja on Windows/macOS
        uses: ninja-build/setup-ninja@v1

      - name: Configure CMake
        run: cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}

      - name: Build
        run: cmake --build build

      - name: Run CTest
        run: ctest --test-dir build --output-on-failure
```

### 11.2 CI verification criteria
CI must enforce:
- Zero compiler warnings (`-Werror` / `/WX`).
- 100% CTest pass rate across all matrix jobs.
- Identical CPU execution traces and register outcomes across Windows, Linux, and macOS.

---

## 12. Repository, collaboration, and licensing

### 12.1 Repository structure

```text
bitforge/
├── .github/
│   └── workflows/
│       └── ci.yml                      # Cross-platform GitHub Actions workflow
├── CMakeLists.txt                      # Root CMake build definition
├── CMakePresets.json                   # Standardized build/test presets
├── .gitignore                          # C++, CMake, OS, and IDE ignore rules
├── README.md                           # Project overview, quickstart, examples
├── BUILDING.md                         # Detailed multi-platform build manual
├── docs/
│   ├── BitForge_Architecture_Specification_v1.0.md
│   ├── BitForge_Architecture_Specification_v1.1.md
│   └── BitForge_Architecture_Specification_v1.2.md
├── include/
│   └── bitforge/
│       ├── core/                       # CPU, ALU, registers, flags, faults, types
│       ├── memory/                     # 64 KiB RAM model
│       ├── bus/                        # IBus interface and SystemBus
│       ├── devices/                    # Virtual FIFO, text/numeric console, LEDs
│       ├── assembler/                  # Lexer, parser, code generation, diagnostics
│       └── debugger/                   # Trace buffer, breakpoints, controller
├── src/
│   ├── core/
│   ├── memory/
│   ├── bus/
│   ├── devices/
│   ├── assembler/
│   ├── debugger/
│   └── cli/                            # Headless CLI main executable
├── tests/
│   ├── test_harness.hpp                # Dependency-free unit test harness
│   ├── test_main.cpp                   # CTest entry runner
│   ├── unit/                           # ALU, CPU, memory, stack, devices, assembler
│   └── integration/                    # Arithmetic, Linear Search, Bubble Sort
└── examples/                           # Verified BitForge assembly programs
    ├── 01_arithmetic.asm
    ├── 02_linear_search.asm
    ├── 03_bubble_sort.asm
    ├── 04_factorial.asm
    └── 05_io_devices.asm
```

### 12.2 Collaboration rules
- **No generated files committed:** Never commit `build/`, `bin/`, `*.exe`, `*.o`, `*.log`, or `.vs/`.
- **No personal paths:** All paths in scripts and CMake files must be relative to the repository root.
- **Clean checkout guarantee:** A fresh clone on any supported platform builds immediately with standard tools.

### 12.3 Licensing options (Decision pending approval)
Until the user formally approves a license, no `LICENSE` file is created and all rights are reserved by the author. Potential options for educational distribution include:
1. **MIT License:** Permissive, widely understood, optimal for educational sharing with classmates.
2. **Apache 2.0:** Permissive with explicit patent grant.
3. **GPLv3:** Copyleft, guarantees derivative works remain open source.
4. **Educational / All Rights Reserved:** Explicit academic sharing statement without public open-source licensing.

---

## 13. Distribution strategy

### 13.1 Phased distribution approach
- **Developer / Source Distribution:** Source archive (`.zip` / `.tar.gz`) for Git clone, building via CMake.
- **Compiled CLI Binaries (GitHub Releases):**
  - Windows: Portable `bitforge-cli.exe` (standalone binary).
  - Linux: Portable static or standard glibc `bitforge-cli` binary.
  - macOS: Universal binary (`x86_64` + `arm64`) `bitforge-cli`.
- **Version Identification:** `bitforge-cli --version` prints semantic version, commit hash, build date, and host architecture.

### 13.2 GUI runtime bundling (Deferred to Phase 6)
- Windows: Deploy Qt runtime DLLs via `windeployqt`.
- macOS: Deploy macOS `.app` bundle via `macdeployqt`.
- Linux: Self-contained AppImage or distribution package.

---

## 14. GUI portability evaluation (Phase 6 Candidate)

- **Candidate framework:** Qt 6 Widgets (`C++17`).
- **Portability:** Native look-and-feel across Windows, Linux, and macOS.
- **Decoupled Architecture:** The GUI must only interact with the emulator through the `Debugger` and `IBus` query APIs. It must never implement CPU semantics.
- **Licensing review required before adoption:** Qt 6 Widgets is available under LGPLv3 or commercial licensing. Dynamic linking must be maintained to comply with LGPLv3 if chosen.

---

## 15. Additional acceptance criteria for release

BitForge v1.2 is ready for release only when:
- [ ] Clean build passes on Windows, Linux, and macOS without compiler warnings.
- [ ] 100% of automated unit and integration tests pass under CTest on all three platforms.
- [ ] Cross-platform text-file and CRLF/LF line-ending handling is verified.
- [ ] Arithmetic and bit-manipulation semantics produce identical results across platforms regardless of host endianness.
- [ ] Bubble Sort and all ADS demo programs execute successfully on the emulated CPU.
- [ ] A developer can clone the repository and build it following only `README.md` and `BUILDING.md`.

---

## 16. Specification Changelog: v1.1 to v1.2

| Change ID | Affected Section | Semantic Decision Recorded |
|---|---|---|
| **CL-09** | Section 3 | **Supported Platforms & Compilers:** Added explicit target OS matrix (Windows 10/11, modern Linux, macOS Intel/Apple Silicon) and minimum compiler versions (GCC 9.3+, Clang 10.0+, Apple Clang 12.0+, MSVC 2019 v16.10+). |
| **CL-10** | Section 4 | **Portable Implementation Rules:** Added rules enforcing strict ISO C++17 (`CMAKE_CXX_EXTENSIONS OFF`), `std::filesystem`, UTF-8 CRLF/LF normalization, explicit fixed-width integers (`<cstdint>`), explicit big-endian word decoding, and conditional compiler warnings. |
| **CL-11** | Section 10 | **Build Experience & Presets:** Mandated `README.md`, `BUILDING.md`, `CMakePresets.json`, and standard 3-step build/test workflow. |
| **CL-12** | Section 11 | **Continuous Integration Matrix:** Designed GitHub Actions workflow matrix spanning Windows, Linux, and macOS with Ninja and CTest. |
| **CL-13** | Section 12 | **Repository & Collaboration Rules:** Formulated repository directory layout, `.gitignore` standards, and documented licensing options requiring user approval. |
| **CL-14** | Section 13, 14 | **Distribution & GUI Strategy:** Documented source archives, release CLI binaries, versioning, and Qt 6 LGPLv3 evaluation criteria for Phase 6. |
| **CL-15** | Section 15 | **Cross-Platform Acceptance Gate:** Formulated release acceptance criteria requiring clean builds and identical test results across Windows, Linux, and macOS. |
