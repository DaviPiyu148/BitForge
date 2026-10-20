# BitForge Phase 2 — CPU Instruction, Arithmetic & Boundary Verification Report

- **Date of Audit:** October 20, 2026
- **Auditor:** Quality Assurance, CPU Verification & Systems Engineering Audit (assisted by Antigravity)
- **Target Repository:** `D:\BitForge`
- **Target Branch:** `master`
- **Document Version:** 1.3 (Phase 3.2 Scope-Reconciled Deliverable)
- **Primary Architecture Authority:** `docs/BitForge_Architecture_Specification_v1.7.md`
- **Stack Architecture Authority:** `docs/decisions/ADR-001-stack-pointer-convention.md`
- **Preceding Phase Reports:** `docs/QA_PHASE0_BASELINE_REPORT.md`, `docs/QA_PHASE1_SPEC_CONFORMANCE_REPORT.md`

---

## 1. Executive Summary

The principal verification scope of Phase 2 was the BitForge Central Processing Unit (`Cpu`) and Arithmetic Logic Unit (`Alu`), focusing specifically on arithmetic/logic edge cases, instruction boundary execution (such as 4-byte instruction fetch wraparound), and CPU fault behavior.

Any preliminary inspection of the memory subsystem or system bus during Phase 2 was strictly limited to supporting CPU instruction boundary interactions (e.g., 4-byte `LD`/`ST` fetch wraparound at `0xFFFD..0xFFFF` and self-modifying code execution). Full, dedicated verification of the 64 KiB memory subsystem contract (`Memory`), system bus delegation, virtual I/O peripherals, and the ADR-001 stack discipline was deliberately reserved for and completed in Phase 3 (`docs/QA_PHASE3_MEMORY_BUS_STACK_REPORT.md`).

Prior to test development, a preliminary consistency pass was completed on the preceding Phase 0 and Phase 1 documentation deliverables:
1. Reconciled the classification of 4-byte instruction wrapping: static transient cursor logic `(cursor + 1) & 0xFFFF` was distinguished from missing dedicated runtime execution fixtures.
2. Formally aligned subtraction flag documentation with the canonical carry/no-borrow convention: $C = 1$ indicates no borrow ($a \ge b$), and $C = 0$ indicates a borrow ($a < b$).
3. Refined scope statements to reflect the specific requirements traced and evidenced rather than overclaiming blanket architectural verification.
4. Inspected and verified both untracked QA reports for trailing whitespace and encoding cleanliness.

### Key Phase 2 Verification Findings

1. **Four-Byte Instruction Boundary Gap Resolved:** Added 7 comprehensive unit test fixtures exercising 4-byte instructions (`LD Rd, [addr16]` and `ST [addr16], Rs`) straddling the 64 KiB boundary at addresses `0xFFFD`, `0xFFFE`, and `0xFFFF`, as well as memory read/write access to address `0xFFFF`. All transient PC modulo-65536 wraparounds and memory mutations executed with 100% architectural conformance.
2. **Exhaustive ALU Verification ($256 \times 256$ Combinations):** Implemented independent, strictly portable specification-derived reference formulas for pure arithmetic and bitwise operations. Evaluated:
   - 65,536 input vectors for `Alu::add`: 0 mismatches.
   - 65,536 input vectors for `Alu::sub`: 0 mismatches.
   - 65,536 input vectors for `Alu::cmp` (verified against `Alu::sub`): 0 mismatches.
   - 512 input vectors for `Alu::inc`: 0 mismatches.
   - 512 input vectors for `Alu::dec`: 0 mismatches.
   - 65,792 input vectors for bitwise operations (256 unary for `not_op`, 65,536 binary pairs for `and_op`, `or_op`, `xor_op`): 0 mismatches across 196,864 bitwise operations.
   - Reconciled metric accounting: **263,465 fixture-evaluated input vectors**, **394,609 target ALU calls**, **65,536 reference ALU calls** (460,145 total ALU function calls), **161 dynamic runtime assertion evaluations** (across 21 static assertion sites in source code), all passing with zero defects.
