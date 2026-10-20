# BitForge Quality Assurance Audit — Phase 4: Assembler, Directives, and Symbol Resolution Audit

- **Document Version:** 1.3 (Phase 4.3 Final Sign-Off Deliverable)
- **Audit Date:** October 2026
- **Auditor:** Quality Assurance & Verification Audit Team (Antigravity-assisted)
- **Repository:** `D:\BitForge`
- **Git HEAD Commit:** `5a88e11`
- **Primary Authorities:**
  - `docs/BitForge_Architecture_Specification_v1.7.md` (Frozen)
  - `docs/decisions/ADR-001-stack-pointer-convention.md` (Frozen)
  - `docs/QA_PHASE0_BASELINE_REPORT.md`
  - `docs/QA_PHASE1_SPEC_CONFORMANCE_REPORT.md`
  - `docs/QA_PHASE2_CPU_EDGE_CASE_REPORT.md` (Version 1.3)
  - `docs/QA_PHASE3_MEMORY_BUS_STACK_REPORT.md` (Version 1.2)

---

## 1. Executive Summary

Phase 4 of the BitForge Final QA Roadmap conducts a specification-driven verification of the two-pass assembler (`bitforge::assembler::Assembler`), lexical analyzer (`Lexer`), symbol table (`SymbolTable`), program image builder (`ProgramImage`), source-to-instruction mapping database (`SourceMap`), and diagnostic reporting engine (`Diagnostic`). Phase 4.1 and Phase 4.2 conduct focused reconciliation passes verifying the failed-assembly `ProgramImage` contract, auditing diagnostic source-location assertions, reconciling repository-state reporting, and aligning the Phase 5/Phase 6 roadmaps with Phase 0 and Phase 3 authorities.

The audit establishes empirical evidence for all 31 canonical BitForge instruction encodings, directive semantics (`.ORG`, `.ENTRY`, `.BYTE`, `.WORD`, `.ASCII`), expression evaluation, case-insensitive symbol resolution, address boundary enforcement, and malformed input handling. In addition, Phase 4 resolves `RISK-P3-002`, verifying the assembler's enforcement of the boundary between general RAM and the reserved stack region ($[0xF000, 0xFFFF]$).

### Key Audit Findings

1. **Instruction Encoding Conformance (31/31 Canonical Instructions):** All 31 instruction mnemonics, operand formats, opcode assignments, and register encodings ($R_0..R_3$, $IX$) conform to v1.7 §4 and §13. Every instruction encoding was validated against independently derived binary reference vectors.
2. **Directive Semantics & Big-Endian Layout:**
   - `.ORG` correctly repositions the location counter within $[0x0000, 0xEFFF]$ and rejects origins in $[0xF000, 0xFFFF]$ or out-of-range addresses.
   - `.ENTRY` correctly accepts labels and numeric addresses, enforces uniqueness, rejects targets in the reserved stack region ($[0xF000, 0xFFFF]$), and rejects entry points that target uninitialized memory gaps outside emitted segments. When omitted, the default entry point $0x0000$ conforms strictly to v1.7 §5.3.
   - `.BYTE` and `.WORD` correctly emit single bytes and big-endian 16-bit words, supporting signed and unsigned values and rejecting range violations.
   - `.ASCII` correctly emits literal bytes and supports standard escape sequences (`\n`, `\t`, `\r`, `\\`, `\"`, `\0`).
3. **Symbol Resolution & Case-Insensitive Matching:** `SymbolTable` normalizes identifiers to uppercase, providing case-insensitive label definition and lookup. Forward and backward label references, label offsets, and duplicate label detection across mixed casing are verified.
4. **Stack Boundary & Collision Enforcement (`RISK-P3-002`):** The assembler prevents emitting program bytes into the reserved stack region ($[0xF000, 0xFFFF]$). The audit confirms that exact-fit instructions and data ending at $0xEFFF$ (including 1-byte, 2-byte, 3-byte, and 4-byte instructions) assemble successfully into valid segments. Conversely, any instruction or directive whose bytes would land at or above $0xF000$ fails assembly with an error diagnostic. Under the public assembler contract (`Assembler::assemble`), any assembly error causes `AssembleResult::success` to be `false`, and `AssembleResult::image` is returned as a default-constructed, empty `ProgramImage` containing 0 segments and 0 total bytes. Even if earlier valid statements precede the overrun, no partial or corrupted segments are returned in the image, and attempting to load the failed image into `Memory` mutates zero bytes.
5. **Diagnostic Quality & Malformed Input Handling:** 31 distinct syntax, register, operand, punctuation, and numeric error conditions are systematically exercised in `AssemblerTest.MalformedInputComprehensiveMatrix`, confirming universal rejection (`success == false`) and diagnostic emission. Precise 1-based source line numbers and error text classifications are asserted across dedicated fixtures (`DuplicateLabelError`, `UnresolvedLabelError`, `InvalidRegisterDiagnostic`, `OutOfRangeImmediateDiagnostic`, `DiagnosticSourceLocationsPrecision`). Unasserted column coordinates in the 31-snippet matrix are identified as an evidence gap rather than declared conformant.
6. **Example Assembly Programs Smoke Test:** `examples/01_arithmetic.asm` (14 bytes) and `examples/02_io_demo.asm` (31 bytes) assemble cleanly and execute to normal `Halted` states with expected register, memory, and peripheral outputs on both CLI and emulator core.
7. **Test Suite Expansion & Multi-Preset Verification:** The test suite expanded from 265 to 272 tests (+7 new comprehensive verification fixtures, bringing `test_assembler.cpp` to 41 test cases). All 272 tests pass under all 4 CMake presets (`ci`, `debug`, `gui`, `gui-release`), while all 83 Qt GUI test slots pass under `gui` and `gui-release`.

---

## 2. Scope & Authority Reference Matrix

The BitForge architecture specifications are frozen. The assembler audit evaluated the implementation against the following normative requirements:

| Specification Document | Section / Topic | Normative Requirement | Assembler Implementation | Conformance Status |
| :--- | :--- | :--- | :--- | :--- |
| `v1.7.md` | §4.1–§4.5 | Canonical instruction set (31 instructions, lengths 1–4 bytes) | `src/assembler/assembler.cpp` (`parse_instruction`) | **Conformant** |
| `v1.7.md` | §5.2 | Address space partitioning ($0x0000..0xEFFF$ RAM, $0xF000..0xFFFF$ Stack) | `kStackReservedStart = 0xF000`, `emit_byte` checks | **Conformant** |
| `v1.7.md` | §5.3 | Program entry point specification and default to $0x0000$ | `.ENTRY` parsing and default `0x0000` initialization | **Conformant** |
| `v1.7.md` | §5.4 | Big-endian byte order for multi-byte values and addresses | `emit_word` (high byte then low byte) | **Conformant** |
| `v1.7.md` | §13 | Opcode enumeration and operand encoding layouts | `Opcode` enum and opcode dispatch tables | **Conformant** |
| `ADR-001` | §2 | Stack region reservation ($0xF000..0xFFFF$) | Stack region rejection in `.ORG`, `.ENTRY`, and `emit_byte` | **Conformant** |

---

## 3. Lexical Analysis & Literal Formats Audit

The lexical analyzer (`bitforge::assembler::Lexer`) translates UTF-8/ASCII assembly source into a sequential token stream (`Token`).

### 3.1 Numeric Literals

The lexer supports three numeric bases:
1. **Decimal:** Unadorned digits `[0-9]+`, optional unary negation (`-128..255`, `-32768..65535`).
2. **Hexadecimal:** `0x` or `0X` prefix followed by hex digits `[0-9a-fA-F]+` (`0x00..0xFFFF`).
3. **Binary:** `0b` or `0B` prefix followed by binary digits `[0-1]+` (`0b00000000..0b11111111`).

Range checking during token scanning uses `std::strtoull` with `errno == ERANGE` detection. Out-of-range integer literals produce error diagnostics rather than silent truncation.

### 3.2 String and Character Literals

- Strings are delimited by double quotes (`"..."`). Unterminated strings (interrupted by newline or EOF) are reported with clear diagnostics.
- Supported escape sequences: `\n` (newline, `0x0A`), `\t` (horizontal tab, `0x09`), `\r` (carriage return, `0x0D`), `\\` (backslash, `0x5C`), `\"` (double quote, `0x22`), `\0` (null byte, `0x00`).
- Character literals are enclosed in single quotes (`'A'`) and evaluated to their 8-bit ASCII value. Empty literals (`''`) and multi-character literals are rejected.

### 3.3 Whitespace and Line Ending Normalization

- Comments start with a semicolon (`;`) and extend to the end of the line.
- The lexer normalizes Windows CRLF (`\r\n`), Unix LF (`\n`), and legacy Mac CR (`\r`) line endings. Source line counters increment identically across all line ending conventions, ensuring deterministic line numbers in diagnostics and debug source maps.

---

## 4. Canonical Instruction Encoding Audit (All 31 Instructions)

The BitForge ISA contains exactly 31 executable instructions partitioned into 12 functional groups. The table below records the opcode, format, length, and audit verification status for each instruction:

| Group | Mnemonic | Opcode | Operands | Encoding Layout (Bytes) | Length | Verification Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **1** | `NOP` | `0x00` | None | `[0x00]` | 1 | **Conformant** |
| **1** | `HALT` | `0x01` | None | `[0x01]` | 1 | **Conformant** |
| **1** | `INX` | `0x17` | None | `[0x17]` | 1 | **Conformant** |
| **1** | `DEX` | `0x18` | None | `[0x18]` | 1 | **Conformant** |
| **1** | `RET` | `0x61` | None | `[0x61]` | 1 | **Conformant** |
| **2** | `INC` | `0x22` | $R_d$ | `[0x22, Rd]` | 2 | **Conformant** ($R_0..R_3$) |
| **2** | `DEC` | `0x23` | $R_d$ | `[0x23, Rd]` | 2 | **Conformant** ($R_0..R_3$) |
| **2** | `NOT` | `0x33` | $R_d$ | `[0x33, Rd]` | 2 | **Conformant** ($R_0..R_3$) |
| **2** | `PUSH` | `0x62` | $R_s$ | `[0x62, Rs]` | 2 | **Conformant** ($R_0..R_3$) |
| **2** | `POP` | `0x63` | $R_d$ | `[0x63, Rd]` | 2 | **Conformant** ($R_0..R_3$) |
| **3** | `MOV` | `0x11` | $R_d, R_s$ | `[0x11, Rd, Rs]` | 3 | **Conformant** ($R_0..R_3$) |
| **3** | `ADD` | `0x20` | $R_d, R_s$ | `[0x20, Rd, Rs]` | 3 | **Conformant** ($R_0..R_3$) |
| **3** | `SUB` | `0x21` | $R_d, R_s$ | `[0x21, Rd, Rs]` | 3 | **Conformant** ($R_0..R_3$) |
| **3** | `AND` | `0x30` | $R_d, R_s$ | `[0x30, Rd, Rs]` | 3 | **Conformant** ($R_0..R_3$) |
| **3** | `OR` | `0x31` | $R_d, R_s$ | `[0x31, Rd, Rs]` | 3 | **Conformant** ($R_0..R_3$) |
| **3** | `XOR` | `0x32` | $R_d, R_s$ | `[0x32, Rd, Rs]` | 3 | **Conformant** ($R_0..R_3$) |
| **3** | `CMP` | `0x40` | $R_d, R_s$ | `[0x40, Rd, Rs]` | 3 | **Conformant** ($R_0..R_3$) |
| **4** | `LDI` | `0x10` | $R_d, \text{imm8}$ | `[0x10, Rd, imm8]` | 3 | **Conformant** ($-128..255$) |
| **5** | `LDI16` | `0x14` | $IX, \text{addr16}$ | `[0x14, hi, lo]` | 3 | **Conformant** (Requires literal `IX`) |
| **6** | `LD` | `0x12` | $R_d, [\text{addr16}]$ | `[0x12, Rd, hi, lo]` | 4 | **Conformant** ($R_0..R_3$, Big-Endian) |
| **7** | `ST` | `0x13` | $[\text{addr16}], R_s$ | `[0x13, hi, lo, Rs]` | 4 | **Conformant** ($R_0..R_3$, Big-Endian) |
| **8** | `LDX` | `0x15` | $R_d, [IX]$ | `[0x15, Rd]` | 2 | **Conformant** ($R_0..R_3$, Requires `[IX]`) |
| **9** | `STX` | `0x16` | $[IX], R_s$ | `[0x16, Rs]` | 2 | **Conformant** ($R_0..R_3$, Requires `[IX]`) |
| **10** | `JMP` | `0x50` | $\text{target16}$ | `[0x50, hi, lo]` | 3 | **Conformant** (Labels & Addresses) |
| **10** | `JZ` | `0x51` | $\text{target16}$ | `[0x51, hi, lo]` | 3 | **Conformant** (Labels & Addresses) |
| **10** | `JNZ` | `0x52` | $\text{target16}$ | `[0x52, hi, lo]` | 3 | **Conformant** (Labels & Addresses) |
| **10** | `JC` | `0x53` | $\text{target16}$ | `[0x53, hi, lo]` | 3 | **Conformant** (Labels & Addresses) |
| **10** | `JNC` | `0x54` | $\text{target16}$ | `[0x54, hi, lo]` | 3 | **Conformant** (Labels & Addresses) |
| **10** | `CALL` | `0x60` | $\text{target16}$ | `[0x60, hi, lo]` | 3 | **Conformant** (Labels & Addresses) |
| **11** | `IN` | `0x70` | $R_d, \text{port8}$ | `[0x70, Rd, port8]` | 3 | **Conformant** ($R_0..R_3$, Port $0x00..0xFF$) |
| **12** | `OUT` | `0x71` | $\text{port8}, R_s$ | `[0x71, port8, Rs]` | 3 | **Conformant** ($R_0..R_3$, Port $0x00..0xFF$) |

