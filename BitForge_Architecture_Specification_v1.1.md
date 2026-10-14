# BitForge Architecture Specification v1.1

**Project:** BitForge — An 8-Bit Computer Emulator, Assembler and Interactive Debugging Workbench  
**Document status:** Approved architectural baseline  
**Primary language:** C++17 or later  
**Initial target:** Desktop application; the emulator core must also run without a GUI  
**Project context:** Individual Algorithm and Data Structures mini project  
**Predecessor document:** BitForge Architecture Specification v1.0 (preserved unchanged)

---

## 1. Purpose and product vision

BitForge is an educational software model of a small fictional computer. Users write programs in BitForge Assembly, assemble them into machine code, run them on an emulated CPU, and inspect execution through a debugger and visual workbench.

The signature demonstration is a Bubble Sort program written in BitForge Assembly and executed by the emulated processor. The application should let an observer see the current instruction, CPU registers and flags, memory changes, stack activity, and execution trace.

BitForge is an instruction-set simulator. It is **not** intended to simulate transistor-level behavior, electrical timing, or a physical processor's exact microarchitecture. Its CPU semantics will be deterministic and documented.

### 1.1 Primary goals

1. Define a small, internally consistent 8-bit instruction set.
2. Execute assembled machine code accurately and deterministically.
3. Provide an assembler with labels, directives, and useful diagnostics.
4. Support single-stepping, breakpoints, tracing, and CPU/memory inspection.
5. Demonstrate ADS concepts through programs that run on the simulated CPU.
6. Test core behavior independently from the graphical interface.

### 1.2 Explicit non-goals for v1.1

- Cycle-accurate or transistor-level simulation.
- A pipelined or out-of-order CPU.
- A complete commercial assembler or compiler.
- An operating system, interrupts, multitasking, or privilege modes.
- A full electrical simulator or hardware design suite.
- Multiple CPU architectures.

---

## 2. High-level components

BitForge uses distinct modules with narrow interfaces:

1. **CPU core:** instruction decoding and execution, registers, flags, program state, and faults.
2. **Memory:** 65,536 byte-addressable locations and program-image loading.
3. **Machine bus:** minimal bus interface routing memory access and virtual port I/O.
4. **Assembler:** source parsing, label resolution, instruction encoding, directives, and diagnostics.
5. **Virtual devices:** deterministic input queue, text/numeric output, and virtual LEDs.
6. **Debugger:** step/run/pause/reset, breakpoints, trace history, and symbol/source mapping.
7. **CLI runner:** a small interface for tests and headless execution.
8. **GUI workbench:** assembly editor, register view, memory view, stack view, trace, and device output.
9. **Example programs and tests:** known-good programs and automated CPU/assembler/integration tests.

The CPU core must not depend on Qt or any other GUI framework. The graphical interface observes and controls the core through the debugger API; it must not implement CPU semantics itself.

---

## 3. Machine architecture

### 3.1 Data model

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
| Interrupts | Not implemented in v1.1 |

All general-register arithmetic produces an 8-bit result. Results outside `0..255` wrap modulo 256, with flags recording the relevant condition.

### 3.2 Registers

- **R0–R3:** General-purpose 8-bit registers.
- **IX:** 16-bit index register for indirect byte-array access. `LDI16`, `LDX`, `STX`, `INX`, and `DEX` operate on it.
- **PC:** Address of the next instruction to fetch between completed instructions.
- **SP:** Stack pointer. It points to the next free stack location; the stack grows toward lower addresses.
- **Z (Zero):** Set when the operation's 8-bit result is zero.
- **C (Carry/no-borrow):** Set to carry-out for addition; set when subtraction/comparison requires no unsigned borrow (left operand is greater than or equal to right operand).
- **N (Negative):** Copies bit 7 of the operation's 8-bit result.
- **V (Signed overflow):** Set when signed two's-complement arithmetic overflows the range `-128..127`.

Flags are Boolean. Reset clears all flags. Instructions not specified to modify flags leave them unchanged.

### 3.3 Initial/reset state

On a full machine reset:

- `R0..R3 = 0`
- `IX = 0`
- `PC = program entry address`
- `SP = 0xFFFF` (empty-stack sentinel / next free byte)
- `Z = C = N = V = 0`
- machine state is `Ready`
- trace, console output, LED state, and fault state are cleared
- memory is initialized to zero before the assembled image is loaded

The entry address comes from the `.ENTRY` directive. If it is omitted, the default entry address is `0x0000`.

### 3.4 Memory conventions

