# BitForge Architecture Specification v1.0

**Project:** BitForge — An 8-Bit Computer Emulator, Assembler and Interactive Debugging Workbench  
**Document status:** Proposed baseline for implementation  
**Primary language:** C++17 or later  
**Initial target:** Desktop application; the emulator core must also run without a GUI  
**Project context:** Individual Algorithm and Data Structures mini project

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

### 1.2 Explicit non-goals for v1.0

- Cycle-accurate or transistor-level simulation.
- A pipelined or out-of-order CPU.
- A complete commercial assembler or compiler.
- An operating system, interrupts, multitasking, or privilege modes.
- A full electrical simulator or hardware design suite.
- Multiple CPU architectures.

---

## 2. High-level components

BitForge will use distinct modules with narrow interfaces:

1. **CPU core:** instruction decoding and execution, registers, flags, program state, and faults.
2. **Memory:** 65,536 byte-addressable locations and program-image loading.
3. **Assembler:** source parsing, label resolution, instruction encoding, directives, and diagnostics.
4. **Virtual devices:** deterministic input queue, text/numeric output, and virtual LEDs.
5. **Debugger:** step/run/pause/reset, breakpoints, trace history, and symbol/source mapping.
6. **CLI runner:** a small interface for tests and headless execution.
7. **GUI workbench:** assembly editor, register view, memory view, stack view, trace, and device output.
8. **Example programs and tests:** known-good programs and automated CPU/assembler/integration tests.

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
| Interrupts | Not implemented in v1.0 |

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

Assembler-emitted bytes at `0xF000` or above are rejected in v1.0 to prevent normal program images from overwriting the stack region. CPU `LD`/`ST`/`LDX`/`STX` instructions still address the flat 16-bit memory space; v1.0 does not enforce memory protection at runtime.

A program may use `.ORG` to place code or data at an explicit address. Overlapping emitted bytes are an assembly error. The assembler rejects an emitted byte at or beyond `0xF000`.

### 3.5 Stack conventions

- Stack storage occupies `0xF000–0xFFFE`.
- An empty stack has `SP = 0xFFFF`.
- `PUSH` of one byte decrements `SP` by one, then stores the byte at `[SP]`.
- `POP` reads `[SP]`, then increments `SP` by one.
- A 16-bit value is stored big-endian: high byte at the lower address, low byte at the next address.
- `CALL` pushes the address of the instruction immediately following the `CALL`, then branches to the target.
- `RET` pops a 16-bit return address and loads it into `PC`.
- `PUSH`/`POP` operate on one 8-bit general register. `CALL`/`RET` use a 16-bit return address.
- Stack overflow and underflow cause a CPU fault. The failing instruction must not partially modify the stack, registers, or `PC`.

For `PUSH`/`CALL`, the core checks bounds before writing. For `POP`/`RET`, the core checks that enough bytes are available before reading. `POP`/`RET` do not track value types; using `POP` on bytes that were not pushed in the intended order is a program logic error, not a type-system error.

### 3.6 Byte order and address arithmetic

- 16-bit addresses embedded in instruction bytes are encoded big-endian: high byte first, low byte second.
- `.WORD` emits the high byte followed by the low byte.
- 16-bit stack values use the big-endian convention defined above.
- `PC`, `IX`, and address calculations wrap modulo `65,536` unless the instruction's stack-boundary checks fault first.

---

## 4. Instruction set and encoding

### 4.1 Encoding rules

Each instruction begins with a one-byte opcode. Operand bytes follow in the order shown in the table below. Register IDs are `R0 = 0`, `R1 = 1`, `R2 = 2`, and `R3 = 3`. Any other register ID is invalid. `IX` is a dedicated 16-bit register and is not encoded as a general register ID.

`addr16` and 16-bit immediates use big-endian byte order. All addresses are absolute in v1.0; relative branches are not supported.

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

The assembler must use this table as the single source of truth for instruction lengths and opcode encoding. The implementation should not duplicate inconsistent opcode definitions across the assembler, CPU, and GUI.

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

