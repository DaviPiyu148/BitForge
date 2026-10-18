# BitForge Architecture Specification v1.7: Final Implementation Baseline

**Project:** BitForge — An 8-Bit Computer Emulator, Assembler and Interactive Debugging Workbench  
**Document status:** Approved architectural baseline for implementation  
**Primary language:** Standard C++17 (Strict ISO standard, extensions disabled; C++ language only)  
**Target platforms:** Windows 10/11, Modern Mainstream Linux, macOS (Intel x86_64 & Apple Silicon arm64)  
**Initial target:** Headless desktop core and CLI; GUI workbench deferred to Phase 6  
**Project context:** Individual Algorithm and Data Structures mini project  
**Predecessor documents:**
- `BitForge_Architecture_Specification_v1.0.md` (preserved unchanged)
- `BitForge_Architecture_Specification_v1.1.md` (preserved unchanged)
- `BitForge_Architecture_Specification_v1.2.md` (preserved unchanged)
- `BitForge_Architecture_Specification_v1.3.md` (preserved unchanged)
- `BitForge_Architecture_Specification_v1.4.md` (preserved unchanged)
- `BitForge_Architecture_Specification_v1.5.md` (preserved unchanged)
- `BitForge_Architecture_Specification_v1.6.md` (preserved unchanged)

---

## 1. Purpose and product vision

BitForge is an educational software model of a small fictional computer. Users write programs in BitForge Assembly, assemble them into machine code, run them on an emulated CPU, and inspect execution through a debugger and visual workbench.

The signature demonstration is a Bubble Sort program written in BitForge Assembly and executed by the emulated processor. The application enables an observer to monitor the current instruction, CPU registers and flags, memory mutations, stack activity, and execution trace.

BitForge is an instruction-set simulator. It is **not** intended to simulate transistor-level behavior, electrical timing, or a physical processor's exact microarchitecture. Its CPU semantics are deterministic, cross-platform, and fully documented.

### 1.1 Primary goals

1. Define a small, internally consistent 8-bit instruction set containing exactly 31 instruction mnemonics.
2. Execute assembled machine code accurately and deterministically across all supported host platforms.
3. Provide a two-pass assembler with labels, directives, and structured source diagnostics.
4. Support single-stepping, breakpoints, tracing, and CPU/memory inspection.
5. Demonstrate ADS concepts through programs that execute on the simulated CPU.
6. Test core behavior independently from the graphical interface.
7. Support clean-checkout collaboration, continuous integration, and seamless cross-platform building.

### 1.2 Explicit non-goals for v1.7

- Cycle-accurate or transistor-level simulation.
- A pipelined, superscalar, or out-of-order CPU.
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
9. **Automated test suite:** Unit tests, integration tests, and ADS algorithmic benchmarks driven by CTest with explicit instruction-budget termination and rich diagnostic reporting.

The CPU core and assembler must have zero dependencies on Qt, OS GUI toolkits, or host terminal APIs.

---

## 3. Supported platforms and toolchains

### 3.1 Target operating systems vs CI-verified platforms

A rigorous distinction is maintained between intended design targets and platforms verified by automated CI builds:

| Operating System | Architectures | Target Status | Proposed CI Verification Runner |
|---|---|---|---|
| **Windows 10/11** | x86_64 | Supported | `windows-latest` (MSVC 2022 + Ninja via PowerShell) |
| **Linux (Ubuntu)** | x86_64 | Supported | `ubuntu-latest` (GCC + Ninja; Clang + Ninja via Bash) |
| **Linux (Other distros)** | x86_64, aarch64 | Intended (Portable ISO C++17) | Unverified by default CI |
| **macOS (Apple Silicon)** | arm64 | Supported | `macos-15` (Apple Clang + Ninja via Bash) |
| **macOS (Intel)** | x86_64 | Supported (Transitional Sunset Track) | `macos-15-intel` (Apple Clang + Ninja via Bash) |