The entire address space is byte-addressable. The following regions are conventions enforced by the assembler where stated; they are not a memory-protection system inside the CPU.

| Address range | Convention |
|---|---|
| `0x0000–0x7FFF` | Program code and small constants |
| `0x8000–0xEFFF` | Static data and arrays |
| `0xF000–0xFFFE` | Reserved stack storage |
| `0xFFFF` | Stack-pointer empty sentinel; not available to assembler-emitted program data |

Assembler-emitted bytes at `0xF000` or above are rejected to prevent normal program images from overwriting the stack region. CPU `LD`/`ST`/`LDX`/`STX` instructions still address the flat 16-bit memory space; v1.1 does not enforce memory protection at runtime.

A program may use `.ORG` to place code or data at an explicit address. Overlapping emitted bytes are an assembly error. The assembler rejects an emitted byte at or beyond `0xF000`.

### 3.5 Stack conventions and boundary validation

The stack grows toward lower addresses within the reserved storage space `0xF000`–`0xFFFE`. Address `0xFFFF` is the empty-stack sentinel.

All stack operations must validate all required boundaries **before** committing any register, memory, SP, or PC modification:

1. **Empty stack sentinel:** An empty stack has `SP = 0xFFFF`.
2. **`PUSH Rs` boundary rule:** Requires 1 free stack byte. The target address is `SP - 1`.
   - Condition: If `SP <= 0xF000`, decrementing would produce an address `< 0xF000`. The CPU immediately triggers a `StackOverflow` fault.
   - Commitment: Only if `SP > 0xF000` does the CPU decrement `SP` by one, then write `Rs` to `memory[SP]`.
3. **`POP Rd` boundary rule:** Requires 1 pushed byte. The source address is `SP`.
   - Condition: If `SP >= 0xFFFF`, the stack contains no pushed data. The CPU immediately triggers a `StackUnderflow` fault.
   - Commitment: Only if `SP < 0xFFFF` does the CPU read `memory[SP]`, store it into `Rd`, then increment `SP` by one.
4. **`CALL addr16` boundary and storage rule:** Requires 2 free stack bytes. Pushes the 16-bit address of the instruction immediately following `CALL`.
   - Condition: If `SP < 0xF002`, pushing 2 bytes would require addresses `< 0xF000`. The CPU immediately triggers a `StackOverflow` fault.
   - Storage format (Big-Endian): High byte stored at lower address, low byte at next higher address.
     - Let `ret_addr` be the 16-bit return address (`ret_hi = (ret_addr >> 8) & 0xFF`, `ret_lo = ret_addr & 0xFF`).
     - Memory writes: `memory[SP - 2] = ret_hi`, `memory[SP - 1] = ret_lo`.
     - `SP` is updated: `SP = SP - 2` (so `SP` points directly to `ret_hi` at the lower address, and `SP + 1` contains `ret_lo`).
     - `PC` is set to `addr16`.
5. **`RET` boundary and storage rule:** Requires at least 2 pushed bytes on the stack.
   - Condition: If `SP > 0xFFFD` (`SP == 0xFFFE` or `SP == 0xFFFF`), fewer than 2 bytes are available. The CPU immediately triggers a `StackUnderflow` fault.
   - Commitment: Pops high byte from `memory[SP]`, increments `SP`, pops low byte from `memory[SP]`, increments `SP`, and reconstructs `PC = (ret_hi << 8) | ret_lo`.
6. **Atomicity:** In any stack fault, `SP`, `PC`, general registers, flags, and memory remain unchanged.

### 3.6 Byte order and address arithmetic

- 16-bit addresses embedded in instruction bytes are encoded big-endian: high byte first, low byte second.
- `.WORD` emits the high byte followed by the low byte.
- 16-bit stack values use big-endian storage as defined in Section 3.5.
- `PC`, `IX`, and address calculations wrap modulo `65,536` unless the instruction's stack-boundary checks fault first.

---

## 4. Instruction set and encoding

### 4.1 Encoding rules

Each instruction begins with a one-byte opcode. Operand bytes follow in the order shown in the table below. Register IDs are `R0 = 0`, `R1 = 1`, `R2 = 2`, and `R3 = 3`. Any other register ID is invalid and triggers an `InvalidRegisterEncoding` fault. `IX` is a dedicated 16-bit register and is not encoded as a general register ID.

`addr16` and 16-bit immediates use big-endian byte order. All addresses are absolute in v1.1; relative branches are not supported.

### 4.2 Opcode table

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