3. **Deterministic Boundary Pair Verification:** Validated all 36 combinations of the boundary set $\{0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF\}^2$ via direct ALU methods (108 operations) and via CPU instruction dispatch (`ADD R0, R1`: 36 CPU instruction steps).
4. **Register Encoding Fault Atomicity:** Tested register validation across all 19 register-consuming instruction forms (26 total operand permutations) using invalid register ID `0x04`. Confirmed that illegal register IDs trigger `FaultType::InvalidRegisterEncoding`, preserve $PC$ at instruction start, and commit zero mutations to registers, flags, or memory.
5. **Self-Modifying Code Observability:** Verified that writes to code memory (`ST [addr16], Rs`) are immediately observable on subsequent instruction fetches within the flat memory model, conforming to §5.1.
6. **Zero Code Defects Found in Tested CPU/ALU Scope:** The existing implementation in `src/core/cpu.cpp` and `src/core/alu.cpp` executed without failures across all tested arithmetic, boundary, wraparound, and fault cases in this phase's scope. No production code changes were required.
7. **Regression Suite Expansion & Full GUI Stability:** Unit test count expanded from **233 to 254 test cases** (+21 new comprehensive tests). Build preset `ci` enforces `-Werror`, while `debug`, `gui`, and `gui-release` compiled cleanly with 0 observed warnings. The complete GUI test suite was verified with all **83 QtTest entries** (81 test methods + 2 fixture slots) passing across both `gui` and `gui-release`.

---

## 2. Starting Git State

| Property | Value | Evidence / Command Output |
|:---|:---|:---|
| **Working Directory** | `D:\BitForge` | `Get-Location` |
| **Git Repository Root** | `D:/BitForge` | `git rev-parse --show-toplevel` |
| **Active Branch** | `master` | `git status --short --branch` |
| **Tracking Status** | `## master...origin/master [ahead 1]` | 1 local commit ahead of origin |
| **HEAD Commit Hash** | `5a88e11b7833a119ed847733bcc4db2bc4cbbbfa` | `git rev-parse HEAD` |
| **HEAD Commit Message** | `fix(gui,ux): finalize design token geometry and audit evidence` | Commit log |
| **Starting Modified Files**| None in tracked tree | `git diff --check` returned 0 |
| **Untracked Deliverables**| `docs/QA_PHASE0_BASELINE_REPORT.md`<br>`docs/QA_PHASE1_SPEC_CONFORMANCE_REPORT.md` | `git status --short` |

---

## 3. CPU Edge-Case & Instruction Boundary Test Matrix

The following matrix inventories all canonical instruction families, relevant boundary conditions, existing coverage, and the Phase 2 test additions:

| Instruction / Family | Canonical Opcodes | Boundary & Edge Conditions | Pre-Phase 2 Coverage | Phase 2 Verification Added | Status |
|:---|:---:|:---|:---|:---|:---:|
| **Control & Halt** | `0x00` (NOP)<br>`0x01` (HALT) | PC wrapping at `0xFFFF`; HALT state transition; repeated stepping when halted | `test_cpu_instructions.cpp:27-56` | Sequential stepping boundaries; no-op when halted | **Verified** |
| **Immediate Loads** | `0x10` (LDI)<br>`0x14` (LDI16) | Boundary immediates (`0x00, 0xFF, 0xFFFF`); 3-byte fetch wraparound at `0xFFFE` | `test_cpu_instructions.cpp:58-137` | Unaffected register isolation; invalid register ID fault | **Verified** |
| **Register Transfer** | `0x11` (MOV) | Self-transfer (`MOV R0, R0`); all 16 register pairs; invalid dst/src | `test_cpu_instructions.cpp:88-106` | Invalid register encoding validation for both operands | **Verified** |
| **Direct Load/Store** | `0x12` (LD)<br>`0x13` (ST) | 4-byte fetch across `0xFFFD, 0xFFFE, 0xFFFF`; target memory at `0xFFFF`; invalid reg | Single test at `0x0200` | Dedicated boundary wraparound tests at `0xFFFD, 0xFFFE, 0xFFFF` | **Verified** |
| **Indexed Load/Store** | `0x15` (LDX)<br>`0x16` (STX) | Access at `IX = 0x0000, 0xFFFF`; invalid reg | `test_cpu_instructions.cpp:140-176` | Register encoding validation; state atomicity | **Verified** |
| **Index Arithmetic** | `0x17` (INX)<br>`0x18` (DEX) | Modular rollover (`0xFFFF->0x0000`, `0x0000->0xFFFF`); flags untouched | `test_cpu_instructions.cpp:179-204` | Flag preservation test: assert all flags remain identical | **Verified** |
| **Addition** | `0x20` (ADD) | Carry transitions ($a+b > 255$); signed overflow ($+127+1, -128+-128$); zeros | `test_alu.cpp:4-36` | Exhaustive $256 \times 256$ input validation (65,536 cases); boundary matrix | **Verified** |
| **Subtraction & CMP**| `0x21` (SUB)<br>`0x40` (CMP) | Carry/no-borrow ($a \ge b$); signed underflow; identical CMP flags | `test_alu.cpp:64-124` | Exhaustive $256 \times 256$ input validation (65,536 cases each); boundary matrix | **Verified** |
| **Increment/Decrement**| `0x22` (INC)<br>`0x23` (DEC) | Carry preservation; overflow ($0x7F \to 0x80$, $0x80 \to 0x7F$); wrap ($0xFF \to 0x00$, $0x00 \to 0xFF$) | `test_alu.cpp:126-196` | Exhaustive 512 input/carry cases each; flag chaining sequences | **Verified** |
| **Bitwise Operations**| `0x30` (AND)<br>`0x31` (OR)<br>`0x32` (XOR)<br>`0x33` (NOT) | Sign bit masking; zero results; explicit clearing of $C=0, V=0$ | `test_alu.cpp:198-244` | Exhaustive $256 \times 256$ input validation; flag clearing verification | **Verified** |
| **Branches** | `0x50`..`0x54` | Branch taken vs not taken; branch to wrapped address `0x0005` | `test_cpu_instructions.cpp:450-536` | Unaffected register and flag isolation across branches | **Verified** |
| **Stack & Subroutine**| `0x60`..`0x63` | ADR-001 empty sentinel `0xFFFF`; overflow at `0xF000`; atomicity | `test_stack_instructions.cpp:25-256`| Invalid register ID validation for PUSH and POP | **Verified** |
| **Virtual I/O** | `0x70` (IN)<br>`0x71` (OUT) | FIFO empty read (0x00); console ASCII and hex escapes; direction mismatch | `test_io_instructions.cpp:28-256` | Register encoding validation for IN and OUT | **Verified** |

---

## 4. Four-Byte Instruction 64 KiB Wraparound Tests

The normative 4-byte instructions in BitForge are:
- `LD Rd, [addr16]` (Opcode `0x12`): 4 bytes (`0x12`, `Rd`, `addr16_hi`, `addr16_lo`).
- `ST [addr16], Rs` (Opcode `0x13`): 4 bytes (`0x13`, `addr16_hi`, `addr16_lo`, `Rs`).

### 4.1 Boundary Analysis & Mathematical Mapping

| Instruction | Start Address | Opcode Offset | Operand 1 Offset | Operand 2 Offset | Operand 3 Offset | Final PC |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|
| `LD R1, [0x8050]` | `0xFFFD` | `0xFFFD` (`0x12`) | `0xFFFE` (`0x01`) | `0xFFFF` (`0x80`) | `0x0000` (`0x50`) | `0x0001` |
| `LD R2, [0x8055]` | `0xFFFE` | `0xFFFE` (`0x12`) | `0xFFFF` (`0x02`) | `0x0000` (`0x80`) | `0x0001` (`0x55`) | `0x0002` |
| `LD R3, [0x8070]` | `0xFFFF` | `0xFFFF` (`0x12`) | `0x0000` (`0x03`) | `0x0001` (`0x80`) | `0x0002` (`0x70`) | `0x0003` |
| `ST [0x8060], R2` | `0xFFFD` | `0xFFFD` (`0x13`) | `0xFFFE` (`0x80`) | `0xFFFF` (`0x60`) | `0x0000` (`0x02`) | `0x0001` |
| `ST [0x8065], R3` | `0xFFFE` | `0xFFFE` (`0x13`) | `0xFFFF` (`0x80`) | `0x0000` (`0x65`) | `0x0001` (`0x03`) | `0x0002` |
| `ST [0x8075], R1` | `0xFFFF` | `0xFFFF` (`0x13`) | `0x0000` (`0x80`) | `0x0001` (`0x75`) | `0x0002` (`0x01`) | `0x0003` |