**AND, OR, XOR, NOT** update `Z` and `N` from their 8-bit result, and clear `C` and `V`.

**Other instructions** leave flags unchanged, unless an explicit future extension changes this contract. In particular, `LDI`, `MOV`, memory operations, `PUSH`, `POP`, `IN`, `OUT`, `INX`, and `DEX` do not alter flags in v1.0.

### 4.4 Branch meaning

- `JZ`: branch if the previous flag-setting operation left `Z = 1`.
- `JNZ`: branch if `Z = 0`.
- `JC`: branch if `C = 1`. Following `ADD`, this means carry-out; following `SUB` or `CMP`, it means no unsigned borrow (`left >= right`).
- `JNC`: branch if `C = 0`. Following `SUB` or `CMP`, this means unsigned `left < right`.

This dual interpretation of `C` is deliberate and must be explained in the user documentation. The assembler and debugger must not silently reinterpret it.

---

## 5. Virtual input/output devices

`IN` and `OUT` use port numbers rather than memory-mapped I/O. The first version implements these ports:

| Port | Direction | Behavior |
|---:|---|---|
| `0x00` | Input | Read the next byte from a preloaded input FIFO |
| `0x01` | Output | Append the byte as a character to the text console; non-printable bytes are displayed as escaped hexadecimal |
| `0x02` | Output | Append the register's unsigned decimal value to the numeric console stream |
| `0x10` | Output | Set the eight virtual LEDs to the written byte; each bit controls one LED |

If the input FIFO is empty, `IN` returns `0x00` and records an input-underflow event in the trace; it does not block. Access to an unimplemented port raises an `InvalidPort` CPU fault. The UI should display console output and LED state separately from ordinary debug logging.

The device layer is deterministic. It must not make tests depend on real-time keyboard input or physical hardware.

---

## 6. Assembly language specification

### 6.1 Lexical conventions

- Mnemonics and register names are case-insensitive.
- A semicolon (`;`) starts a comment that continues to the end of the line.
- A label has the form `name:` and begins with a letter or underscore; subsequent characters may be letters, digits, or underscores.
- Decimal literals are written normally, hexadecimal literals as `0xNN`, and binary literals as `0bNN`.
- Registers are written `R0` through `R3`; the index register is `IX`.
- Brackets indicate memory operands: `LD R0, [0x8000]`.
- Labels resolve to 16-bit absolute addresses.

### 6.2 Supported directives

| Directive | Meaning |
|---|---|
| `.ORG address` | Set the assembly location counter |
| `.ENTRY label` | Set the program entry address; at most one per source file |
| `.BYTE value, ...` | Emit one or more bytes |
| `.WORD value` | Emit a 16-bit value in big-endian order |
| `.ASCII "text"` | Emit the ASCII bytes of the string without an automatic null terminator |

The assembler supports simple numeric literals and label references in instruction address operands. The `.ENTRY` label must resolve to an emitted byte address outside the reserved stack region. If `.ENTRY` is omitted, address `0x0000` is used and must contain an emitted byte. General arithmetic expressions, macros, include files, and conditional assembly are outside v1.0.

### 6.3 Assembly pipeline

The assembler uses two passes:

1. **Pass 1:** tokenize and parse each statement, track the location counter, determine instruction/directive lengths, collect labels, identify duplicate labels, detect address overflow/overlap, and record source locations.
2. **Pass 2:** resolve label references, encode opcodes and operands, construct the program image, and produce diagnostics for unresolved symbols or invalid operands.

The assembler output is a `ProgramImage` containing:

- one or more address/byte segments;
- entry address;
- symbol table mapping labels to addresses;
- source-line mapping for emitted bytes/instructions;
- assembly diagnostics.

Errors include unknown mnemonic, invalid register, missing or extra operand, immediate out of range, invalid directive, duplicate label, unresolved label, overlapping output bytes, and output in the reserved stack region. The assembler must report a source line and, where possible, a column.

### 6.4 Example assembly program