In `tests/unit/test_assembler.cpp`, `AssemblerTest.All31InstructionEncodings` and `AssemblerTest.InstructionEncodingRegisterMatrix` verify all 31 opcodes across various operand registers, immediate boundaries, and target addresses. Every emitted byte matches independently derived expectations.

---

## 5. Assembler Directives & Memory Layout Audit

BitForge supports five assembler directives:

### 5.1 `.ORG` Directive

- Syntax: `.ORG <address16>`
- Updates the internal location counter (`location_counter_`).
- Rejects negative origins and addresses $> 0xFFFF$.
- Rejects addresses in $[0xF000, 0xFFFF]$ (reserved stack space).
- Multiple `.ORG` directives emitting into disjoint memory blocks are preserved in `ProgramImage::segments_` sorted in ascending address order. Overlapping segments are detected in Pass 2 and rejected with error diagnostics.

### 5.2 `.ENTRY` Directive

- Syntax: `.ENTRY <label | address16>`
- Sets `ProgramImage::entry_point_`.
- When omitted, the entry point defaults to $0x0000$ per v1.7 §5.3.
- Multiple `.ENTRY` directives in a single program are rejected as duplicate entry points.
- An entry point located in $[0xF000, 0xFFFF]$ is rejected.
- An entry point that points to an uninitialized memory gap outside any emitted segment is detected and rejected with a clear diagnostic.

### 5.3 `.BYTE` Directive

- Syntax: `.BYTE <val1> [, <val2>, ...]`
- Emits sequential 8-bit bytes into the active segment.
- Validates operand range in $[-128, 255]$. Values $< -128$ or $> 255$ are rejected with diagnostic errors.

### 5.4 `.WORD` Directive

- Syntax: `.WORD <val1> [, <val2>, ...]`
- Emits sequential 16-bit words in big-endian network byte order (high byte followed by low byte).
- Validates operand range in $[-32768, 65535]$. Values outside this range are rejected with diagnostic errors.

### 5.5 `.ASCII` Directive

- Syntax: `.ASCII "<string>"`
- Emits each ASCII character byte sequentially without an implicit null terminator (matching educational assembly expectations). Null-terminated strings can be constructed explicitly via `.ASCII "text\0"` or `.ASCII "text" \n .BYTE 0`.

---

## 6. Symbol Resolution & Expression Evaluation Audit

### 6.1 Two-Pass Resolution Architecture

The assembler employs a standard two-pass architecture:
- **Pass 1:** Defines labels in `SymbolTable`, computes location counter advancements, records directives, and validates label uniqueness. For forward references appearing in expressions during Pass 1, a dummy value of 0 is returned to measure instruction sizing.
- **Pass 2:** Evaluates all expressions and symbols against the populated `SymbolTable`, emits binary bytes into contiguous `Segment` objects, checks for overlapping memory writes, and builds the `SourceMap` database.

### 6.2 Case Insensitivity & Label Normalization

`SymbolTable::normalize()` transforms all identifiers to uppercase. The table below illustrates normalization behavior:

| Defined Label | Reference in Source | SymbolTable Key | Lookup Result | Status |
| :--- | :--- | :--- | :--- | :--- |
| `MyLoop:` | `JMP myloop` | `"MYLOOP"` | Matched | **Conformant** |
| `Start:` | `CALL START` | `"START"` | Matched | **Conformant** |
| `target:` | `JZ Target` | `"TARGET"` | Matched | **Conformant** |
| `my_label:` followed by `MY_LABEL:` | N/A | `"MY_LABEL"` | Duplicate Rejected | **Conformant** |

### 6.3 Symbol Arithmetic & Expressions