### 4.2 Test Implementations and Execution Results

All seven fixtures were implemented in `tests/unit/test_cpu_instructions.cpp`:
1. `CpuInstructions.FourByteInstruction_LdStraddling64K_FFFD`: Verified `R1 == 0x42`, `PC == 0x0001`. **Passed.**
2. `CpuInstructions.FourByteInstruction_LdStraddling64K_FFFE`: Verified `R2 == 0x77`, `PC == 0x0002`. **Passed.**
3. `CpuInstructions.FourByteInstruction_LdStraddling64K_FFFF`: Verified `R3 == 0x88`, `PC == 0x0003`. **Passed.**
4. `CpuInstructions.FourByteInstruction_StStraddling64K_FFFD`: Verified `memory[0x8060] == 0x99`, `PC == 0x0001`. **Passed.**
5. `CpuInstructions.FourByteInstruction_StStraddling64K_FFFE`: Verified `memory[0x8065] == 0xAA`, `PC == 0x0002`. **Passed.**
6. `CpuInstructions.FourByteInstruction_StStraddling64K_FFFF`: Verified `memory[0x8075] == 0x55`, `PC == 0x0003`. **Passed.**
7. `CpuInstructions.FourByteInstruction_TargetAddressAt64KBoundary`: Verified reading and writing target address `0xFFFF`. **Passed.**

---

## 5. Exhaustive Arithmetic & Flag Boundary Verification

To verify that the production ALU implementation contains no hidden edge-case defects, independent specification references were formulated directly from §6.2.

### 5.1 Independent Reference Formulation & Portability

Under ISO C++17, converting an unsigned byte value $\ge 128$ to a signed integer of the same width via `static_cast<int8_t>` is implementation-defined. To ensure 100% portable conformance across all host architectures, an explicit mathematical mapping function `to_signed_int8` was implemented:

$$\text{to\_signed\_int8}(v) = \begin{cases} v - 256 & \text{if } v \ge 128 \\ v & \text{if } v < 128 \end{cases}$$

- **Signed Overflow ($V$) Reference for ADD:**
  Mathematical signed sum: $S_{signed} = \text{to\_signed\_int8}(a) + \text{to\_signed\_int8}(b)$.
  $$V_{ref} = (S_{signed} < -128 \lor S_{signed} > 127)$$
  This algebraic definition is completely portable and independent of the production bitwise expression `((~(a ^ b) & (a ^ r) & 0x80) != 0)`.

- **Signed Overflow ($V$) Reference for SUB:**
  Mathematical signed difference: $D_{signed} = \text{to\_signed\_int8}(a) - \text{to\_signed\_int8}(b)$.
  $$V_{ref} = (D_{signed} < -128 \lor D_{signed} > 127)$$
  This algebraic definition is completely portable and independent of the production bitwise expression `(((a ^ b) & (a ^ r) & 0x80) != 0)`.

- **Carry ($C$) Reference for SUB:**
  $$C_{ref} = (a \ge b) \quad \text{(Carry generated / no unsigned borrow)}$$

### 5.2 Reconciled Metric Accounting: Operations, Invocations & Assertions

The table below provides a rigorous, reconciled breakdown of test metrics across all Phase 2 additions in `tests/unit/test_alu.cpp`.

*Definitions:*
- **Fixture-Evaluated Input Vectors:** The count of input parameter combinations evaluated by the specific fixture. (The total is the sum across fixtures, not a globally unique set of pairs, since common pairs such as $(0, 0)$ are evaluated across multiple distinct operation fixtures).
- **Target ALU Calls:** Direct invocations of the specific ALU production function under test.
- **Reference ALU Calls:** Additional calls to other ALU functions invoked strictly as comparison reference baselines.
- **Dynamic Assertion Evaluations:** Total assertions executed dynamically at runtime.
- **Static Assertion Sites:** The count of distinct assertion macro statement locations in source code.