The assembler must use this table as the single source of truth for instruction lengths and opcode encoding.

### 4.3 Arithmetic and flag semantics

Let `a` be the original destination/left operand, `b` the source/right operand, and `r` the 8-bit result.

**ADD**
- `wide = a + b`; `r = wide & 0xFF`
- `Z = (r == 0)`
- `N = bit 7 of r`
- `C = (wide > 0xFF)`
- `V = ((~(a ^ b) & (a ^ r) & 0x80) != 0)`
- destination register receives `r`

**SUB**
- `r = (a - b) & 0xFF`
- `Z = (r == 0)`
- `N = bit 7 of r`
- `C = (a >= b)` (one means no unsigned borrow)
- `V = (((a ^ b) & (a ^ r) & 0x80) != 0)`
- destination register receives `r`

**CMP** uses the same flags as `SUB` for the left and right operands, but does not write the subtraction result to a register.

**INC** updates the register modulo 256 and sets `Z`, `N`, and `V`. `V` is set when `0x7F` becomes `0x80`. `C` is unchanged.

**DEC** updates the register modulo 256 and sets `Z`, `N`, and `V`. `V` is set when `0x80` becomes `0x7F`. `C` is unchanged.

**AND, OR, XOR, NOT** update `Z` and `N` from their 8-bit result, and clear `C` and `V` (`C = 0, V = 0`).

**Other instructions** leave flags unchanged in v1.1 (`LDI`, `MOV`, memory operations, `PUSH`, `POP`, `IN`, `OUT`, `INX`, `DEX`, branches, calls, returns).

### 4.4 Branch meaning

- `JZ`: branch if the previous flag-setting operation left `Z = 1`.
- `JNZ`: branch if `Z = 0`.
- `JC`: branch if `C = 1` (carry-out after ADD; no unsigned borrow / `left >= right` after SUB/CMP).
- `JNC`: branch if `C = 0` (no carry-out after ADD; unsigned borrow / `left < right` after SUB/CMP).

---

## 5. Virtual input/output devices

`IN` and `OUT` communicate through dedicated 8-bit port numbers. The defined ports in v1.1 are:

| Port | Direction | Behavior |
|---:|---|---|
| `0x00` | Input | Read the next byte from a preloaded input FIFO |
| `0x01` | Output | Append the byte as a character to the text console; non-printable bytes displayed as escaped hexadecimal (`\xNN`) |
| `0x02` | Output | Append the register's unsigned decimal value (`0..255`) to the numeric console stream |
| `0x10` | Output | Set the eight virtual LEDs to the written byte; each bit controls one LED |

### 5.1 Directionality and error semantics

1. **Port direction enforcement:**
   - Attempting `OUT` to an input-only port (e.g. `OUT 0x00, Rs`) raises an `InvalidPort` CPU fault.
   - Attempting `IN` from an output-only port (e.g. `IN Rd, 0x01`, `0x02`, or `0x10`) raises an `InvalidPort` CPU fault.
2. **Unimplemented ports:** Access to any port other than `0x00`, `0x01`, `0x02`, or `0x10` raises an `InvalidPort` CPU fault.
3. **Empty input FIFO:** If the input FIFO on port `0x00` is empty when `IN` executes:
   - It is a **non-faulting** condition.
   - `IN` returns `0x00` into `Rd`.
   - CPU flags are unchanged.
   - An `InputUnderflow` event is recorded in the trace.
   - Execution continues without blocking.

---

## 6. Assembly language specification

### 6.1 Lexical conventions and numeric ranges

- Mnemonics and register names are case-insensitive.
- Semicolon (`;`) starts a line comment.
- Labels follow `name:` at definition, beginning with a letter or underscore, followed by letters, digits, or underscores.
- **Numeric literal formats:**
  - Decimal: written normally (e.g. `42`, `-10`).
  - Hexadecimal: prefixed with `0x` or `0X` (e.g. `0x2A`).
  - Binary: prefixed with `0b` or `0B` (e.g. `0b00101010`).
- **8-bit Immediate range (`imm8` and `.BYTE`):**
  - Permitted range: `-128` through `255` inclusive.
  - Negative values (`-128..-1`) are encoded using standard 8-bit two's complement (`0x80..0xFF`).
  - Positive values (`0..255`) are encoded directly as unsigned bytes.
  - Values outside `[-128, 255]` produce an assembly diagnostic ("immediate value out of range").