`Assembler::parse_expression()` supports addition (`+`) and subtraction (`-`) across numbers and symbols:
- Immediate arithmetic: `LDI R0, 5 + 15` $\rightarrow$ evaluates to 20 (`0x14`).
- Label offsets: `LD R2, [data_block + 2]` $\rightarrow$ adds 2 to the symbol address of `data_block`.
- Relative branch targets: `JMP start + 3` $\rightarrow$ jumps to `start + 3`.
- Negative word arithmetic: `.WORD 0x1000 - 0x0001` $\rightarrow$ emits `0x0F, 0xFF`.

---

## 7. Stack Boundary & Reserved Region Audit (`RISK-P3-002`)

### 7.1 Problem Definition & Architectural Requirements

Per v1.7 §5.2 and ADR-001, the address range $[0xF000, 0xFFFF]$ (4,096 bytes) is reserved exclusively for the CPU execution stack. Program code and static data must never overlap with this region.

`RISK-P3-002` investigated whether multi-byte instructions or data structures starting just below $0xF000$ (e.g. at $0xEFFF$) could overflow into the stack region without detection.

### 7.2 Enforcement Mechanism in Implementation

Inspection of `src/assembler/assembler.cpp` revealed that boundary checking is applied at three distinct levels:
1. **`.ORG` Pre-Check:**
   ```cpp
   if (*val >= kStackReservedStart) {
       add_error(dir_tok.location, "Cannot set .ORG into reserved stack region (address " +
                                      to_hex_word(static_cast<word_t>(*val)) + " >= 0xF000)");
       return;
   }
   ```
2. **`.ENTRY` Pre-Check:**
   ```cpp
   if (sym->address >= kStackReservedStart) {
       add_error(target_tok.location, "Entry address " + to_hex_word(sym->address) +
                                      " cannot be in reserved stack region (>= 0xF000)");
   }
   ```
3. **`emit_byte` Invariant Enforcement:**
   ```cpp
   if (location_counter_ >= kStackReservedStart) {
       add_error(loc, "Cannot emit program data into reserved stack region (address " +
                      to_hex_word(location_counter_) + " >= 0xF000)");
       return;
   }
   ```

Because every multi-byte instruction (2-byte, 3-byte, 4-byte) and every data directive emits bytes sequentially through `emit_byte`, any subsequent byte whose location counter reaches $0xF000$ immediately triggers an error diagnostic and halts emission.

### 7.3 Failed-Assembly ProgramImage API Contract

The assembler interface specifies that `Assembler::assemble(source)` returns an `AssembleResult`:

```cpp
struct AssembleResult {
    bool success{false};
    ProgramImage image{};
    std::vector<Diagnostic> diagnostics{};
    SymbolTable symbols{};
};
```

When an error occurs during Pass 1 (e.g. `location_counter_ >= 0xF000` in `emit_byte`), Pass 2 is aborted immediately and `result.success` is set to `false`. Crucially:
- `result.image` is returned as a default-constructed `ProgramImage` with 0 segments and 0 total bytes.
- Even if earlier statements in the source program were valid (e.g. `.ORG 0x0200 \n NOP` preceding `.ORG 0xEFFF \n INC R0`), those earlier bytes remain strictly internal to the aborted pass and are **never** emitted into `result.image`.
- Calling `result.image.load_into(memory)` traverses an empty segment list and performs zero memory writes.
- Thus, failure does not emit partial, corrupt, or stack-colliding segments.

### 7.4 Boundary Matrix Verification Results

In `tests/unit/test_assembler.cpp`, `AssemblerTest.StackBoundaryCollisionExhaustiveMatrix` verifies boundary edge cases and failure image invariants:

| Scenario | Start Address | Instruction / Directive | Emitted Range | Expected Outcome | Actual Outcome & Returned Image | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Exact-Fit 1-Byte** | `0xEFFF` | `NOP` (1 byte) | `0xEFFF` | Success, 1 byte | Success (Segment: `0xEFFF`, size 1) | **Conformant** |
| **Exact-Fit 2-Byte** | `0xEFFE` | `INC R0` (2 bytes) | `0xEFFE..0xEFFF` | Success, 2 bytes | Success (Segment: `0xEFFE`, size 2) | **Conformant** |
| **Exact-Fit 3-Byte** | `0xEFFD` | `LDI R0, 1` (3 bytes) | `0xEFFD..0xEFFF` | Success, 3 bytes | Success (Segment: `0xEFFD`, size 3) | **Conformant** |
| **Exact-Fit 4-Byte** | `0xEFFC` | `LD R0, [0x1000]` (4 bytes) | `0xEFFC..0xEFFF` | Success, 4 bytes | Success (Segment: `0xEFFC`, size 4) | **Conformant** |
| **Exact-Fit Data** | `0xEFFE` | `.WORD 0x1234` (2 bytes) | `0xEFFE..0xEFFF` | Success, 2 bytes | Success (Segment: `0xEFFE`, size 2) | **Conformant** |
| **Overrun 2-Byte** | `0xEFFF` | `INC R0` (2 bytes) | `0xEFFF..0xF000` | Assembly Failure | Rejected (Diag: `address 0xF000 >= 0xF000`; Image: 0 segments, 0 bytes) | **Conformant** |
| **Overrun 3-Byte** | `0xEFFE` | `LDI R0, 1` (3 bytes) | `0xEFFE..0xF000` | Assembly Failure | Rejected (Diag: `address 0xF000 >= 0xF000`; Image: 0 segments, 0 bytes) | **Conformant** |
| **Overrun 4-Byte** | `0xEFFD` | `LD R0, [0x1000]` (4 bytes) | `0xEFFD..0xF000` | Assembly Failure | Rejected (Diag: `address 0xF000 >= 0xF000`; Image: 0 segments, 0 bytes) | **Conformant** |
| **Overrun Data** | `0xEFFF` | `.WORD 0x1234` (2 bytes) | `0xEFFF..0xF000` | Assembly Failure | Rejected (Diag: `address 0xF000 >= 0xF000`; Image: 0 segments, 0 bytes) | **Conformant** |
| **Origin at Bound** | `0xF000` | `.ORG 0xF000` | N/A | Assembly Failure | Rejected (Diag: `.ORG into reserved stack`; Image: 0 segments) | **Conformant** |
| **Origin in Stack** | `0xFFFF` | `.ORG 0xFFFF` | N/A | Assembly Failure | Rejected (Diag: `.ORG into reserved stack`; Image: 0 segments) | **Conformant** |
| **Entry in Stack** | `0xF100` | `.ENTRY 0xF100` | N/A | Assembly Failure | Rejected (Diag: `Entry address in stack`; Image: 0 segments) | **Conformant** |
| **Multi-Seg Overrun** | `0x0200` + `0xEFFF` | `NOP` + `INC R0` | `0x0200`, `0xEFFF..` | Assembly Failure | Rejected (Diag: `address 0xF000 >= 0xF000`; Image: 0 segments, 0 memory mutations) | **Conformant** |