| Test Fixture Name | Method Under Test | Fixture-Evaluated Input Vectors | Target ALU Calls | Reference ALU Calls | Dynamic Assertion Evaluations | Static Assertion Sites | Mismatches | Duration |
|:---|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| `AluTest.ExhaustiveAdd256x256` | `Alu::add(a, b)` | 65,536 | 65,536 | 0 | 1 | 1 | **0** | < 2 ms |
| `AluTest.ExhaustiveSub256x256` | `Alu::sub(a, b)` | 65,536 | 65,536 | 0 | 1 | 1 | **0** | < 2 ms |
| `AluTest.ExhaustiveCmpMatchesSubFlags256x256` | `Alu::cmp(a, b)` vs `Alu::sub` | 65,536 | 65,536 | 65,536 *(sub)* | 1 | 1 | **0** | < 2 ms |
| `AluTest.ExhaustiveIncAllInputsAndCarryStates` | `Alu::inc(a, flags)` | 512 | 512 | 0 | 1 | 1 | **0** | < 1 ms |
| `AluTest.ExhaustiveDecAllInputsAndCarryStates` | `Alu::dec(a, flags)` | 512 | 512 | 0 | 1 | 1 | **0** | < 1 ms |
| `AluTest.ExhaustiveBitwiseOperations256x256` | `not_op`, `and_op`, `or_op`, `xor_op` | 65,792 | 196,864 | 0 | 1 | 1 | **0** | < 5 ms |
| `AluTest.DeterministicBoundaryPairsMatrix` | Direct `add`, `sub`, `cmp` | 36 | 108 | 0 | 144 | 4 | **0** | < 1 ms |
| `AluTest.FlagChainingSequences` | Cascading sequential operations | 5 | 5 | 0 | 11 | 11 | **0** | < 1 ms |
| **Total `test_alu.cpp` Additions** | | **263,465** | **394,609** | **65,536** | **161** | **21** | **0** | **< 15 ms** |

*Note on Total ALU Calls:* Summing Target ALU Calls (394,609) and Reference ALU Calls (65,536) yields **460,145 total ALU function calls** executed in `test_alu.cpp`.

*Note on CPU Boundary Testing:* Separately, `CpuInstructions.ArithmeticInstructionBoundaryMatrix` in `tests/unit/test_cpu_instructions.cpp` evaluates the 36 boundary pairs through CPU execution, performing **36 CPU arithmetic instruction commits**, **36 reference `Alu::add` calls**, and **108 dynamic assertion evaluations** across 3 static assertion sites in source code.

---

## 6. State, Fault and Atomicity Tests

### 6.1 Register Encoding Validation Across All Instructions

The test `CpuInstructions.ExhaustiveRegisterEncodingValidationAndAtomicity` exercised 26 distinct instruction permutations with invalid register identifier `0x04` across all 19 register-consuming instruction forms:
- `LDI reg=4`
- `MOV dst=4`, `MOV src=4`
- `LD reg=4`
- `ST reg=4`
- `LDX reg=4`, `STX reg=4`
- `ADD dst=4`, `ADD src=4`
- `SUB dst=4`, `SUB src=4`
- `INC reg=4`, `DEC reg=4`
- `AND dst=4`, `AND src=4`
- `OR dst=4`, `OR src=4`
- `XOR dst=4`, `XOR src=4`
- `NOT reg=4`
- `CMP left=4`, `CMP right=4`
- `PUSH reg=4`, `POP reg=4`
- `IN reg=4`, `OUT reg=4`

*Scope Note:* This test specifically verifies register operand decoding and validation across every applicable operand position and syntax form using representative invalid value `0x04`; it does not enumerate all 252 invalid byte values (`0x04..0xFF`).

**Results for all 26 cases:**
1. `res.executed == false` and `res.faulted == true`.
2. `cpu.last_fault().type == FaultType::InvalidRegisterEncoding`.
3. `res.instruction_address == 0x0200` and `cpu.registers().pc == 0x0200` (Universal PC preservation rule verified).
4. State atomicity verified: zero changes to `R0..R3`, `IX`, `SP`, or status flags `Z`, `C`, `N`, `V`.

### 6.2 Invalid Opcode Coverage and Fault Atomicity

The test `CpuInstructions.InvalidOpcodeFaultAtomicityComprehensive` exercised 17 representative unmapped opcode bytes (`0x02`, `0x0F`, `0x19`, `0x1A`, `0x24`, `0x2F`, `0x34`, `0x3F`, `0x41`, `0x4E`, `0x55`, `0x5E`, `0x64`, `0x6F`, `0x72`, `0xFE`, `0xFF`) specifically to verify fault atomicity and universal PC preservation.