*Platform verification and macOS runner lifecycle:*
- Platform claims are strictly gated by passing automated test runs on that specific configuration.
- The proposed CI workflow pins explicit architecture-specific runners: `macos-15` for Apple Silicon (arm64) and `macos-15-intel` for Intel (x86_64).
- The retired `macos-13` runner and deprecated `macos-14` runner are not used.
- **Intel macOS Sunset Horizon:** GitHub has officially published that macOS 15 is the final operating system release that will provide x86_64 Intel runner hardware. `macos-15-intel` is provided as transitional CI coverage. When GitHub ends x86_64 runner availability, this job must be retired, transitioning macOS CI exclusively to Apple Silicon.
- The status of CI in this document is a **proposed engineering design**, not an executed pass claim.

### 3.2 Supported compiler families and minimum versions

| Compiler Family | Minimum Version | Recommended Version | Standard Library |
|---|---|---|---|
| **GCC** | 9.3+ | 11.0+ / 14.0+ | `libstdc++` (GCC 9+ provides complete `<filesystem>`) |
| **LLVM Clang** | 10.0+ | 14.0+ / 18.0+ | `libc++` or `libstdc++` |
| **Apple Clang** | Xcode 12.0+ (Apple Clang 12.0+) | Xcode 15.0+ / 16.0+ | Apple `libc++` |
| **MSVC** | Visual Studio 2019 (v16.10 / MSVC 19.28) | Visual Studio 2022 (v17.0+ / MSVC 19.30+) | Microsoft STL (`/std:c++17`, `/permissive-`) |

### 3.3 Build system requirements

- **CMake:** Minimum version **3.20** required. Version **3.25+** recommended for full preset schema v5 support.
- **Language configuration:** CMake project explicitly enables **C++ only** (`project(BitForge LANGUAGES CXX)`), avoiding any implicit or unnecessary C compiler dependency.
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

### 4.7 Compiler warning policy
Warnings are conditioned per compiler family in CMake, with warnings-as-errors controlled via a dedicated build option:
- **CMake option:** `option(BITFORGE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)`
- **Policy:**
  - **Local Developer Builds:** Default to `OFF` so developers on diverse compiler versions are not blocked by compiler-specific warnings.
  - **Continuous Integration (CI):** Explicitly enabled (`-DBITFORGE_WARNINGS_AS_ERRORS=ON`) across all matrix jobs.
- **Flags:**
  - **GCC / Clang:** `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor` (appends `-Werror` when enabled).
  - **MSVC:** `/W4 /permissive-` (appends `/WX` when enabled).

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
| Interrupts | Not implemented in v1.7 |

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

### 5.4 Address wraparound rules and testing bypass

The 16-bit address space wraps strictly modulo 65,536:
1. **Sequential PC advancement:** Advancing `PC` wraps from `0xFFFF` to `0x0000`: `PC = static_cast<word_t>((PC + 1) & 0xFFFF)`.
2. **Multi-byte instruction fetch across boundary:** If an instruction begins at `0xFFFE` or `0xFFFF`:
   - Opcode byte fetched from `PC`.
   - Operand bytes fetched at `(PC + 1) & 0xFFFF`, `(PC + 2) & 0xFFFF`, wrapping safely into low memory without out-of-bounds array access.
3. **Index register arithmetic:** `INX` wraps `0xFFFF -> 0x0000`; `DEX` wraps `0x0000 -> 0xFFFF`.
4. **Data memory reads and writes:** `LD`, `ST`, `LDX`, `STX` access `memory[addr]` where `addr` is an unsigned 16-bit integer `0x0000..0xFFFF`. Out-of-bounds indexing is mathematically impossible because the storage array has exactly 65,536 entries.
5. **CPU test loading bypass:** While the assembler rejects program data emitted at or beyond `0xF000` via `.ORG`, CPU-level unit tests may load bytes directly into `Memory` at any address (`0x0000..0xFFFF`) to explicitly verify boundary execution, multibyte wrap across `0xFFFF/0x0000`, and extreme PC rollover.
6. **Stack boundary exemption:** The only addresses exempted from cyclic wraparound are stack push/pop/call/ret operations, which trigger `StackOverflow` or `StackUnderflow` upon reaching reserved stack bounds rather than wrapping.

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

## 6. Instruction set and encoding (31 Instruction Mnemonics)

The BitForge instruction set contains exactly **31 instruction mnemonics** organized into four functional groups:

1. **Basic Control & Data Movement (11 mnemonics):** `NOP`, `HALT`, `LDI`, `MOV`, `LD`, `ST`, `LDI16`, `LDX`, `STX`, `INX`, `DEX`.
2. **Arithmetic & Logic (9 mnemonics):** `ADD`, `SUB`, `INC`, `DEC`, `AND`, `OR`, `XOR`, `NOT`, `CMP`.
3. **Branch, Stack & Call (9 mnemonics):** `JMP`, `JZ`, `JNZ`, `JC`, `JNC`, `CALL`, `RET`, `PUSH`, `POP`.
4. **Input/Output (2 mnemonics):** `IN`, `OUT`.

### 6.1 Canonical opcode table

| Opcode | Mnemonic and assembly form | Group | Length | Operand-byte encoding | Purpose |
|---:|---|---|---:|---|---|
| `0x00` | `NOP` | Control/Data | 1 | — | No operation |
| `0x01` | `HALT` | Control/Data | 1 | — | Stop execution normally |
| `0x10` | `LDI Rd, imm8` | Control/Data | 3 | `reg, imm8` | Load an 8-bit constant into a register |
| `0x11` | `MOV Rd, Rs` | Control/Data | 3 | `dst, src` | Copy one register to another |
| `0x12` | `LD Rd, [addr16]` | Control/Data | 4 | `reg, addr_hi, addr_lo` | Load a byte from absolute memory |
| `0x13` | `ST [addr16], Rs` | Control/Data | 4 | `addr_hi, addr_lo, reg` | Store a byte to absolute memory |
| `0x14` | `LDI16 IX, addr16` | Control/Data | 3 | `addr_hi, addr_lo` | Load an address into `IX` |
| `0x15` | `LDX Rd, [IX]` | Control/Data | 2 | `reg` | Load a byte from memory addressed by `IX` |
| `0x16` | `STX [IX], Rs` | Control/Data | 2 | `reg` | Store a byte to memory addressed by `IX` |
| `0x17` | `INX` | Control/Data | 1 | — | Increment `IX` modulo 65,536 |
| `0x18` | `DEX` | Control/Data | 1 | — | Decrement `IX` modulo 65,536 |
| `0x20` | `ADD Rd, Rs` | Arithmetic/Logic | 3 | `dst, src` | Add source to destination register |
| `0x21` | `SUB Rd, Rs` | Arithmetic/Logic | 3 | `dst, src` | Subtract source from destination register |
| `0x22` | `INC Rd` | Arithmetic/Logic | 2 | `reg` | Increment an 8-bit register |
| `0x23` | `DEC Rd` | Arithmetic/Logic | 2 | `reg` | Decrement an 8-bit register |
| `0x30` | `AND Rd, Rs` | Arithmetic/Logic | 3 | `dst, src` | Bitwise AND |
| `0x31` | `OR Rd, Rs` | Arithmetic/Logic | 3 | `dst, src` | Bitwise OR |
| `0x32` | `XOR Rd, Rs` | Arithmetic/Logic | 3 | `dst, src` | Bitwise XOR |
| `0x33` | `NOT Rd` | Arithmetic/Logic | 2 | `reg` | Bitwise complement |
| `0x40` | `CMP Ra, Rb` | Arithmetic/Logic | 3 | `left, right` | Set subtraction-comparison flags without storing a result |
| `0x50` | `JMP addr16` | Branch/Stack/Call | 3 | `addr_hi, addr_lo` | Unconditional absolute jump |
| `0x51` | `JZ addr16` | Branch/Stack/Call | 3 | `addr_hi, addr_lo` | Jump if `Z = 1` |
| `0x52` | `JNZ addr16` | Branch/Stack/Call | 3 | `addr_hi, addr_lo` | Jump if `Z = 0` |
| `0x53` | `JC addr16` | Branch/Stack/Call | 3 | `addr_hi, addr_lo` | Jump if `C = 1` |
| `0x54` | `JNC addr16` | Branch/Stack/Call | 3 | `addr_hi, addr_lo` | Jump if `C = 0` |
| `0x60` | `CALL addr16` | Branch/Stack/Call | 3 | `addr_hi, addr_lo` | Push return address and call absolute target |
| `0x61` | `RET` | Branch/Stack/Call | 1 | — | Return using the stack |
| `0x62` | `PUSH Rs` | Branch/Stack/Call | 2 | `reg` | Push one register byte |
| `0x63` | `POP Rd` | Branch/Stack/Call | 2 | `reg` | Pop one byte into a register |
| `0x70` | `IN Rd, port8` | I/O | 3 | `reg, port` | Read a byte from a virtual input port |
| `0x71` | `OUT port8, Rs` | I/O | 3 | `port, reg` | Write a register byte to a virtual output port |