```asm
; Add 10 and 20, then store the result at RESULT.
.ENTRY main
.ORG 0x0000

main:
    LDI R0, 10
    LDI R1, 20
    ADD R0, R1
    ST [RESULT], R0
    HALT

.ORG 0x8000
RESULT:
    .BYTE 0
```

After normal completion, RAM at `RESULT` should contain decimal 30 (`0x1E`). This program is a specification example, not evidence that an implementation has already passed tests.

---

## 7. CPU execution and fault model

### 7.1 Instruction step

A debugger step executes exactly one complete instruction. Internally, decoding uses a temporary instruction cursor so the CPU can validate the opcode and operand bytes before committing architectural state. A successful instruction commits its final register/memory/flag/PC state and creates one trace entry.

`PC` identifies the next instruction between completed instructions. A jump/call/return may replace the sequential PC. `HALT` advances PC past itself and transitions the machine to `Halted`.

### 7.2 Machine states

- `Ready`: program loaded and reset; execution has not started.
- `Running`: the run loop is active.
- `Paused`: execution stopped by a user pause or breakpoint.
- `Halted`: program executed `HALT`.
- `Faulted`: execution stopped due to an invalid opcode, invalid register encoding, invalid port, stack error, or other defined CPU fault.

The run loop stops on a breakpoint, pause request, `HALT`, CPU fault, or the configured instruction budget. The initial default budget is 1,000,000 instructions per run request to prevent runaway loops. The limit is configurable. Reaching the budget pauses execution and reports `InstructionLimitReached`; it is not a CPU instruction fault.

### 7.3 Fault behavior

Defined v1.0 faults include:

- `InvalidOpcode`
- `InvalidRegisterEncoding`
- `InvalidPort`
- `StackOverflow`
- `StackUnderflow`

The fault record contains fault type, instruction-start address, and a human-readable explanation. The failing instruction must not partially change registers, flags, stack, or memory. The debugger displays the faulting instruction address and preserves the last known-good machine state. The trace records the fault event. For invalid opcodes, `PC` remains at the offending instruction's start address.

Because the virtual memory space is fully allocated and initialized, every address in `0x0000–0xFFFF` is readable at the memory layer. Fetching from an address not emitted by the assembler therefore reads the current memory contents (zero after reset unless changed); the debugger should make this behavior visible rather than inventing an unmapped-memory fault.

---

## 8. Debugger and workbench requirements

### 8.1 Required controls

- Assemble source.
- Load/reset the assembled program.
- Run, pause, single-step, and reset.
- Set and remove breakpoints by source line or instruction address.
- Inspect `R0–R3`, `IX`, `PC`, `SP`, and `Z/C/N/V`.
- Inspect RAM by address in hexadecimal and decimal/ASCII interpretations.
- Inspect the stack from current `SP` toward higher addresses.
- View machine-code bytes and resolved symbols.
- View console output, LED state, faults, and trace history.

### 8.2 Trace entry

Each successful instruction trace entry records at least:

- instruction-start PC;
- instruction bytes and decoded mnemonic;
- register/flag/PC/SP/IX state before and after the step, or a reversible state delta;
- memory writes caused by the instruction;
- virtual I/O events;
- resulting machine state.

Trace storage must have a configurable limit to avoid unbounded memory growth. If older entries are discarded, the UI must indicate that history was truncated.

### 8.3 Visual presentation

The UI should highlight the current source line and machine instruction, display registers and flags in hexadecimal and binary where useful, show memory changes after stores, show stack changes around calls/returns, and separate program output from debugging messages.

The simplified datapath illustration is explanatory. It should reflect the current decoded instruction and operation; it should not imply cycle-accurate signals or electrical simulation.

---

## 9. ADS demonstration programs

The initial example set should include:

1. **Arithmetic and branches:** demonstrate `LDI`, `ADD`, `CMP`, conditional branches, and memory writes.
2. **Linear search:** search an array in RAM and return the found index or a documented not-found value.
3. **Bubble Sort:** sort a byte array in RAM using indexed reads/writes through `IX`.
4. **Factorial with a subroutine:** demonstrate `CALL`, `RET`, `PUSH`, and `POP`; a later extension may demonstrate recursion if the selected code and stack discipline support it.
5. **Output demonstration:** display character/numeric output and exercise the virtual LED port.