- **16-bit Immediate range (`.WORD`):**
  - Permitted range: `-32768` through `65535` inclusive.
  - Negative values (`-32768..-1`) are encoded using standard 16-bit two's complement (`0x8000..0xFFFF`).
  - Positive values (`0..65535`) are encoded directly as unsigned words.
  - Values are emitted in big-endian byte order (high byte first).
  - Values outside `[-32768, 65535]` produce an assembly diagnostic.

### 6.2 Supported directives

| Directive | Meaning | Valid Argument Range |
|---|---|---|
| `.ORG address` | Set the assembly location counter | `0x0000`–`0xEFFF` |
| `.ENTRY label` | Set the program entry address; at most one per source file | Resolved label must be `< 0xF000` |
| `.BYTE val, ...` | Emit one or more bytes | Each value in `-128..255` |
| `.WORD val` | Emit a 16-bit value in big-endian order | Value in `-32768..65535` |
| `.ASCII "text"` | Emit ASCII bytes without an automatic null terminator | Printable ASCII string; standard escapes `\n`, `\t`, `\r`, `\\`, `\"`, `\0` supported |

The assembler rejects any emitted byte at or beyond `0xF000` (the reserved stack space).

### 6.3 Assembly pipeline

1. **Pass 1:** Tokenize and parse statements, track location counter, compute statement lengths, collect label definitions, identify duplicate labels, detect address overflow/overlap, and record source locations.
2. **Pass 2:** Resolve label references into absolute 16-bit addresses, encode opcodes and operands, construct the `ProgramImage`, and emit diagnostics for unresolved symbols or invalid operands.

---

## 7. CPU execution and fault model

### 7.1 Instruction step

A debugger step executes exactly one complete instruction. Internally, decoding uses an instruction cursor to validate opcode, operands, and boundary preconditions before committing any architectural state change.

A successful instruction commits its register/memory/flag/PC mutations and records one trace entry. `PC` advances past the instruction, or is loaded with the branch/call/return target. `HALT` advances `PC` past itself by one byte and transitions the machine state to `Halted`.

### 7.2 Machine states

- `Ready`: program loaded and reset; execution has not started.
- `Running`: the run loop is active.
- `Paused`: execution stopped by a user pause, breakpoint, or instruction limit.
- `Halted`: program executed `HALT`.
- `Faulted`: execution stopped due to a defined CPU fault.

### 7.3 Fault behavior and architectural atomicity

Defined faults in v1.1 include:

- `InvalidOpcode`: Unrecognized opcode byte.
- `InvalidRegisterEncoding`: Register operand byte not in `0..3`.
- `InvalidPort`: Access to an unmapped port, or accessing a valid port in the wrong direction (`IN` from output port or `OUT` to input port).
- `StackOverflow`: Stack boundary violated on `PUSH` or `CALL`.
- `StackUnderflow`: Stack boundary violated on `POP` or `RET`.

**Architectural preservation on fault:**
When any fault occurs:
1. **Instruction atomicity:** The failing instruction commits **no** partial changes. Registers (`R0..R3`, `IX`), memory, flags (`Z`, `C`, `N`, `V`), and stack pointer (`SP`) remain in their state prior to the instruction attempt.
2. **Program Counter (`PC`):** For all faults without exception, `PC` remains at the start address of the faulting instruction.
3. **Machine State:** The machine transitions to `Faulted`.
4. **Fault Record:** A fault record is generated containing the fault type, instruction-start `PC`, and an explanatory message.
5. **Trace Recording:** The fault event is recorded in the trace history.

---

## 8. Debugger and workbench requirements

### 8.1 Required controls

- Assemble source.
- Load/reset the assembled program.
- Run, pause, single-step, and reset.
- Set and remove breakpoints by source line or instruction address.
- Inspect `R0–R3`, `IX`, `PC`, `SP`, and `Z/C/N/V`.
- Inspect RAM by address in hexadecimal and decimal/ASCII interpretations.
- Inspect stack storage from current `SP` toward higher addresses (`0xFFFF`).
- View machine-code bytes and resolved symbols.
- View console output, LED state, faults, and trace history.

### 8.2 Trace entry

Each instruction trace entry records:
- instruction-start `PC`;
- instruction bytes and decoded mnemonic;
- register/flag/PC/SP/IX state before and after the step (or reversible state delta);
- memory writes caused by the instruction;
- virtual I/O events;
- resulting machine state.

Trace storage must have a configurable limit to prevent unbounded memory consumption.

---

## 9. ADS demonstration programs