`RISK-P3-002` is fully resolved: the assembler strictly prevents code or data from entering the stack space and emits zero segments upon failure.

---

## 8. Diagnostic Quality & Malformed Input Handling Matrix

The assembler diagnostics engine produces structured `Diagnostic` records containing severity (`Error` or `Warning`), source location (1-based line and column), and descriptive error messages.

### 8.1 Assertion Scope in Automated Tests

It is essential to distinguish between the assertions executed in automated regression tests:
1. **Rejection & Error Generation:** In `AssemblerTest.MalformedInputComprehensiveMatrix`, all 31 snippets below are asserted for `EXPECT_FALSE(res.success)` and `EXPECT_FALSE(res.diagnostics.empty())`.
2. **Specific Message & Line Assertions:** Dedicated unit tests (`DuplicateLabelError`, `UnresolvedLabelError`, `InvalidRegisterDiagnostic`, `OutOfRangeImmediateDiagnostic`, `DiagnosticSourceLocationsPrecision`, `RejectDuplicateEntryDirective`, `RejectExplicitEntryInUninitializedGap`, `RejectStackRegionEmission`) assert exact line numbers and diagnostic message substrings.
3. **Column Coordinate Assertions:** `DiagnosticSourceLocationsPrecision` asserts `line == 2` and `column > 0`. Granular column coordinates across all 31 malformed snippets are not individually asserted in the test suite and represent an evidence gap (tracked under Section 11).

### 8.2 Malformed Input Rejection Matrix

| Category | Tested Malformed Input | Implementation Diagnostic Message | Matrix Assertion | Status |
| :--- | :--- | :--- | :--- | :--- |
| **Mnemonic** | `INVALID_OPCODE\n` | `"Unknown mnemonic or directive: 'INVALID_OPCODE'"` | `success == false, !diag.empty()` | **Conformant** |
| **Operands** | `MOV R0\n` | `"Expected comma after destination register"` | `success == false, !diag.empty()` | **Conformant** |
| **Registers** | `MOV R0, R4\n` | `"Expected source register (R0..R3)"` | `success == false, !diag.empty()` | **Conformant** |
| **Registers** | `MOV R4, R0\n` | `"Expected destination register (R0..R3)"` | `success == false, !diag.empty()` | **Conformant** |
| **Syntax** | `MOV R0 R1\n` | `"Expected comma after destination register"` | `success == false, !diag.empty()` | **Conformant** |
| **Operands** | `LDI R0\n` | `"Expected comma after destination register"` | `success == false, !diag.empty()` | **Conformant** |
| **Operands** | `LDI R0,\n` | `"Expected 8-bit immediate value after comma"` | `success == false, !diag.empty()` | **Conformant** |
| **Range** | `LDI R0, 256\n` | `"Immediate value out of range [-128, 255]: 256"` | `success == false, !diag.empty()` | **Conformant** |
| **Range** | `LDI R0, -129\n` | `"Immediate value out of range [-128, 255]: -129"` | `success == false, !diag.empty()` | **Conformant** |
| **Register** | `LDI16 R0, 0x1000\n` | `"Expected 'IX' register for LDI16"` | `success == false, !diag.empty()` | **Conformant** |
| **Operands** | `LDI16 IX\n` | `"Expected comma after IX"` | `success == false, !diag.empty()` | **Conformant** |
| **Syntax** | `LD R0, 0x1000\n` | `"Expected '[' for memory address operand"` | `success == false, !diag.empty()` | **Conformant** |
| **Syntax** | `LD R0, [0x1000\n` | `"Expected ']' after memory address"` | `success == false, !diag.empty()` | **Conformant** |
| **Registers** | `LD R4, [0x1000]\n` | `"Expected destination register (R0..R3)"` | `success == false, !diag.empty()` | **Conformant** |
| **Syntax** | `ST 0x1000, R0\n` | `"Expected '[' for destination memory address"` | `success == false, !diag.empty()` | **Conformant** |
| **Registers** | `ST [0x1000], R4\n` | `"Expected source register (R0..R3)"` | `success == false, !diag.empty()` | **Conformant** |
| **Syntax** | `LDX R0, [R1]\n` | `"Expected 'IX' register inside brackets"` | `success == false, !diag.empty()` | **Conformant** |
| **Syntax** | `STX [R0], R1\n` | `"Expected 'IX' register inside brackets"` | `success == false, !diag.empty()` | **Conformant** |
| **Registers** | `INC R4\n` | `"Expected register (R0..R3) after INC"` | `success == false, !diag.empty()` | **Conformant** |
| **Registers** | `DEC R4\n` | `"Expected register (R0..R3) after DEC"` | `success == false, !diag.empty()` | **Conformant** |
| **Registers** | `PUSH R4\n` | `"Expected register (R0..R3) after PUSH"` | `success == false, !diag.empty()` | **Conformant** |
| **Registers** | `POP R4\n` | `"Expected register (R0..R3) after POP"` | `success == false, !diag.empty()` | **Conformant** |
| **Range** | `IN R0, 256\n` | `"Port number out of range [0, 255]: 256"` | `success == false, !diag.empty()` | **Conformant** |
| **Range** | `OUT 256, R0\n` | `"Port number out of range [0, 255]: 256"` | `success == false, !diag.empty()` | **Conformant** |
| **Registers** | `IN R4, 0x01\n` | `"Expected destination register (R0..R3)"` | `success == false, !diag.empty()` | **Conformant** |
| **Registers** | `OUT 0x01, R4\n` | `"Expected source register (R0..R3)"` | `success == false, !diag.empty()` | **Conformant** |
| **Range** | `.BYTE 256\n` | `".BYTE value out of range [-128, 255]: 256"` | `success == false, !diag.empty()` | **Conformant** |
| **Range** | `.BYTE -129\n` | `".BYTE value out of range [-128, 255]: -129"` | `success == false, !diag.empty()` | **Conformant** |
| **Range** | `.WORD 65536\n` | `".WORD value out of range [-32768, 65535]: 65536"` | `success == false, !diag.empty()` | **Conformant** |
| **Range** | `.WORD -32769\n` | `".WORD value out of range [-32768, 65535]: -32769"` | `success == false, !diag.empty()` | **Conformant** |
| **Range** | `.ORG 0x10000\n` | `".ORG address out of range [0x0000, 0xFFFF]: 65536"` | `success == false, !diag.empty()` | **Conformant** |