Register IDs: `R0 = 0`, `R1 = 1`, `R2 = 2`, `R3 = 3`. Any other ID triggers `InvalidRegisterEncoding`.

### 6.2 Flag semantics

- **ADD:** `r = (a + b) & 0xFF`, `Z = (r == 0)`, `N = (r & 0x80) != 0`, `C = (a + b > 0xFF)`, `V = ((~(a ^ b) & (a ^ r) & 0x80) != 0)`.
- **SUB / CMP:** `r = (a - b) & 0xFF`, `Z = (r == 0)`, `N = (r & 0x80) != 0`, `C = (a >= b)`, `V = (((a ^ b) & (a ^ r) & 0x80) != 0)`.
- **INC / DEC:** update `Z`, `N`, `V` (`V` set on `0x7F -> 0x80` for INC, `0x80 -> 0x7F` for DEC). `C` is unchanged.
- **AND, OR, XOR, NOT:** update `Z`, `N`; clear `C = 0, V = 0`.
- **Other instructions:** leave flags unchanged.

---

## 7. Machine bus, virtual devices, and state transition contracts

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

### 7.3 CPU state transitions and instruction execution contract

The CPU operates as a deterministic, sequential instruction interpreter following explicit state transitions:

```
[Reset] ---> (Ready)
               |
          step() from Ready
               |
               +---> Normal Instruction ---> (Running)
               +---> HALT Instruction -----> (Halted)
               +---> CPU Fault ------------> (Faulted)

(Running) ---> step() ---> (Running)
             |
             +---> HALT Instruction --------> (Halted)
             +---> CPU Fault --------------> (Faulted)
             +---> User Pause / Budget -----> (Paused)
```

1. **State upon Reset:** `cpu.reset(entry)` sets machine state to `MachineState::Ready`.
2. **Stepping from `Ready`:**
   - If the instruction executes successfully and is not `HALT`, state transitions to `MachineState::Running`.
   - If the instruction is `HALT`, state transitions to `MachineState::Halted`.
   - If the instruction faults, state transitions to `MachineState::Faulted`.
3. **Stepping from `Running`:**
   - Continues in `MachineState::Running` unless `HALT` (transitions to `Halted`), fault (transitions to `Faulted`), or budget reached (`Paused`).
4. **Stepping from `Paused`:**
   - Single-stepping executes exactly one instruction and returns to `MachineState::Paused` (unless `HALT` or fault).
5. **Stepping from `Halted` or `Faulted`:**
   - Step is an explicit no-op; returns immediately with `executed = false` without changing architectural state.

### 7.4 Sequential instruction validation and commit sequence

Every instruction step follows an eight-stage sequence:
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

## 11. Complete proposed continuous integration (CI) workflow

### 11.1 Proposed `.github/workflows/ci.yml`

Automated verification is driven by GitHub Actions on every push and pull request. CI explicitly enables C++ only, selects compilers cleanly, separates Windows PowerShell from POSIX Bash jobs (eliminating invalid expression usage in `defaults.run`), verifies compiler paths in `CMakeCache.txt`, and avoids error suppression:

```yaml
name: BitForge CI

on:
  push:
    branches: [ master, main ]
  pull_request:
    branches: [ master, main ]

jobs:
  # -------------------------------------------------------------
  # Windows MSVC Build & Test (PowerShell shell prevents link.exe collisions)
  # -------------------------------------------------------------
  windows-msvc:
    name: ${{ matrix.name }}
    runs-on: windows-latest
    strategy:
      fail-fast: false
      matrix:
        include:
          - name: "Windows (MSVC 2022) [Debug]"
            build_type: Debug
          - name: "Windows (MSVC 2022) [Release]"
            build_type: Release

    defaults:
      run:
        shell: pwsh

    steps:
      - name: Checkout repository
        uses: actions/checkout@v4

      - name: Set up MSVC environment
        uses: ilammy/msvc-dev-cmd@v1

      - name: Set up Ninja
        uses: ninja-build/setup-ninja@v1

      - name: Verify MSVC compiler on PATH
        run: |
          Get-Command cl.exe | Select-Object -ExpandProperty Source
          cmake --version
          ninja --version

      - name: Configure CMake
        run: |
          cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} -DCMAKE_CXX_COMPILER=cl -DBITFORGE_WARNINGS_AS_ERRORS=ON

      - name: Verify CMake-selected compiler in CMakeCache.txt
        run: |
          $line = Get-Content build/CMakeCache.txt | Where-Object { $_ -match "^CMAKE_CXX_COMPILER(:.*)?=(.*)$" }
          if (-not $line) { throw "CMAKE_CXX_COMPILER entry not found in build/CMakeCache.txt" }
          $cxxPath = $Matches[2]
          Write-Host "Configured CMake CXX Compiler: $cxxPath"
          if ($cxxPath -notmatch "cl(\.exe)?$") { throw "Expected MSVC cl.exe but found: $cxxPath" }

      - name: Build
        run: cmake --build build

      - name: Run CTest
        run: ctest --test-dir build --output-on-failure

  # -------------------------------------------------------------
  # POSIX Linux & macOS Build & Test (Bash shell)
  # -------------------------------------------------------------
  posix-build:
    name: ${{ matrix.name }}
    runs-on: ${{ matrix.os }}
    strategy:
      fail-fast: false
      matrix:
        include:
          # Linux GCC
          - name: "Linux (GCC) [Debug]"
            os: ubuntu-latest
            cxx: g++
            expected: "g++"
            build_type: Debug

          # Linux Clang
          - name: "Linux (Clang) [Debug]"
            os: ubuntu-latest
            cxx: clang++
            expected: "clang++"
            build_type: Debug

          # macOS Apple Silicon (arm64, macOS 15)
          - name: "macOS Apple Silicon (Apple Clang) [Debug]"
            os: macos-15
            cxx: clang++
            expected: "clang++"
            build_type: Debug

          # macOS Intel (x86_64, macOS 15 Intel fallback)
          - name: "macOS Intel (Apple Clang) [Debug]"
            os: macos-15-intel
            cxx: clang++
            expected: "clang++"
            build_type: Debug

    defaults:
      run:
        shell: bash

    steps:
      - name: Checkout repository
        uses: actions/checkout@v4

      - name: Set up Ninja
        uses: ninja-build/setup-ninja@v1

      - name: Install Linux build dependencies
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y gcc g++ clang

      - name: Verify compiler on PATH
        run: |
          which ${{ matrix.cxx }}
          ${{ matrix.cxx }} --version
          cmake --version
          ninja --version

      - name: Configure CMake
        run: |
          cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} -DCMAKE_CXX_COMPILER=${{ matrix.cxx }} -DBITFORGE_WARNINGS_AS_ERRORS=ON

      - name: Verify CMake-selected compiler in CMakeCache.txt
        run: |
          CXX_PATH=$(grep -E "^CMAKE_CXX_COMPILER(:.*)?=" build/CMakeCache.txt | cut -d'=' -f2)
          echo "Configured CMake CXX Compiler: $CXX_PATH"
          if [ -z "$CXX_PATH" ]; then
            echo "Error: CMAKE_CXX_COMPILER not found in build/CMakeCache.txt"
            exit 1
          fi
          if [[ "$CXX_PATH" != *"${{ matrix.expected }}"* ]]; then
            echo "Error: Expected compiler containing '${{ matrix.expected }}', got '$CXX_PATH'"
            exit 1
          fi

      - name: Build
        run: cmake --build build

      - name: Run CTest
        run: ctest --test-dir build --output-on-failure
```

*Status note: This workflow represents a proposed, designed workflow for repository implementation. Actual CI status will be reported only when the workflow is executed on GitHub.*

---

## 12. Reproducible ADS test interfaces and instruction budget