These examples are selected to connect the emulator to ADS topics. They are assembly programs for the fictional ISA, not host-language C++ implementations of the algorithms.

For algorithm benchmarks, record both the algorithm's input-size-dependent behavior and the emulator's instruction count. Instruction count is a repeatable simulator metric; wall-clock execution time mostly measures the host computer and software implementation, so it must be interpreted separately.

---

## 10. Testing and acceptance criteria

### 10.1 Unit tests

- Each opcode has tests for normal behavior and relevant edge cases.
- Arithmetic tests cover zero, carry, unsigned borrow, signed overflow, and wraparound.
- Flags are tested independently from the resulting register value.
- Stack tests cover empty/full boundaries, nested calls, and return-address byte order.
- The assembler tests every instruction form, directive, label-resolution path, and major diagnostic category.
- Device tests cover queued input, empty input, character output, numeric output, LED updates, and invalid ports.

### 10.2 Integration tests

- Assemble and execute the arithmetic example; verify the result byte is `0x1E`.
- Execute a branching loop and verify its final register/memory state.
- Execute linear search on found and absent values.
- Execute Bubble Sort on empty, one-element, duplicate, sorted, reverse-sorted, and mixed arrays. The result must be sorted and retain exactly the original multiset of bytes.
- Execute a subroutine example and verify that `SP` is restored after a balanced call/return sequence.
- Confirm that breakpoints, stepping, reset, HALT, and faults produce consistent state and trace information.

### 10.3 Release acceptance checklist

BitForge v1.0 is ready for the project demonstration only when:

- [ ] The CPU, assembler, and test runner build from a clean checkout.
- [ ] All implemented opcodes have automated tests.
- [ ] The assembler can resolve forward labels and emit a valid `ProgramImage`.
- [ ] The arithmetic example passes in both CLI and GUI mode.
- [ ] The debugger can single-step and inspect register/memory state.
- [ ] Bubble Sort executes on the emulated CPU and sorts all selected test arrays correctly.
- [ ] Stack operations and subroutine calls pass boundary tests.
- [ ] Runtime errors produce useful, reproducible diagnostics rather than crashes.
- [ ] The README explains build/run instructions, architecture, instruction set, known limitations, and the demo sequence.
- [ ] The final report distinguishes implemented, tested features from planned or incomplete extensions.

---

## 11. Recommended implementation order

1. Lock this specification before coding.
2. Build a small headless C++ core and unit-test the memory, registers, ALU, and instruction stepping.
3. Implement the assembler and use it to generate programs for the CPU.
4. Add stack behavior, subroutines, virtual devices, faults, and the CLI runner.
5. Implement the linear-search and Bubble Sort assembly examples and test them thoroughly.
6. Add debugger controls and trace recording.
7. Build the GUI on top of the tested core.
8. Polish visualizations, write documentation, and prepare the live demonstration.

Do not implement the GUI before the CPU/assembler contract is stable. Keeping the core headless and testable reduces debugging risk and makes failures easier to isolate.

---

## 12. Decisions deliberately deferred

The following are possible future extensions, not v1.0 requirements:

- a disassembler;
- control-flow graph generation and graph traversal;
- a virtual timer or interrupt model;
- memory-mapped devices;
- an interactive ALU/datapath visualization;
- an executable-image file format;
- a configurable CPU profile;
- pipeline stages or cycle-accurate execution.

Any extension must preserve the v1.0 instruction semantics unless a documented version change is made.

---

## 13. Versioning and change control

This document defines the proposed v1.0 contract for the implementation. If a change affects instruction lengths, opcode values, flags, stack behavior, assembler grammar, memory conventions, or virtual-device semantics, update this specification before changing the implementation. Tests and examples must be updated alongside the specification.

**Implementation status:** Not yet implemented or verified. This document is the blueprint and acceptance contract; implementation claims should only be made after corresponding tests pass.