*Scope Note:* Full enumeration of all 225 undefined opcodes across the entire $0x00..0xFF$ range is performed by `CpuInstructions.ExhaustiveInvalidOpcodesFault` (`test_cpu_instructions.cpp:598`), which verifies that every non-canonical byte triggers a fault. The Phase 2 test complements this by asserting complete architectural state preservation (registers, flags, memory, SP, IX) across diverse unmapped ranges.

### 6.3 State Transitions and Reset Contract

The test `CpuInstructions.ResetFromFaultedAndHaltedRestoresDocumentedBaseline` confirmed:
- Stepping while in `Faulted` state is an explicit no-op (`executed = false`).
- Stepping while in `Halted` state is an explicit no-op (`executed = false`).
- `cpu.reset(entry)` from `Faulted` or `Halted` completely restores documented baseline:
  - `state == MachineState::Ready`
  - `PC == entry`
  - `SP == 0xFFFF`
  - `R0..R3 == 0`, `IX == 0`
  - `Flags == [Z=0, C=0, N=0, V=0]`
  - `last_fault().type == FaultType::None`

---

## 7. Self-Modifying Code and Execution Limits

### 7.1 Dynamic Code Mutation Observability

The BitForge architecture specifies a flat 64 KiB memory space without hardware distinction between code and data. The test `CpuInstructions.SelfModifyingCode_OverwritesSubsequentInstruction` tested this capability:
1. Instruction at `0x0200`: `LDI R0, 0x01` (loads HALT opcode).
2. Instruction at `0x0203`: `ST [0x0207], R0` (dynamically overwrites memory at `0x0207`, initially containing `NOP`).
3. Memory at `0x0207` is mutated to `0x01`.
4. Subsequent step fetches from `0x0207`: reads the newly stored `0x01` byte, decodes it as `HALT`, and transitions the CPU to `MachineState::Halted` with `PC = 0x0208`.
5. The subsequent instruction at `0x0208` never executes.

**Result:** Dynamic memory mutations to code memory are immediately observable on subsequent instruction fetches without pipeline stale-cache anomalies.

---

## 8. Test Execution Results and Preset Configuration Accounting

All unit and integration test targets were built and executed across all four CMake presets using the native MSYS2 UCRT64 toolchain.

### 8.1 Compiler Warning Policy by Preset

Inspection of `CMakePresets.json` and `CMakeLists.txt` establishes the exact warning enforcement configuration:
- **`ci` Preset:** Configures `BITFORGE_WARNINGS_AS_ERRORS: "ON"`. The build command explicitly appends `-Werror` to compiler flags. Completed cleanly with **0 compiler warnings and 0 errors**.
- **`debug` Preset:** Configures `BITFORGE_WARNINGS_AS_ERRORS: "OFF"`. Compiles with `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor`. Completed cleanly with **0 observed compiler warnings**.
- **`gui` Preset:** Configures `BITFORGE_WARNINGS_AS_ERRORS: "OFF"`. Compiles with `-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor`. Completed cleanly with **0 observed compiler warnings**.
- **`gui-release` Preset:** Configures `BITFORGE_WARNINGS_AS_ERRORS: "OFF"`. Compiles with `-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor`. Completed cleanly with **0 observed compiler warnings**.

### 8.2 Test Execution Results Summary

| CMake Preset | Warning Policy | Test Binaries Executed | Core Unit Tests | GUI QtTest Entries | Failures / Skips | Execution Duration |
|:---|:---|:---|:---:|:---:|:---:|:---:|
| **`ci`** | `-Werror` enforced | `bitforge_tests.exe` | **254 passed** | N/A *(GUI disabled)* | 0 / 0 | 0.29 s |
| **`debug`** | 0 warnings observed | `bitforge_tests.exe` | **254 passed** | N/A | 0 / 0 | 0.29 s |
| **`gui`** | 0 warnings observed | `bitforge_tests.exe`<br>`bitforge_gui_tests.exe` | **254 passed** | **83 passed**<br>*(81 methods + 2 fixture slots)* | 0 / 0 | 0.14 s *(core)*<br>12.61 s *(GUI)* |
| **`gui-release`** | 0 warnings observed | `bitforge_tests.exe`<br>`bitforge_gui_tests.exe` | **254 passed** | **83 passed**<br>*(81 methods + 2 fixture slots)* | 0 / 0 | 0.06 s *(core)*<br>12.09 s *(GUI)* |