### 12.1 Instruction-budget policy and test harness execution contract

To prevent runaway loops or infinite hangs during automated integration testing:
1. **Explicit Budget Constants:**
   - Default integration budget: `constexpr uint64_t kDefaultInstructionBudget = 1'000'000;`
   - Algorithm benchmark budget: `constexpr uint64_t kBenchmarkInstructionBudget = 100'000;`
   - Micro/unit test budget: `constexpr uint64_t kMicroInstructionBudget = 1'000;`
2. **Harness Execution Loop Contract (`tests/test_harness.hpp`):**
   ```cpp
   template <typename CpuType>
   struct RunResult {
       bool success{false};
       uint64_t steps_executed{0};
       std::string stop_reason;
   };

   template <typename CpuType>
   RunResult<CpuType> run_with_budget(CpuType& cpu, uint64_t max_instructions = kDefaultInstructionBudget) {
       RunResult<CpuType> res;
       while ((cpu.state() == MachineState::Ready || cpu.state() == MachineState::Running) && res.steps_executed < max_instructions) {
           StepResult step_res = cpu.step();
           ++res.steps_executed;
           if (step_res.faulted || cpu.state() == MachineState::Faulted) {
               res.stop_reason = "Faulted";
               res.success = false;
               return res;
           }
           if (step_res.halted || cpu.state() == MachineState::Halted) {
               res.stop_reason = "Halted";
               res.success = true;
               return res;
           }
       }
       if (res.steps_executed >= max_instructions && cpu.state() != MachineState::Halted) {
           res.stop_reason = "InstructionBudgetExceeded";
           res.success = false;
           std::cerr << "TEST FAILURE: Instruction budget exceeded (" << max_instructions << " steps)\n"
                     << "  Machine State: " << to_string(cpu.state()) << "\n"
                     << "  Final PC: 0x" << std::hex << cpu.registers().pc << "\n"
                     << "  Opcode at PC: 0x" << (int)cpu.bus().read_byte(cpu.registers().pc) << "\n"
                     << "  R0: 0x" << (int)cpu.registers().r[0] << " R1: 0x" << (int)cpu.registers().r[1]
                     << " R2: 0x" << (int)cpu.registers().r[2] << " R3: 0x" << (int)cpu.registers().r[3] << "\n"
                     << "  IX: 0x" << cpu.registers().ix << " SP: 0x" << cpu.registers().sp << "\n"
                     << "  Flags: [Z=" << cpu.flags().z << " C=" << cpu.flags().c 
                     << " N=" << cpu.flags().n << " V=" << cpu.flags().v << "]\n";
       }
       return res;
   }
   ```
3. **Mandatory Execution Unit Tests in Phase 1:**
   - Test 1: Program starting in `Ready` executes and transitions state.
   - Test 2: Program executes and reaches `Halted` successfully.
   - Test 3: Intentional infinite loop (`loop: JMP loop`) terminates and fails when budget is exhausted.
   - Test 4: Faulted CPU (e.g. invalid opcode `0xFF`) stops immediately and produces rich diagnostics without hanging.

### 12.2 Reproducible ADS test interfaces

#### 1. Bubble Sort Benchmark (`03_bubble_sort.asm`)
- **Memory Contract:**
  - `0x8000`: Array length $N$ (unsigned byte, $0 \le N \le 255$).
  - `0x8001 .. 0x8000 + N`: Array element bytes.
- **Harness Setup:**
  - Harness writes length byte $N$ to `0x8000`, writes $N$ input bytes starting at `0x8001`.
  - Machine reset with `SP = 0xFFFF`. Run under budget `kBenchmarkInstructionBudget` until `Halted`.
- **Harness Verification:**
  - Asserts machine state is `Halted`.
  - Reads $N$ bytes from `0x8001 .. 0x8000 + N`.
  - Verifies non-decreasing order: `memory[i] <= memory[i + 1]`.
  - Verifies exact multiset conservation against input.