1. **Arithmetic and branches:** demonstrate `LDI`, `ADD`, `CMP`, conditional branches, and memory write storing `0x1E` at `RESULT`.
2. **Linear search:** search an array in RAM and return found index or documented sentinel.
3. **Bubble Sort:** sort a byte array in RAM using indexed reads/writes through `IX`.
4. **Factorial with a subroutine:** demonstrate `CALL`, `RET`, `PUSH`, and `POP` with verified balanced stack restoration.
5. **Output demonstration:** exercise text console, numeric stream, and virtual LEDs.

---

## 10. Testing and acceptance criteria

### 10.1 Phase 1 Unit Acceptance Gate
- Automated tests pass for every opcode and component implemented in Phase 1 (memory, ALU, core register movement, arithmetic, logic, branching, basic faults).
- Test execution driven via standard CMake/CTest.
- Instructions not yet implemented in Phase 1 do not block Phase 1 completion, but are tested as introduced in subsequent phases.

### 10.2 Full Release Acceptance Gate
BitForge v1.1 is ready for final release only when:
- [ ] The CPU, assembler, and test runner build cleanly via CMake.
- [ ] 100% of defined opcodes have automated unit tests.
- [ ] The assembler resolves forward/backward labels and emits a valid `ProgramImage`.
- [ ] All ADS demonstration programs pass on the simulated CPU.
- [ ] Bubble Sort sorts empty, single-element, duplicate, sorted, reverse-sorted, and mixed arrays.
- [ ] All fault conditions produce accurate diagnostics with atomic state preservation.
- [ ] Full documentation and build instructions are provided.

---

## 11. Recommended implementation order

1. Finalize and freeze Specification v1.1.
2. Establish modern C++17 build environment with CMake and CTest.
3. Build headless CPU core, memory, ALU, and bus interface; verify with automated unit tests.
4. Implement stack operations, boundary validation, and virtual I/O devices.
5. Implement two-pass assembler and verify encoding, directives, and diagnostics.
6. Implement debugger controller, trace engine, and headless CLI runner.
7. Implement and verify the ADS benchmark assembly programs.
8. Implement GUI workbench on top of the tested core.

---

## 12. Decisions deliberately deferred

- Disassembler.
- Control-flow graph generation.
- Virtual timer or interrupt model.
- Memory-mapped devices.
- Interactive ALU/datapath electrical visualization.
- Executable image binary file format.
- Cycle-accurate execution.

---

## 13. Versioning and change control

Any changes affecting instruction lengths, opcodes, flags, stack behavior, assembler grammar, memory conventions, or device semantics require a documented specification version bump.

---

## 14. Specification Changelog: v1.0 to v1.1

| Change ID | Affected Section | Semantic Decision Recorded |
|---|---|---|
| **CL-01** | Section 3.5 | **Stack Boundary Validation:** Explicitly defined boundary preconditions for `PUSH`, `POP`, `CALL`, and `RET`. Mandated that all boundaries be validated before committing any register, memory, SP, or PC modification. |
| **CL-02** | Section 3.5, 3.6 | **Big-Endian Stack Return Addresses:** Explicitly specified that `CALL` pushes return address with high byte at lower address (`SP`) and low byte at `SP + 1`. `RET` pops high byte first, low byte second. |
| **CL-03** | Section 7.3 | **Universal PC Preservation on Fault:** Mandated that all CPU faults (not just invalid opcodes) preserve `PC` at the instruction-start address and leave architectural state unmodified. |
| **CL-04** | Section 5.1 | **Port Directionality Enforcement:** Specified that accessing a port in the wrong direction (`OUT` to Port 0x00, or `IN` from Ports 0x01, 0x02, 0x10) triggers an `InvalidPort` CPU fault. |
| **CL-05** | Section 5.1 | **Non-Faulting FIFO Underflow:** Confirmed that reading from an empty Port 0x00 input FIFO returns `0x00`, leaves flags unchanged, logs an `InputUnderflow` trace event, and does not fault. |
| **CL-06** | Section 6.1 | **8-Bit Immediate Range:** Specified that `imm8` and `.BYTE` accept literals from `-128` through `255`, encoding negative values via 8-bit two's complement. |
| **CL-07** | Section 6.1 | **16-Bit Immediate Range:** Specified that `.WORD` accepts literals from `-32768` through `65535`, encoding negative values via 16-bit two's complement in big-endian byte order. |
| **CL-08** | Section 2, 10.1 | **Bus Architecture & Phase 1 Scope:** Added bus interface to high-level components and scoped Phase 1 acceptance gates to Phase 1 opcodes driven by CTest. |