---

## 9. Example Programs Smoke Verification (`examples/*.asm`)

The repository includes two canonical assembly examples in `examples/`. Both were verified via automated unit testing (`AssemblerTest.ExamplesAssemblyAndExecutionVerification`) and standalone command-line testing via `bitforge.exe`.

### 9.1 `examples/01_arithmetic.asm`

- **Purpose:** Baseline arithmetic demonstration ($10 + 20 = 30$ stored at address `0x8000`).
- **Assembly Metrics:**
  - Status: Success (0 diagnostics)
  - Entry Point: `0x0200`
  - Total Size: 14 bytes
  - Segments: 1 segment (`0x0200..0x020D`)
- **Execution Output:**
  - Machine State: `Halted`
  - Steps Executed: 5 instructions
  - Final Registers: $R_0 = 30$ (`0x1E`), $R_1 = 20$ (`0x14`), $R_2 = 0$, $R_3 = 0$
  - Memory at `0x8000`: `0x1E` (30)
  - Flags: $Z=0, C=0, N=0, V=0$

### 9.2 `examples/02_io_demo.asm`

- **Purpose:** Virtual peripheral demonstration across Text Console (Port `0x01`), Numeric Console (Port `0x02`), and Virtual LEDs (Port `0x10`).
- **Assembly Metrics:**
  - Status: Success (0 diagnostics)
  - Entry Point: `0x0200`
  - Total Size: 31 bytes
  - Segments: 1 segment (`0x0200..0x021E`)
- **Execution Output:**
  - Machine State: `Halted`
  - Steps Executed: 11 instructions
  - Text Output: `"HI!"` (Ports `0x01`: `'H'`, `'I'`, `'!'`)
  - Numeric Output: `"42"` (Port `0x02`: `42`)
  - Virtual LEDs: `0x55` (`0b01010101`) (Port `0x10`: `0x55`)

---

## 10. Test Matrix & Regression Results

The BitForge core test suite expanded from 265 unit tests to 272 unit tests (+7 tests, +2.64%). All tests were executed across all four CMake build presets configured in `CMakePresets.json`:

| CMake Build Preset | Build Toolchain | Unit Tests Executed | Unit Tests Passed | GUI Tests Executed | GUI Tests Passed | Total Pass Rate |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **`ci`** | GCC 14.2.0 (MSYS2 UCRT64, Ninja) | 272 | 272 | N/A (CLI/Core Preset) | N/A | **100%** |
| **`debug`** | GCC 14.2.0 (MSYS2 UCRT64, Ninja) | 272 | 272 | N/A (Core Debug Preset) | N/A | **100%** |
| **`gui`** | GCC 14.2.0 + Qt 6.8.0 (Ninja) | 272 | 272 | 83 test entries | 83 | **100%** |
| **`gui-release`**| GCC 14.2.0 + Qt 6.8.0 (Ninja, `-O3`) | 272 | 272 | 83 test entries | 83 | **100%** |

### Breakdown of Assembler Test Suite (`tests/unit/test_assembler.cpp`)