- **Reproducible Test Vectors:**
  1. *Empty Array ($N=0$):* Input: `[]` at `0x8001` (length `0x00` at `0x8000`) -> Output: `[]`.
  2. *Single-Element ($N=1$):* Input: `[0x2A]` (length `0x01`) -> Output: `[0x2A]`.
  3. *Two-Element Sorted ($N=2$):* Input: `[0x0A, 0x14]` (length `0x02`) -> Output: `[0x0A, 0x14]`.
  4. *Two-Element Reversed ($N=2$):* Input: `[0x14, 0x0A]` (length `0x02`) -> Output: `[0x0A, 0x14]`.
  5. *Duplicates ($N=5$):* Input: `[0x07, 0x07, 0x07, 0x07, 0x07]` (length `0x05`) -> Output: `[0x07, 0x07, 0x07, 0x07, 0x07]`.
  6. *Pre-Sorted ($N=8$):* Input: `[0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08]` (length `0x08`) -> Output: `[0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08]`.
  7. *Reverse-Sorted ($N=8$):* Input: `[0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01]` (length `0x08`) -> Output: `[0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08]`.
  8. *Pseudorandom Mixed ($N=7$):* Input: `[0x40, 0x22, 0x19, 0x0C, 0x16, 0x0B, 0x5A]` (length `0x07`) -> Output: `[0x0B, 0x0C, 0x16, 0x19, 0x22, 0x40, 0x5A]`.
  9. *Bounded Exhaustive Permutations ($N=4$):* All $4! = 24$ permutations of multiset $\{0x01, 0x02, 0x03, 0x04\}$ each independently verified to yield `[0x01, 0x02, 0x03, 0x04]`.

#### 2. Linear Search Benchmark (`02_linear_search.asm`)
- **Memory & Register Contract:**
  - `0x8000`: Array length $N$ (unsigned byte, $0 \le N \le 255$).
  - `0x8001`: Search target key (unsigned byte).
  - `0x8002 .. 0x8001 + N`: Array element bytes (occupies up to `0x8100` when $N=255$).
  - `0x8101`: Dedicated result storage byte (guaranteed non-overlapping with array).
  - Return: Register `R0` (and `memory[0x8101]`) contains 0-based index if found, or sentinel `0xFF` if absent.
- **Harness Verification:**
  Array: `[0x05, 0x12, 0x2A, 0x3F, 0x50]` (length `5` at `0x8000`).
  1. Key `0x05`: Returns `R0 == 0x00`, `memory[0x8101] == 0x00`.
  2. Key `0x2A`: Returns `R0 == 0x02`, `memory[0x8101] == 0x02`.
  3. Key `0x50`: Returns `R0 == 0x04`, `memory[0x8101] == 0x04`.
  4. Key `0x99`: Returns `R0 == 0xFF`, `memory[0x8101] == 0xFF`.

#### 3. Factorial Subroutine Benchmark (`04_factorial.asm`)
- **Memory & Calling Contract:**
  - `0x8000`: Input value $N$ (unsigned byte, $0 \le N \le 5$, written by test harness).
  - `0x8001`: Result storage byte $N!$ (written by program).
  - Calling discipline: Program loads $N$ from `[0x8000]` into `R0`, calls `factorial` subroutine via `CALL`, stores return value `R0` to `[0x8001]`, and executes `HALT`.
- **Harness Verification:**
  - Initial `SP` recorded at reset (`0xFFFF`). Harness writes $N$ to `memory[0x8000]`.
  - Runs under budget until `Halted`.
  - $0!$: Returns `R0 == 0x01`, `memory[0x8001] == 0x01`.
  - $1!$: Returns `R0 == 0x01`, `memory[0x8001] == 0x01`.
  - $5!$: Returns `R0 == 0x78` (`120`), `memory[0x8001] == 0x78`.
  - **Stack Balance Check:** Assert `SP == 0xFFFF` upon halt, proving zero stack leakage across all subroutine calls.

#### 4. Arithmetic Baseline (`01_arithmetic.asm`)
- Assembled program computes `10 + 20` and stores `0x1E` at address `0x8000`.
- Harness asserts `memory[0x8000] == 0x1E`, `R0 == 0x1E`, `Z=0, C=0, N=0, V=0`.