### 8.3 Reconciliation of GUI Test Inventory

In CTest, the GUI test suite is represented by a single test target: `Test #2: BitForgeGuiTests`.
Within the standalone `bitforge_gui_tests.exe` executable (executed with `-platform offscreen`), QtTest executes:
- **81 test methods** (covering editor, assembler diagnostics, debugging controls, docks, themes, examples, tutorials, and accessibility).
- **2 lifecycle fixture slots** (`initTestCase()` and `cleanupTestCase()`).
- **Total QtTest Result:** `Totals: 83 passed, 0 failed, 0 skipped, 0 blacklisted` across both `gui` and `gui-release`.
- This matches the Phase 0 baseline inventory of 83 QtTest execution slots, confirming 100% GUI regression stability.

---

## 9. Confirmed Defects and Corrections

**Zero CPU implementation defects were discovered within the tested Phase 2 scope.**
The CPU execution loop, ALU flag calculation algorithms, transient address cursor for 4-byte fetches, register decoders, and fault abort mechanisms conformed to `docs/BitForge_Architecture_Specification_v1.7.md` across all evaluated test fixtures.

No fixes or modifications were required in `src/core/cpu.cpp` or `src/core/alu.cpp`.

---

## 10. Remaining Gaps and Specification Ambiguities

1. **Non-Architectural Stack Pointer Corruption (`SP < 0xF000`):**
   - In normative CPU execution, `SP` is provably confined to `0xF000..0xFFFF`. If an external debugger or uninitialized memory forces `SP < 0xF000`, POP and RET currently read from memory without checking a lower bound. Scheduled for evaluation in Phase 3.
2. **Assembler Omitted `.ENTRY` Educational Diagnostic:**
   - When `.ORG 0x0200` is used without `.ENTRY`, execution defaults to `0x0000`, running through uninitialized zero bytes (interpreted as `NOP`) until reaching `0x0200`. Conforms strictly to §5.3, but could benefit from an educational warning in Phase 4.

---

## 11. Updated Status of Phase 1 Findings

| Finding ID | Topic | Phase 1 Status | Phase 2 Action & Outcome | Updated Status |
|:---:|:---|:---:|:---|:---:|
| **Finding 1** | 4-byte instruction fetch wraparound near `0xFFFF` | **Coverage Gap** (Static logic verified; runtime tests missing) | Implemented 7 dedicated runtime unit fixtures (`0xFFFD, 0xFFFE, 0xFFFF` for both `LD` and `ST`). All passed. | **Verified & Closed** |
| **Finding 2** | POP/RET boundary behavior when `SP < 0xF000` | **Ambiguity** | Deferred to Phase 3 (Memory and Stack Integrity Audit). | Open (Phase 3) |
| **Finding 3** | `SystemBus` memory-write event accumulation in CLI | **Suspected Defect** | Deferred to Phase 5 (CLI) and Phase 12 (Stress). | Open (Phase 5) |
| **Finding 4** | Default entry point `0x0000` on `.ORG > 0x0000` | **Conformant** | Deferred to Phase 4 (Assembler diagnostics). | Open (Phase 4) |
| **Finding 5** | Watch Panel symbolic variable resolution | **Enhancement** | Deferred to Phase 8 (GUI). | Open (Phase 8) |
| **Finding 6** | Breakpoints placed on operand byte offsets | **Ergonomics** | Deferred to Phase 6 (Debugger). | Open (Phase 6) |

---

## 12. Statement of Files Changed and Unchanged