The assembler unit test file now comprises 41 comprehensive test fixtures:
1. `All31InstructionEncodings`: Baseline instruction encoding vector verification.
2. `DirectiveByteAndWordAndAscii`: `.BYTE`, `.WORD`, and `.ASCII` directive emission and endianness.
3. `CaseInsensitiveRegistersAndMnemonics`: Lowercase and mixed-case instruction parsing.
4. `LabelsAndForwardReferences`: Forward references and label resolution.
5. `MultipleDisjointSegments`: Disjoint `.ORG` blocks and segment creation.
6. `RejectOverlappingSegments`: Detection of overlapping memory segments.
7. `RejectStackRegionEmission`: Rejection of `.ORG` in reserved stack space.
8. `RejectEntryInStackRegion`: Rejection of `.ENTRY` in reserved stack space.
9. `DuplicateLabelError`: Rejection of duplicate label definitions.
10. `UnresolvedLabelError`: Diagnostic for undefined label references.
11. `InvalidRegisterDiagnostic`: Rejection of non-existent register names.
12. `OutOfRangeImmediateDiagnostic`: Immediate value out-of-range diagnostics.
13. `CrlfAndLfNormalizationIdenticalOutput`: Verification of identical output across CRLF and LF.
14. `EndToEndArithmeticBenchmarkProgram`: End-to-end arithmetic program execution on CPU.
15. `EndToEndSubroutineAndIoProgram`: Subroutine call, return, and peripheral I/O execution.
16. `RejectExplicitEntryInUninitializedGap`: Rejection of entry point outside emitted segments.
17. `RejectExplicitLabelEntryAtEndOfFileGap`: Rejection of EOF label entry point without bytes.
18. `RejectDuplicateEntryDirective`: Rejection of multiple `.ENTRY` directives.
19. `DefaultEntryBehaviorAtZero`: Verification of default entry point $0x0000$ when `.ENTRY` is omitted.
20. `EmptyProgramAndWhitespaceDiagnostics`: Diagnostic for empty or whitespace-only sources.
21. `ProgramContainingOnlyDataDiagnostic`: Diagnostic warning for data-only assemblies.
22. `ForwardAndBackwardOrgSegments`: Sorting disjoint segments into ascending address order.
23. `SegmentEndingExactlyAtPermittedBoundary`: Exact-fit segment at boundary $0xEFFF$.
24. `EmissionCrossingIntoReservedStackRegion`: 2-byte data emission crossing from $0xEFFF$ to $0xF000$.
25. `InstructionCrossingIntoReservedStackRegion`: 3-byte instruction crossing into $0xF000$.
26. `LabelsImmediatelyBeforeAndAfterDirectives`: Address assignment for boundary labels.
27. `UnterminatedStringDiagnostics`: Diagnostics for unterminated string literals.
28. `InvalidEscapeSequenceDiagnostic`: Diagnostics for unknown string escape characters.
29. `InvalidAndOverflowingNumericLiterals`: Diagnostics for number overflows and malformed literals.
30. `MissingOperandsDiagnostics`: Diagnostics for instructions and directives missing operands.
31. `UnexpectedTrailingTokensDiagnostic`: Diagnostics for extra tokens at statement endings.
32. `StandaloneCarriageReturnLineEndings`: Source handling for standalone CR line breaks.
33. `DiagnosticSourceLocationsPrecision`: Line and column precision in diagnostic outputs.
34. `ProgramImageLoadIntoSeparation`: Mutation isolation between `Memory` loading and `Cpu` reset.
35. `InstructionEncodingRegisterMatrix` *(New)*: Systematic verification of all 31 instructions across register variants ($R_0..R_3$, $IX$).
36. `ExpressionsAndSymbolArithmetic` *(New)*: Arithmetic expressions (`+`, `-`) in instructions, directives, and symbol offsets.
37. `SymbolResolutionCaseSensitivityAndShadowing` *(New)*: Case-insensitive symbol matching and duplicate detection across casing.
38. `StackBoundaryCollisionExhaustiveMatrix` *(New)*: Systematic audit of 1-, 2-, 3-, 4-byte boundary fits at $0xEFFF$ vs. 1-byte stack overflows, confirming failure generates 0 segments.
39. `MalformedInputComprehensiveMatrix` *(New)*: 31 malformed inputs spanning syntax, registers, operands, and ranges.
40. `ExamplesAssemblyAndExecutionVerification` *(New)*: Full assembly and CPU execution of `examples/01_arithmetic.asm` and `examples/02_io_demo.asm`.
41. `ProgramImageAndSourceMapQueryAudit` *(New)*: ProgramImage methods and SourceMap line/breakpoint query resolution.

---

## 11. Residual Risks & Technical Debt Assessment

| Risk ID | Component | Description | Severity | Mitigation / Resolution Status |
| :--- | :--- | :--- | :--- | :--- |
| **RISK-P4-001** | Assembler | Multi-pass symbol sizing assumption: Pass 1 assumes all instructions have fixed length (1–4 bytes) regardless of symbol values. | Low | **Mitigated:** In BitForge ISA v1.7, instruction lengths are strictly determined by opcode/mnemonic type, not operand values. Sizing is completely invariant to symbol addresses. |
| **RISK-P4-002** | Assembler | Memory allocation on large disjoint `.ORG` gaps: Emitting widely spaced segments does not allocate intermediate uninitialized memory. | Very Low | **Verified:** `ProgramImage` stores only non-contiguous `Segment` records. Intermediate memory remains unaffected until explicitly loaded. |
| **RISK-P4-003** | SourceMap | Debugger breakpoint resolution on data-only or comment lines. | Low | **Verified:** `SourceMap::resolve_line_breakpoint()` returns `success = false` with explanatory messages for comments, labels, and data lines. |
| **RISK-P3-001** | `SystemBus` Event Log | In headless CLI execution (`cli::execute_run`), `SystemBus::write_byte` accumulates `MemoryWriteEvent` and `IoEvent` records without drainage in the step loop. | Medium | **Scheduled for Phase 5:** Memory footprint impact under long-running headless workloads remains unmeasured and will be profiled in Phase 5. |
| **RISK-P3-002** | Assembler | Stack collision via multi-byte instructions ending at $0xEFFF$. | Closed | **Resolved:** Fully audited and verified in Section 7. `emit_byte` pre-checks prevent crossing into $[0xF000, 0xFFFF]$, and failure emits 0 segments. |
| **GAP-P4-001** | Test Suite | Diagnostic column coordinates across 31 malformed snippets in `AssemblerTest.MalformedInputComprehensiveMatrix` are asserted for failure and non-empty diagnostics, but not granular column numbers. | Low | **Evidence Gap:** Documented as an evidence gap. Basic line and column validity is asserted in `DiagnosticSourceLocationsPrecision`. |

---

## 12. Sign-Off & Recommendations for Phase 5

### 12.1 Phase 4 Audit Sign-Off

The BitForge assembler subsystem demonstrably satisfies all normative architectural requirements of specification v1.7 and ADR-001. All 31 instruction encodings, directives, symbol handling mechanisms, and stack protection boundaries have been verified with zero regressions. The documented diagnostic behaviours were exercised by the reported tests, while `GAP-P4-001` remains open for granular column-coordinate assertions across all 31 malformed inputs.

**Phase 4 is hereby certified COMPLETE and ready for sign-off.**

### 12.2 Recommendations for Phase 5 (Virtual I/O, CLI Runner, and Headless Execution Audit)