### 12.3 Measurable release conformance criteria
1. **Instruction Set Conformance:** Automated conformance unit tests pass for each of the **31 defined instruction mnemonics**, verifying documented semantics, flag side effects, and edge cases.
2. **Flag Verification:** Edge cases verified for `Z`, `C`, `N`, and `V` (signed overflow `0x7F + 1 = 0x80`, underflow `0x80 - 1 = 0x7F`, carry-out, borrow).
3. **Fault Atomicity:** All 5 defined faults (`InvalidOpcode`, `InvalidRegisterEncoding`, `InvalidPort`, `StackOverflow`, `StackUnderflow`) verified to preserve architectural state with `PC` remaining at the instruction start address.
4. **16-Bit Address Wraparound:**
   - Sequential PC rollover (`0xFFFF -> 0x0000`).
   - Multibyte instruction fetch straddling `0xFFFF` and `0x0000` (e.g. 3-byte `LDI R0, 42` loaded at `0xFFFE` fetching imm8 from `0x0000`, resulting in `PC = 0x0001`).
   - Branch target wrapping across boundaries.
   - Big-endian address encoding and decoding `(hi << 8) | lo`.
   - Boundary memory reads and writes at `0x0000` and `0xFFFF` without buffer overruns.
5. **Multi-Platform CI Gate:** Clean compilation and 100% CTest pass rate across all matrix jobs: Windows MSVC (Debug & Release), Linux GCC, Linux Clang, macOS Intel (`macos-15-intel`), and macOS Apple Silicon (`macos-15`).
6. **Code Coverage Policy:** Do not claim 100% source-code coverage unless a code coverage tool (`gcov` / `llvm-cov`) is explicitly configured, executed, and its report generated separately.

---

## 13. Repository structure and specifications location

Historical specifications are permanently preserved at the repository root:

```text
bitforge/
├── .github/workflows/ci.yml           # Multi-platform CI workflow
├── CMakeLists.txt                      # Root CMake build definition (LANGUAGES CXX)
├── CMakePresets.json                   # Standardized build/test presets
├── .gitignore                          # Git ignore rules
├── README.md                           # Project overview & quickstart
├── BUILDING.md                         # Multi-platform build manual
├── BitForge_Architecture_Specification_v1.0.md  # Preserved historical root spec
├── BitForge_Architecture_Specification_v1.1.md  # Preserved historical root spec
├── BitForge_Architecture_Specification_v1.2.md  # Preserved historical root spec
├── BitForge_Architecture_Specification_v1.3.md  # Preserved historical root spec
├── BitForge_Architecture_Specification_v1.4.md  # Preserved historical root spec
├── BitForge_Architecture_Specification_v1.5.md  # Preserved historical root spec
├── BitForge_Architecture_Specification_v1.6.md  # Preserved historical root spec
├── BitForge_Architecture_Specification_v1.7.md  # Approved active specification
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

## 14. Specification Changelog: v1.6 to v1.7

| Change ID | Affected Section in v1.7 | Semantic Decision Recorded |
|---|---|---|
| **CL-41** | Section 11.1 | **CI Workflow Syntax Fix:** Separated `windows-msvc` (`pwsh`) from `posix-build` (`bash`) jobs, eliminating invalid dynamic expression usage in GitHub Actions `defaults.run.shell`. |
| **CL-42** | Section 7.3, 12.1 | **CPU State Transition & Ready Execution Contract:** Explicitly documented that `step()` from `Ready` transitions to `Running` (or `Halted`/`Faulted`), and updated `run_with_budget()` to step while `Ready || Running`. Added four mandatory execution state unit tests. |
| **CL-43** | Section 12.2 | **Factorial Input Contract Alignment:** Formulated memory-based input contract passing $N$ via `memory[0x8000]` and receiving $N!$ at `memory[0x8001]`, resolving read-only register constraints after reset. |
| **CL-44** | Section 12.1 | **Explicit Instruction Budget Constants & Diagnostics:** Defined `kDefaultInstructionBudget`, `kBenchmarkInstructionBudget`, and `kMicroInstructionBudget`, and standardized rich failure diagnostics (state, PC, opcode, registers, IX, SP, flags). |
| **CL-45** | Section 11.1 | **Robust CMakeCache Compiler Verification:** Enhanced verification step in PowerShell and Bash to check `CMAKE_CXX_COMPILER` across variable cache patterns and compare against expected compiler name without error suppression. |