### Files Modified in Phase 2, 2.1 & 2.2
1. `docs/QA_PHASE0_BASELINE_REPORT.md` (Amended metadata block to remove trailing spaces; zero content changes).
2. `docs/QA_PHASE1_SPEC_CONFORMANCE_REPORT.md` (Amended preliminary pass: reconciled 4-byte instruction classification, aligned subtraction carry/no-borrow terminology, removed trailing spaces, avoided overclaiming).
3. `tests/unit/test_alu.cpp` (Added 8 exhaustive and boundary test fixtures: 256x256 ADD, 256x256 SUB, 256x256 CMP, 512 INC, 512 DEC, 256x256 Bitwise, boundary matrix, flag chaining; refactored signed overflow reference helper with portable `to_signed_int8`).
4. `tests/unit/test_cpu_instructions.cpp` (Added 13 new test fixtures: 4-byte boundary wraparounds, boundary matrix on CPU, register isolation, exhaustive register validation, comprehensive invalid opcodes, self-modifying code, reset contract).
5. `docs/QA_PHASE2_CPU_EDGE_CASE_REPORT.md` (Created Phase 2 deliverable and iteratively reconciled in Phase 2.1 and Phase 2.2 with verified operation accounting, GUI test reconciliations, reference portability, and compiler flag precision).

### Files Preserved Unchanged
- **Zero application source files** in `src/` or `include/` were modified.
- **Zero build configuration files** (`CMakeLists.txt`, `CMakePresets.json`) were altered.
- **Zero frozen architectural specifications** (`docs/BitForge_Architecture_Specification_v1.0.md` through `v1.7.md` and `docs/decisions/ADR-001-stack-pointer-convention.md`) were changed.
- **Zero assembly examples** in `examples/` were modified.
- **No changes were committed, staged, or pushed to any remote repository.**

---

## 13. Phase 2.1 & 2.2 Reconciliation Notes

### 13.1 Phase 2.1 Reconciliations
1. **ALU Operation Accounting:** Recalculated the exact iteration loops and operation invocations from `tests/unit/test_alu.cpp`. Reconciled the previous narrative discrepancy by formally distinguishing input combinations (263,465), target operations executed (394,609), total ALU function calls (460,145), dynamic assertion evaluations (161), and static assertion sites (21).
2. **GUI Test Verification:** Ran standalone `bitforge_gui_tests.exe` with `-platform offscreen` under both `gui` and `gui-release` presets. Captured the complete QtTest summary establishing that all 81 test methods and 2 fixture slots (83 total QtTest execution entries) passed with 0 failures and 0 skips, resolving the previous placeholder discrepancy.
3. **Compiler-Warning Policy:** Verified CMake cache variables and compiler invocation lines. Corrected the report to state that `ci` is the sole preset enforcing `-Werror`, while `debug`, `gui`, and `gui-release` completed cleanly with 0 observed warnings.
4. **Scope Precision:** Refined the scope descriptions for `InvalidOpcodeFaultAtomicityComprehensive` (17 representative opcodes tested for atomicity, complementing the 225 undefined opcode fault tests) and `ExhaustiveRegisterEncodingValidationAndAtomicity` (26 permutations across 19 instruction forms with invalid ID `0x04`).

### 13.2 Phase 2.2 Metric and Reference-Oracle Audit
1. **Signed-Value Reference Portability:** Replaced the implementation-defined cast `static_cast<int8_t>(byte_t)` in `AluTest.ExhaustiveAdd256x256` and `AluTest.ExhaustiveSub256x256` with a portable `constexpr int to_signed_int8(unsigned int val)` function. This mathematically interprets values $0x00..0x7F$ as $0..127$ and $0x80..0xFF$ as $-128..-1$ in standard `int`, eliminating all implementation-defined behavior under ISO C++17 while maintaining complete independence from the production ALU bitwise algorithm.
2. **Metric Categorization & Terminology Clarification:** Clarified that the 263,465 figure represents the sum of fixture-evaluated input vectors across individual test fixtures rather than a globally unique set of input pairs. Explicitly categorized and separated target ALU calls (394,609), reference ALU calls (65,536), dynamic assertion evaluations (161), and static assertion sites (21).

### 13.3 Phase 3.2 Scope Clarification
1. **CPU/ALU Scope Demarcation:** Formally clarified that Phase 2's principal verification scope was restricted to CPU execution, ALU arithmetic/logic correctness, instruction boundaries, and fault behavior. Explicitly documented that any preliminary Memory and SystemBus inspection performed in Phase 2 served solely to support instruction fetch boundaries (e.g. 4-byte wraparound), and that the dedicated comprehensive audit of `Memory`, `SystemBus`, virtual I/O, and ADR-001 stack discipline was performed in Phase 3.