In strict alignment with the Phase 0 baseline roadmap (`QA_PHASE0_BASELINE_REPORT.md` Section 6) and the Phase 3 audit (`QA_PHASE3_MEMORY_BUS_STACK_REPORT.md` Section 10), Phase 5 is designated for **Virtual I/O, CLI Runner, and Headless Execution Verification**:

1. **CLI Execution & Argument Parsing:** Audit `src/cli/runner.cpp`, command-line option parsing (`--max-steps`, `--dump-regs`), usage help text, and exit codes:
   - Exit Code `0`: Success (program halted normally or assembly succeeded).
   - Exit Code `1`: Command-line usage error or invalid arguments.
   - Exit Code `2`: Source file read error or assembly error.
   - Exit Code `3`: CPU execution fault.
   - Exit Code `4`: Instruction execution budget exceeded.
2. **Headless Execution Budget & Termination Invariants:** Verify instruction budget enforcement (`--max-steps`) and infinite loop termination behavior.
3. **Event Journal Accumulation & Drainage Stress Audit (`RISK-P3-001`):** Profile heap memory consumption under long-running headless workloads (e.g. 100,000 to 1,000,000 steps) where `SystemBus::write_byte` and I/O writes accumulate `MemoryWriteEvent` and `IoEvent` records without per-step drainage.
4. **Virtual Peripheral Headless Polling:** Audit input FIFO underflow / empty behavior (returns `0x00` without faulting), text console, numeric console, and LED patterns in non-GUI mode.
5. **Preservation of Phase 6 Scope for Debugger:** Interactive debugger breakpoints, stepping (`step_into`, `step_over`, `step_out`), trace history ring buffer limits, and execution delta snapshots remain assigned to **Phase 6** (CPU Execution & Debugger Observability Audit).

---

## 13. Phase 4.1 Reconciliation Summary

Phase 4.1 executed a focused closeout and reconciliation pass:
1. **Failed-Assembly `ProgramImage` Contract:** Verified through implementation analysis and tests that on assembly failure, `AssembleResult::image` contains 0 segments and 0 bytes. Added an explicit test in `AssemblerTest.StackBoundaryCollisionExhaustiveMatrix` demonstrating that earlier valid code preceding a stack overrun is not returned in a partial image, and loading the failed result into memory mutates zero bytes.
2. **Diagnostic Error Matrix & Location Precision:** Reconciled Section 8 to accurately state that `AssemblerTest.MalformedInputComprehensiveMatrix` asserts rejection (`success == false`) and diagnostic emission across all 31 inputs, while line numbers are asserted in dedicated tests. Recorded granular column coordinate checking across all 31 snippets as an evidence gap (`GAP-P4-001`).
3. **Roadmap Realignment:** Reconciled Section 12.2 to align Phase 5 with the Phase 0 and Phase 3 roadmaps (CLI runner, headless execution, and `RISK-P3-001` event-journal stress audit), retaining debugger observability for Phase 6.
4. **Preserved Repository State:** The Git repository state was preserved without staging, committing, pushing, resetting, or discarding any files. The working tree retains the expected modified test suites (six tracked unit test files across Phases 2 through 4) and five untracked QA audit reports. Frozen specifications and ADR-001 remain unmodified.

---

## 14. Phase 4.2 Closeout & Final Regression Record

Phase 4.2 completes the final documentation closeout and multi-preset regression pass for the assembler audit:

### 14.1 Documentation Corrections
1. **Repository-State Accuracy:** Replaced previous broad references to a "clean working tree" with precise accounting of the expected modified unit test suites (`tests/unit/test_alu.cpp`, `test_assembler.cpp`, `test_bus.cpp`, `test_cpu_instructions.cpp`, `test_memory.cpp`, `test_stack_instructions.cpp`) and the five untracked QA deliverables (`docs/QA_PHASE0_BASELINE_REPORT.md` through `docs/QA_PHASE4_ASSEMBLER_DIRECTIVES_REPORT.md`).
2. **Commit and Staging Attestation:** Confirmed by direct Git inspection that HEAD remains unchanged at `5a88e11`, branch `master` remains ahead of `origin/master` by 1 commit, and zero changes have been staged, committed, or pushed.
3. **Whitespace Attestation Scoping:** Distinguished between `git diff --check` (which validates the tracked working-tree diff against whitespace errors) and separate programmatic regex scans (`\s+$`) validating zero trailing whitespace across all five untracked QA reports.
4. **Preserved Findings:** Preserved the failed-assembly empty `ProgramImage` contract verification, the `GAP-P4-001` diagnostic-coordinate evidence gap, and the reconciled Phase 5 (`RISK-P3-001`, CLI runner) / Phase 6 (Debugger) scopes.

### 14.2 Final Multi-Preset Regression Results (Updated Source)
Following the Phase 4.1 test source updates in `tests/unit/test_assembler.cpp`, builds and test runs were executed across all four CMake presets:

| CMake Preset | Invocation Commands | Core Unit Tests | Qt GUI Test Slots | Overall Result |
| :--- | :--- | :--- | :--- | :--- |
| **`ci`** | `cmake --build --preset ci`<br>`ctest --preset ci --output-on-failure` | **272 / 272 Passed** (0.17s) | N/A (CLI preset) | **100% Pass** |
| **`debug`** | `cmake --build --preset debug`<br>`ctest --preset debug --output-on-failure` | **272 / 272 Passed** (0.36s) | N/A (Debug preset) | **100% Pass** |
| **`gui`** | `cmake --build --preset gui`<br>`ctest --preset gui --output-on-failure` | **272 / 272 Passed** (0.35s) | **83 / 83 Passed** (15.32s) | **100% Pass** |
| **`gui-release`** | `cmake --build --preset gui-release`<br>`ctest --preset gui-release --output-on-failure` | **272 / 272 Passed** (0.13s) | **83 / 83 Passed** (14.54s) | **100% Pass** |

All 272 unit tests and 83 Qt GUI tests pass cleanly on the final working-tree source.
