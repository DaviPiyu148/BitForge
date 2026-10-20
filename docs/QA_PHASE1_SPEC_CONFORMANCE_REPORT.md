# BitForge Phase 1 — Architecture & Specification Conformance Audit Report

- **Date of Audit:** October 20, 2026
- **Auditor:** Quality Assurance, CPU Architecture Verification & Systems Engineering Audit (assisted by Antigravity)
- **Target Repository:** `D:\BitForge`
- **Target Branch:** `master`
- **Document Version:** 1.1 (Phase 1 Baseline Conformance Deliverable — Reconciled)
- **Primary Architecture Authority:** `docs/BitForge_Architecture_Specification_v1.7.md`
- **Stack Architecture Authority:** `docs/decisions/ADR-001-stack-pointer-convention.md`

---

## 1. Executive Summary

This Phase 1 Conformance Audit performs a systematic, requirement-by-requirement verification of the BitForge emulator, assembler, system bus, command-line interface (CLI), and debugger implementation against the frozen architectural baseline defined in `docs/BitForge_Architecture_Specification_v1.7.md` and `docs/decisions/ADR-001-stack-pointer-convention.md`.

In strict adherence to Phase 1 non-destructive requirements, this audit is an analytical and verification exercise. No application source code, test implementations, build configuration files, assembly examples, or frozen architectural specifications were modified.

### Key Audit Findings

1. **Core Architectural Conformance (High Integrity):** The BitForge core CPU emulator (`src/core/cpu.cpp`), Arithmetic Logic Unit (`src/core/alu.cpp`), 64 KiB Memory model (`src/memory/memory.cpp`), System Bus (`src/bus/system_bus.cpp`), two-pass Assembler (`src/assembler/assembler.cpp`), and Debugger Session (`src/debugger/debugger_session.cpp`) demonstrate a high degree of fidelity to the v1.7 specification.
2. **Canonical Instruction Set (31/31 Conformant):** All 31 canonical instruction mnemonics, opcode byte encodings (`0x00` through `0x71`), instruction lengths (1, 2, 3, and 4 bytes), register operands (`R0..R3`), and flag side effects (`Z`, `C`, `N`, `V`) match the normative tables in §6.1 and §6.2.
3. **Stack Pointer & Subroutine Discipline (ADR-001 Full Compliance):** The implementation strictly follows ADR-001:
   - Initial empty-stack sentinel: `SP = 0xFFFF`.
   - Reserved storage: `0xF000..0xFFFE` (4,095 bytes maximum capacity).
   - Occupied top-of-stack semantics: `SP` points directly to the latest written byte when non-empty.
   - PUSH pre-validation: `SP > 0xF000` (decrements before writing).
   - POP pre-validation: `SP < 0xFFFF` (reads before incrementing).
   - CALL return address: 2-byte big-endian push at `SP - 2` (`ret_hi`) and `SP - 1` (`ret_lo`), advancing `PC = addr16`.
   - RET return address: 2-byte big-endian pop from `[SP]` and `[SP + 1]`, advancing `PC = (ret_hi << 8) | ret_lo`.
   - Fault atomicity: Universal PC preservation with zero architectural state mutations.
4. **Confirmed Discrepancies (Documentary / Structural Only):**
   - **Spec File Locations:** `v1.7.md` §13 states specification markdown files reside at the repository root; commit `6094caa` consolidated them into `docs/`.
   - **Virtual Device Directory Structure:** `v1.7.md` §13 illustrates separate `include/bitforge/devices/` and `src/devices/` directories; the actual implementation integrates virtual devices directly into `SystemBus` (`include/bitforge/bus/system_bus.hpp` and `src/bus/system_bus.cpp`).
5. **Suspected Resource Leak (CLI Long Execution):**
   - `SystemBus::write_byte` unconditionally pushes records into `memory_write_events_`. While `DebuggerSession` drains this log every step, the headless CLI runner (`src/cli/runner.cpp`) never clears it. Programs performing millions of writes will accumulate unbounded heap memory.
6. **Coverage Gaps for Phase 2–4:**
   - 4-byte instruction wraparound (`LD Rd, [addr16]` and `ST [addr16], Rs`) straddling `0xFFFF / 0x0000` (static fetch logic verified; dedicated runtime test coverage missing).
   - Non-architectural stack pointer state defense (`SP < 0xF000` before POP/RET).
   - Omitted `.ENTRY` on programs beginning at `.ORG > 0x0000` (silently executes uninitialized NOP gaps from `0x0000`).

---

## 2. Repository State and Scope

| Property | Value | Evidence / Command Output |
|:---|:---|:---|
| **Working Directory** | `D:\BitForge` | `Get-Location` |
| **Git Repository Root** | `D:/BitForge` | `git rev-parse --show-toplevel` |
| **Active Branch** | `master` | `git status --short --branch` |
| **Tracking Status** | `## master...origin/master [ahead 1]` | 1 local commit ahead of origin |
| **HEAD Commit Hash** | `5a88e11b7833a119ed847733bcc4db2bc4cbbbfa` | `git rev-parse HEAD` |
| **HEAD Commit Description**| `fix(gui,ux): finalize design token geometry and audit evidence` | Commit log |
| **Preflight State** | Working tree clean of tracked modifications; `docs/QA_PHASE0_BASELINE_REPORT.md` present as an untracked baseline artifact. | `git status --short` |

### Audit Scope & Boundaries
- **In Scope:** Static inspection, code path tracing, requirement mapping, and verification of existing tests across CPU core, memory, bus, assembler, CLI, and debugger components.
- **Out of Scope (Deferred):** Exhaustive arithmetic fuzzing (Phase 2), memory stress campaigns (Phase 3 & Phase 12), full GUI usability evaluations (Phase 8 & Phase 9), code refactoring, and defect fixes.

---

## 3. Specification Documents and Sections Inspected

| Authority Document | Version / Status | Inspected Sections | Scope of Verification |
|:---|:---|:---|:---|
| `docs/BitForge_Architecture_Specification_v1.7.md` | v1.7 (Approved active baseline) | §1, §2, §4, §5, §6, §7, §8, §9, §10, §11, §12, §13, §14 | Traced architectural, hardware, instruction set, memory model, bus, assembler, fault, and ADS testing requirements in scope for Phase 1. |
| `docs/decisions/ADR-001-stack-pointer-convention.md` | Accepted Architectural Decision | Context, Decision 1–8, Consequences | Authoritative definition of empty stack sentinel (`0xFFFF`), occupied TOS semantics, push/pop/call/ret pre-validations, and fault atomicity. |
| `docs/BitForge_Architecture_Specification_v1.0.md` .. `v1.6.md` | Frozen Historical Baselines | Evolution history & changelogs | Verified preservation of historical specifications; confirmed v1.7 as active authority. |

---

## 4. Architecture Requirement-to-Implementation Matrix

The following matrix traces every normative requirement from `v1.7.md` and `ADR-001` to its implementing source code and verification test evidence.

| ID | Requirement Statement | Spec Ref | Implementation Ref | Existing Test Evidence | Status | Notes / Risk |
|:---|:---|:---|:---|:---|:---:|:---|
| **REQ-CORE-01** | Instruction set contains exactly 31 defined instruction mnemonics across 4 functional groups. | §1.1, §6, §6.1 | `include/bitforge/core/opcodes.hpp:11`<br>`include/bitforge/core/instruction_info.hpp:46` | `test_types.cpp:58`<br>`test_assembler.cpp:26` | **Conformant** | 31 distinct mnemonics verified by compile-time array and runtime tests. |
| **REQ-CORE-02** | Instruction length distribution: five 1-byte, seven 2-byte, seventeen 3-byte, and two 4-byte instructions. | §6.1 | `include/bitforge/core/opcodes.hpp:73` | `test_types.cpp:81`<br>`test_assembler.cpp:26` | **Conformant** | Length mapping function `instruction_length()` verified for all 256 byte values. |
| **REQ-CORE-03** | Register IDs strictly restricted to `R0=0`, `R1=1`, `R2=2`, `R3=3`. IDs `> 3` trigger `InvalidRegisterEncoding`. | §6.1 | `src/core/cpu.cpp:11`<br>`include/bitforge/core/constants.hpp:24` | `test_cpu_instructions.cpp:542`<br>`test_stack_instructions.cpp:256` | **Conformant** | `is_valid_reg(r)` returns `r <= 3`; pre-validation rejects invalid IDs before execution. |
| **REQ-CORE-04** | ADD flag semantics: `Z=(r==0)`, `C=(a+b>0xFF)`, `N=(r&0x80)!=0`, `V=((~(a^b)&(a^r)&0x80)!=0)`. | §6.2 | `src/core/alu.cpp:5-15` | `test_alu.cpp:4`<br>`test_alu.cpp:38` | **Conformant** | Mathematical formula in `Alu::add` matches specification identically. |
| **REQ-CORE-05** | SUB / CMP flag semantics: `Z=(r==0)`, `C=(a>=b)`, `N=(r&0x80)!=0`, `V=(((a^b)&(a^r)&0x80)!=0)`. | §6.2 | `src/core/alu.cpp:17-30` | `test_alu.cpp:64`<br>`test_alu.cpp:90`<br>`test_alu.cpp:108` | **Conformant** | Flag `C` follows the canonical carry/no-borrow convention: `C = 1` indicates carry generated (no borrow, `a >= b`), while `C = 0` indicates a borrow (`a < b`). CMP produces identical flags. |
| **REQ-CORE-06** | INC and DEC update `Z`, `N`, `V` (`V` on `0x7F->0x80` or `0x80->0x7F`) while explicitly preserving `C`. | §6.2 | `src/core/alu.cpp:32-48` | `test_alu.cpp:126`<br>`test_alu.cpp:158` | **Conformant** | `Flags f = current_flags;` guarantees carry bit preservation across increments/decrements. |
| **REQ-CORE-07** | Bitwise operations (AND, OR, XOR, NOT) update `Z`, `N` and explicitly clear `C=0, V=0`. | §6.2 | `src/core/alu.cpp:50-88` | `test_alu.cpp:198`<br>`test_cpu_instructions.cpp:291` | **Conformant** | Flags `C` and `V` are set to `false` in all bitwise ALU methods. |
| **REQ-CORE-08** | Sequential PC advancement wraps strictly modulo 65,536 (`0xFFFF -> 0x0000`). | §5.4 | `src/core/cpu.cpp:77, 94` | `test_cpu_instructions.cpp:759` | **Conformant** | `cursor = static_cast<word_t>((cursor + 1) & 0xFFFF);` guarantees rollover. |
| **REQ-CORE-09** | Multi-byte instruction fetch across 64 KiB boundary wraps safely without out-of-bounds reads. | §5.4 | `src/core/cpu.cpp:89-95` | `test_cpu_instructions.cpp:769` | **Conformant (Static Logic Verified; Runtime Test Gap)** | Static code inspection confirms wraparound via `(cursor + 1) & 0xFFFF`; 3-byte `LDI` verified by runtime test at `0xFFFE`. Dedicated runtime test fixtures for 4-byte instructions (`LD Rd, [addr16]` and `ST [addr16], Rs`) are missing and scheduled for Phase 2. |
| **REQ-CORE-10** | Index register arithmetic wraps strictly modulo 65,536 (`INX: 0xFFFF->0x0000`, `DEX: 0x0000->0xFFFF`). | §5.4 | `src/core/cpu.cpp:218-230` | `test_cpu_instructions.cpp:179` | **Conformant** | Masking with `& 0xFFFF` enforces 16-bit modular rollover without affecting status flags. |
| **REQ-CORE-11** | HALT stops execution normally, transitions machine to `Halted`, and advances `PC` by 1. | §6.1, §7.4 | `src/core/cpu.cpp:125-130` | `test_cpu_instructions.cpp:39` | **Conformant** | PC set to `cursor` (`start_pc + 1`); machine state transitions to `MachineState::Halted`. |
| **REQ-MEM-01** | Flat 64 KiB address space (`0x0000..0xFFFF`) containing exactly 65,536 addressable bytes. | §5.1 | `include/bitforge/core/constants.hpp:9`<br>`include/bitforge/memory/memory.hpp:24` | `test_memory.cpp:7` | **Conformant** | `std::array<byte_t, 65536> storage_;` allocated in flat memory model. |
| **REQ-MEM-02** | Machine reset zero-initializes all 65,536 bytes of memory storage. | §5.3 | `src/memory/memory.cpp:6, 34` | `test_memory.cpp:44` | **Conformant** | `storage_.fill(0x00)` executed on initialization and `reset()`. |
| **REQ-MEM-03** | 16-bit word operations adhere to big-endian ordering (`memory[addr] = hi`, `memory[addr+1] = lo`). | §5.1 | `src/memory/memory.cpp:19-32`<br>`include/bitforge/core/types.hpp:18` | `test_memory.cpp:25`<br>`test_types.cpp:28` | **Conformant** | `make_word(hi, lo)`, `high_byte()`, and `low_byte()` helpers enforce big-endian storage. |
| **REQ-MEM-04** | Word read/write boundary wraps modulo 65,536 across `0xFFFF / 0x0000`. | §5.4 | `src/memory/memory.cpp:21, 30` | `test_memory.cpp:34`<br>`test_bus.cpp:23` | **Conformant** | High byte at `0xFFFF`, low byte at `(0xFFFF + 1) & 0xFFFF = 0x0000`. |
| **REQ-MEM-05** | Memory regions: Code (`0x0000..0x7FFF`), Data (`0x8000..0xEFFF`), Stack (`0xF000..0xFFFE`), Sentinel (`0xFFFF`). | §5.5 | `include/bitforge/core/constants.hpp:14-20` | `test_types.cpp:89` | **Conformant** | Constants defined. Assembler enforces `0xF000` emission boundary. |
| **REQ-MEM-06** | Segment loading validates bounds against remaining capacity, preventing buffer overruns. | §5.4 | `src/memory/memory.cpp:38-57` | `test_memory.cpp:57-133` | **Conformant** | `length > (kMemorySize - start_idx)` arithmetic overflow-safe check prevents overrun. |
| **REQ-STACK-01** | Stack pointer initializes to empty-stack sentinel `SP = 0xFFFF` on CPU reset. | ADR-001, §5.3 | `src/core/cpu.cpp:34`<br>`include/bitforge/core/registers.hpp:33` | `test_stack_instructions.cpp:25`<br>`test_debugger.cpp:40` | **Conformant** | Verified across all test fixtures. |
| **REQ-STACK-02** | Occupied top-of-stack semantics: When non-empty (`SP < 0xFFFF`), `SP` points directly to top occupied byte. | ADR-001 | `src/core/cpu.cpp:414, 432` | `test_stack_instructions.cpp:25` | **Conformant** | PUSH decrements before write; POP reads before increment. Full-descending stack convention. |
| **REQ-STACK-03** | PUSH pre-validates `SP > 0xF000`. If `SP <= 0xF000`, triggers immediate `StackOverflow` fault. | ADR-001, §5.6 | `src/core/cpu.cpp:410` | `test_stack_instructions.cpp:98` | **Conformant** | Pre-validation aborts without modifying `SP`, registers, flags, or memory. |
| **REQ-STACK-04** | POP pre-validates `SP < 0xFFFF`. If `SP >= 0xFFFF`, triggers immediate `StackUnderflow` fault. | ADR-001, §5.6 | `src/core/cpu.cpp:428` | `test_stack_instructions.cpp:80` | **Conformant** | Empty stack pop aborts without modifying `SP` or destination register. |
| **REQ-STACK-05** | CALL pre-validates `SP >= 0xF002`, pushes return address in big-endian order (`SP-2=hi`, `SP-1=lo`), `SP=SP-2`. | ADR-001, §5.6 | `src/core/cpu.cpp:442-455` | `test_stack_instructions.cpp:163, 190` | **Conformant** | Return address is address following CALL instruction (`start_pc + 3`). |
| **REQ-STACK-06** | RET pre-validates `SP <= 0xFFFD`, pops big-endian return address (`ret_hi=[SP]`, `ret_lo=[SP+1]`), `SP=SP+2`. | ADR-001, §5.6 | `src/core/cpu.cpp:462-470` | `test_stack_instructions.cpp:128, 163` | **Conformant** | Restores PC and increments stack pointer by 2. |
| **REQ-STACK-07** | Stack fault atomicity: Any stack fault halts in `MachineState::Faulted`, preserving PC, registers, flags, and memory. | ADR-001, §9.2 | `src/core/cpu.cpp:41–52` | `test_cpu_instructions.cpp:681`<br>`test_debugger.cpp:1194` | **Conformant** | Universal PC preservation verified across stack overflow and underflow tests. |
| **REQ-BUS-01** | `IBus::can_read_io` and `can_write_io` queries are strictly `const noexcept` and side-effect-free. | §7.2 | `src/bus/system_bus.cpp:93-100` | `test_bus.cpp:29`<br>`test_io_instructions.cpp:77` | **Conformant** | Verified queries do not alter FIFO buffers, console text, or LED state. |
| **REQ-BUS-02** | Port `0x00` (Input FIFO): Reads next byte. Empty read returns `0x00`, logs `InputUnderflow`, does not fault. | §7.1, §7.2 | `src/bus/system_bus.cpp:41-51` | `test_io_instructions.cpp:28, 58` | **Conformant** | Underflow counter incremented; zero returned; no CPU fault triggered. |
| **REQ-BUS-03** | Port `0x01` (Text Console Output): Appends ASCII characters (`0x20..0x7E`); non-printable bytes format as `\xNN`. | §7.1 | `src/bus/system_bus.cpp:63-74` | `test_io_instructions.cpp:93, 116` | **Conformant** | Printable strings and hex escape sequences verified. |
| **REQ-BUS-04** | Port `0x02` (Numeric Console Output): Appends unsigned decimal integers (`0..255`) separated by spaces. | §7.1 | `src/bus/system_bus.cpp:76-83` | `test_io_instructions.cpp:132` | **Conformant** | Formatted output matches spec format identically. |
| **REQ-BUS-05** | Port `0x10` (Virtual LEDs): Stores and displays written byte on 8 virtual LEDs. | §7.1 | `src/bus/system_bus.cpp:85-88` | `test_io_instructions.cpp:158` | **Conformant** | LED byte state persisted and readable via `bus.led_state()`. |
| **REQ-BUS-06** | Unmapped ports or direction mismatches (`OUT 0x00`, `IN 0x01..0x10`) trigger `FaultType::InvalidPort`. | §7.1, §7.2 | `src/core/cpu.cpp:484, 506` | `test_io_instructions.cpp:177, 227` | **Conformant** | Immediate CPU fault triggered with universal PC preservation. |
| **REQ-ASM-01** | Two-pass assembly architecture separating symbol collection (Pass 1) from machine-code emission (Pass 2). | §2, §8 | `src/assembler/assembler.cpp:860-878` | `test_assembler.cpp:201` | **Conformant** | Forward label references resolve cleanly in Pass 2. |
| **REQ-ASM-02** | Case-insensitive assembly mnemonics, directives, and register identifiers. | §8.1 | `src/assembler/assembler.cpp:17, 31` | `test_assembler.cpp:170` | **Conformant** | `to_upper()` applied to tokens; lowercase `r0` and `ldi` assemble identically. |
| **REQ-ASM-03** | Assembler directives `.ORG`, `.ENTRY`, `.BYTE`, `.WORD`, `.ASCII` supported with range enforcement. | §8.1 | `src/assembler/assembler.cpp:260-440` | `test_assembler.cpp:138, 834` | **Conformant** | Validates `imm8` (-128..255), `imm16` (-32768..65535), and string escapes. |
| **REQ-ASM-04** | Assembler rejects program data emitted at or beyond `0xF000` (reserved stack collision). | §5.5, §8.3 | `src/assembler/assembler.cpp:148, 278` | `test_assembler.cpp:289, 725` | **Conformant** | Directives attempting emission at `>= 0xF000` fail with error diagnostics. |
| **REQ-ASM-05** | Assembler rejects overlapping memory segments across multiple `.ORG` blocks. | §8.3 | `src/assembler/assembler.cpp:155` | `test_assembler.cpp:261` | **Conformant** | Address tracking set `written_addresses_` detects and rejects overwrites. |
| **REQ-ASM-06** | Default program entry point is `0x0000` when `.ENTRY` directive is omitted. | §5.3 | `src/assembler/assembler.cpp:100` | `test_assembler.cpp:605` | **Conformant** | ProgramImage entry defaults to `0x0000` per §5.3. |
| **REQ-FAULT-01** | Defined fault model: `InvalidOpcode`, `InvalidRegisterEncoding`, `InvalidPort`, `StackOverflow`, `StackUnderflow`. | §9.1 | `include/bitforge/core/faults.hpp:11` | `test_types.cpp:94`<br>`test_cpu_instructions.cpp:598` | **Conformant** | Exactly 5 canonical fault types defined and handled across CPU dispatch. |
| **REQ-FAULT-02** | Universal PC preservation rule: PC remains at instruction start address for all fault types. | §9.2 | `src/core/cpu.cpp:41-52` | `test_cpu_instructions.cpp:681`<br>`test_debugger.cpp:1155` | **Conformant** | `res.instruction_address = pc` and `registers_.pc` preserved across all faults. |
| **REQ-FAULT-03** | Zero partial mutations: Registers, flags, memory, stack pointer, and devices remain unchanged on fault. | §9.2 | `src/core/cpu.cpp:80, 136, 411` | `test_cpu_instructions.cpp:681`<br>`test_io_instructions.cpp:256` | **Conformant** | Pre-validation aborts before any state mutation occurs. |
| **REQ-CLI-01** | CLI supports subcommands: `help`, `version`, `assemble`, and `run` with options `--max-steps` and `--dump-regs`. | §10.3 | `src/cli/runner.cpp:43-147` | `test_cli.cpp:27-186` | **Conformant** | Arguments parsed and executed; usage errors reported cleanly. |
| **REQ-CLI-02** | CLI exit codes conform: 0 (Success), 1 (Usage error), 2 (Source/assembly error), 3 (CPU fault), 4 (Budget exceeded). | §10.3 | `include/bitforge/cli/runner.hpp:20`<br>`src/cli/runner.cpp:180-353` | `test_cli.cpp:96-364` | **Conformant** | All 5 exit codes verified with dedicated integration tests. |
| **REQ-DEBUG-01** | Execution budget enforcement stops infinite loops with `InstructionBudgetExceeded`. | §12.1 | `src/debugger/debugger_session.cpp:232`<br>`tests/test_harness.hpp:145` | `test_debugger.cpp:270`<br>`test_cli.cpp:347` | **Conformant** | Maximum instruction steps limit enforced across harness and debugger. |
| **REQ-DEBUG-02** | Single-step execution contract: stepping from `Ready` transitions to `Running` (or `Halted`/`Faulted`). Stepping from `Halted`/`Faulted` is a no-op. | §7.3, §12.1 | `src/core/cpu.cpp:56-69, 101`<br>`src/debugger/debugger_session.cpp:144` | `test_cpu_instructions.cpp:831`<br>`test_debugger.cpp:40, 107` | **Conformant** | State transitions and terminal no-op stepping verified. |
| **REQ-DEBUG-03** | Breakpoint evaluation: Stops execution before instruction commit; continue resumes without trapping. | §12.1 | `src/debugger/debugger_session.cpp:244-273` | `test_debugger.cpp:148, 177` | **Conformant** | `last_breakpoint_hit_` one-time bypass allows smooth continue execution. |
| **REQ-DEBUG-04** | Debugger maintains bounded circular trace history capturing execution deltas. | §12.1 | `src/debugger/debugger_session.cpp:125`<br>`include/bitforge/debugger/trace.hpp` | `test_debugger.cpp:539, 926, 1102` | **Conformant** | Bounded ring buffer retains recent execution history without unbounded memory growth. |
| **REQ-ADS-01** | Bubble Sort benchmark (`03_bubble_sort.asm`): Sorts $N$ elements at `0x8000` within 100,000 steps. | §1.1, §12.2 | `examples/03_bubble_sort.asm` | `test_example_library.cpp:780` | **Conformant** | Verified across empty, 1-element, 2-element, duplicates, reverse, mixed, and 24 permutations. |
| **REQ-ADS-02** | Linear Search benchmark (`02_linear_search.asm`): Finds key in array, returning index in `R0` or `0xFF`. | §12.2 | `examples/02_linear_search.asm` | `test_example_library.cpp:740` | **Conformant** | Verified for first, middle, last, and absent elements. |
| **REQ-ADS-03** | Factorial subroutine benchmark (`04_factorial.asm`): Memory input $N$ at `0x8000`, returns $N!$ at `0x8001`, `SP` balance verified. | §12.2 | `examples/04_factorial.asm` | `test_example_library.cpp:700` | **Conformant** | Verified for 0!, 1!, 5! with strict assertion of `SP == 0xFFFF` on halt. |

---

## 5. Bidirectional Traceability Findings

### 5.1 Specification → Implementation Traceability
Every requirement identified in `v1.7.md` and `ADR-001` traces to a concrete implementation component in the codebase:
- **Instruction set:** 100% of 31 instructions map to dedicated dispatch cases in `src/core/cpu.cpp` and parser methods in `src/assembler/assembler.cpp`.
- **Memory & Stack:** 100% of memory boundaries and stack pre-validations map to `src/memory/memory.cpp` and `src/core/cpu.cpp`.
- **Virtual Devices:** 100% of defined I/O ports map to `src/bus/system_bus.cpp`.
- **CLI & Debugger:** 100% of subcommands, exit codes, and stepping states map to `src/cli/runner.cpp` and `src/debugger/debugger_session.cpp`.

### 5.2 Implementation → Specification Traceability
The audit identified several implementation behaviors that exist in the codebase but lack explicit mention or detail in the v1.7 specification:

1. **`SystemBus` Memory-Write Journaling:**
   - *Implementation:* `include/bitforge/bus/system_bus.hpp:18` defines `MemoryWriteEvent`, and `SystemBus::write_byte` logs every write to `memory_write_events_`.
   - *Specification Status:* Unspecified in `v1.7.md`. It is an internal implementation mechanism created to support `DebuggerSession::execute_single_step()` in generating `ExecutionDelta`.
   - *Assessment:* Permissible implementation detail for debugger observability, but introduces a resource accumulation issue in headless CLI runs.
2. **One-Time Breakpoint Bypass Mechanism:**
   - *Implementation:* `src/debugger/debugger_session.cpp:244-251` uses `bypass_initial_breakpoint` and `last_breakpoint_hit_` to resume execution past a hit breakpoint without infinite re-trapping.
   - *Specification Status:* Unspecified in `v1.7.md` (which only specifies that continue must work).
   - *Assessment:* Essential and standard debugger implementation technique; conformant with intent.
3. **Data-Only Program Warning:**
   - *Implementation:* `src/assembler/assembler.cpp:920-927` emits an informational warning if a program contains only data directives (`.BYTE`, `.WORD`, `.ASCII`) without executable instructions.
   - *Specification Status:* Unspecified in `v1.7.md`.
   - *Assessment:* Helpful educational diagnostic enhancement; does not violate specification.

---

## 6. Existing Test Evidence and Verification Commands

The core verification suite was executed directly against the built `ci` preset binaries using the native MSYS2 UCRT64 toolchain:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:PATH"
& "D:\BitForge\build\ci\bitforge_tests.exe"
```

**Verification Results:**
```text
====================================================
Running 233 BitForge test cases...
====================================================
[ RUN      ] ExampleLibrary.Fundamentals_NopHalt
[       OK ] ExampleLibrary.Fundamentals_NopHalt
...
[ RUN      ] TypesTest.InstructionInfoMetadata
[       OK ] TypesTest.InstructionInfoMetadata
====================================================
Test Summary: 233 passed, 0 failed out of 233 total tests.
====================================================
```
**Outcome:** All 233 unit tests passed with 0 failures in 0.14 seconds.

---

## 7. Confirmed Discrepancies

The audit confirmed **zero (0) architectural discrepancies** across the requirements traced and verified in this baseline phase (instruction semantics, ALU calculations, address wraparound, stack operations, or fault atomicity). Comprehensive edge-case verification, arithmetic fuzzing, and stress campaigns are scheduled for subsequent phases (Phases 2–12).

Two minor **structural / documentary discrepancies** were confirmed between `v1.7.md` §13 and the physical repository layout:

| Discrepancy ID | Item | Specification Description (§13) | Actual Implementation Layout | Impact | Resolution |
|:---|:---|:---|:---|:---:|:---|
| **DISC-01** | Specification File Locations | §13 lists all specifications (`BitForge_Architecture_Specification_v1.0.md` .. `v1.7.md`) at the repository root. | Consolidated into `docs/` (`docs/BitForge_Architecture_Specification_v*.md`) in commit `6094caa`. | None | Architectural authority rules explicitly recognize `docs/` as the frozen specification repository location. |
| **DISC-02** | Virtual Devices Directory | §13 illustrates separate `include/bitforge/devices/` and `src/devices/` directories. | Virtual devices (FIFO, text, numeric, LEDs) are implemented directly inside `SystemBus` (`include/bitforge/bus/system_bus.hpp` and `src/bus/system_bus.cpp`). | None | Architectural functionality is 100% satisfied; decoupling is preserved via `IBus`. |

---

## 8. Suspected Defects and Unverified Requirements

### Suspected Defect: Unbounded Event Journaling in CLI Runner
- **Location:** `src/cli/runner.cpp:263-275` and `src/bus/system_bus.cpp:18`
- **Description:** `SystemBus::write_byte` appends a `MemoryWriteEvent` on every memory write. During CLI execution (`bitforge run`), these events are never cleared. An assembly program performing 10,000,000 writes in a loop will allocate millions of event structs on the heap (~160 MB+), leading to unnecessary memory inflation and cache degradation.
- **Severity:** Medium (Performance & Resource Management).
- **Follow-up:** Phase 5 (CLI behavior) and Phase 12 (Stress & performance).

### Test Coverage Gap: Runtime Verification of 4-Byte Instruction Wraparound
- **Location:** `src/core/cpu.cpp:89-95`
- **Description:** For 4-byte instructions (`LD Rd, [addr16]` and `ST [addr16], Rs`), starting at address `0xFFFD`, `0xFFFE`, or `0xFFFF` causes operand bytes to straddle into `0x0000`, `0x0001`, and `0x0002`. While static inspection of `src/core/cpu.cpp:89-95` confirms that transient cursor arithmetic is mathematically bounded modulo 65,536 (`(cursor + 1) & 0xFFFF`), current unit test fixtures only test a 3-byte `LDI` instruction straddling `0xFFFE`. Dedicated runtime test fixtures for 4-byte instruction wraparound across the 64 KiB boundary are absent.
- **Severity:** High (CPU Execution Edge Case).
- **Follow-up:** Phase 2 (CPU instruction edge cases).

---

## 9. Specification Ambiguities

### Ambiguity 1: Stack Pointer Definition in §5.2 vs ADR-001
- **Observation:** `v1.7.md` §5.2 stated that `SP` "points to the next free stack location", which informally describes an empty-descending stack. However, §5.3, §5.5, §5.6, and Table 6.1 specified `SP = 0xFFFF` as empty sentinel, valid storage at `0xF000..0xFFFE`, and PUSH decrementing before writing.
- **Resolution:** Formally resolved and superseded by `ADR-001-stack-pointer-convention.md`, which defines full-descending occupied top-of-stack semantics. The implementation strictly conforms to ADR-001.

### Ambiguity 2: Handling of Non-Architectural Stack Pointer Corruption
- **Observation:** Neither `v1.7.md` nor `ADR-001` specifies expected CPU behavior if `SP` is set below `0xF000` through external debugger manipulation or uninitialized state. The implementation checks `SP < 0xFFFF` for POP and `SP <= 0xFFFD` for RET, which is correct for all architecturally reachable states. Whether POP/RET should defensively check `SP >= 0xF000` is an unresolved edge case.
- **Resolution:** Documented as an edge case to be investigated in Phase 3.

---

## 10. Disposition of Phase 0 Findings

Each of the six findings identified during the Phase 0 baseline inspection was analyzed against the specification and implementation evidence:

| Finding | Topic | Architectural Requirement? | Actual Finding Classification | Disposition & Evidence-Based Rationale | Recommended Phase |
|:---:|:---|:---:|:---|:---|:---:|
| **1** | 4-byte instruction fetch wraparound near `0xFFFF` | **Yes** (§5.4) | **Coverage Gap** | Static code analysis confirms wraparound logic `(cursor + 1) & 0xFFFF`, but dedicated runtime test fixtures for 4-byte instructions at `0xFFFD..0xFFFF` are absent. | **Phase 2** |
| **2** | POP/RET boundary behavior when `SP < 0xF000` | **No** (Unreachable state) | **No Issue Found / Ambiguity** | Under architectural CPU execution, `SP` is provably confined to `0xF000..0xFFFF`. ADR-001 does not mandate faulting on POP/RET for `SP < 0xF000`. Conforms strictly to spec. | **Phase 3** |
| **3** | SystemBus memory-write accumulation in CLI | **No** (Implementation detail) | **Suspected Defect** | `MemoryWriteEvent` logging is an internal debugger mechanism that the CLI fails to drain, causing memory inflation on long runs. | **Phase 5 & 12** |
| **4** | Default entry point `0x0000` on `.ORG > 0` | **Yes** (§5.3) | **No Issue Found** | §5.3 explicitly mandates: `PC = program entry address (from .ENTRY, or default 0x0000)`. Conforms strictly to spec and tested by `test_assembler.cpp:605`. | **Phase 4** |
| **5** | Watch Panel symbolic label resolution | **No** (GUI feature) | **Usability Issue** | `v1.7.md` does not specify GUI watch expressions. It is a user-convenience enhancement for the desktop workbench, not an architectural defect. | **Phase 8** |
| **6** | Breakpoints placed on operand byte offsets | **No** (Debugger ergonomics) | **Usability / Ergonomics** | CPU breakpoints evaluate against `PC` at instruction boundaries. Gutter/F9 breakpoints resolve to opcodes via source maps; setting manual raw offsets is an ergonomics nuance. | **Phase 6** |

---

## 11. Recommended Regression Tests for Phases 2–6

Based on the audit findings, the following concrete regression test cases are recommended for subsequent phases:

### Phase 2: CPU Edge Cases
1. `TEST(CpuEdgeCases, FourByteInstructionStraddling64K_FFFD)`: Place `LD R0, [0x1234]` at `0xFFFD`. Verify opcode fetched from `0xFFFD`, `R0` from `0xFFFE`, `0x12` from `0xFFFF`, `0x34` from `0x0000`. Assert `PC = 0x0001` and `R0` loaded correctly.
2. `TEST(CpuEdgeCases, FourByteInstructionStraddling64K_FFFF)`: Place `LD R1, [0x5678]` at `0xFFFF`. Verify opcode at `0xFFFF`, operands at `0x0000`, `0x0001`, `0x0002`. Assert `PC = 0x0003`.
3. `TEST(CpuEdgeCases, SelfModifyingCode_OverwritingImmediate)`: Execute `ST` that modifies the operand byte of the immediately following `LDI`. Verify whether the CPU fetches the modified byte.

### Phase 3: Memory & Stack Integrity
4. `TEST(StackIntegrity, PopWithCorruptedSP_BelowF000)`: Force `cpu.registers().sp = 0x1000`. Verify whether `POP` executes or whether defensive faulting is desirable.
5. `TEST(StackIntegrity, RetWithCorruptedSP_BelowF000)`: Force `cpu.registers().sp = 0x1000`. Verify return address read behavior.

### Phase 4: Assembler Adversarial Testing
6. `TEST(AssemblerAdversarial, ImplicitEntryUninitializedGapWarning)`: Verify assembler diagnostics when `.ORG 0x0200` is defined without `.ENTRY`.
7. `TEST(AssemblerAdversarial, ExtremeLiteralBoundaryValues)`: Feed literals `-129`, `256`, `-32769`, `65536`, `0x10000`, `0b100000000`. Assert exact diagnostic line numbers and error recovery.

### Phase 5: Virtual I/O & CLI Behavior
8. `TEST(CliResourceStability, LongRunMemoryWriteEventsBounded)`: Execute a program performing 1,000,000 memory writes via CLI runner. Verify memory consumption remains bounded.

### Phase 6: Debugger Observability
9. `TEST(DebuggerObservability, BreakpointOnInstructionOperandOffset)`: Attempt setting breakpoint at `PC + 1`. Verify debugger warning or non-trigger behavior.

---

## 12. Prioritized Unresolved Issues

| Rank | Issue | Category | Risk / Impact | Proposed Follow-up |
|:---:|:---|:---:|:---:|:---|
| **1** | 4-byte instruction wraparound test coverage across 64 KiB boundary | Coverage Gap | High | Implement direct-memory straddling unit tests in Phase 2. |
| **2** | `SystemBus` memory-write event heap accumulation in CLI runs | Suspected Defect | Medium | Profile CLI memory usage in Phase 5; clear events or disable journaling in non-debugger runs. |
| **3** | Non-architectural stack pointer state behavior (`SP < 0xF000`) | Ambiguity | Medium | Evaluate defensive lower-bound checking on POP/RET in Phase 3. |
| **4** | Educational trap: Implicit entry point `0x0000` with non-zero `.ORG` | Usability | Low | Evaluate adding an optional diagnostic warning in Phase 4. |
| **5** | Watch Panel symbolic assembly variable resolution | Usability | Low | Implement symbol table lookup in `WatchPanel::parseExpression` in Phase 8. |

---

## 13. Statement of Files Changed and Not Changed

### Files Modified in Phase 1
1. `docs/QA_PHASE0_BASELINE_REPORT.md` (Amended strictly for the four preflight documentation corrections: distinguishing `bitforge` CLI executable from `bitforge_cli` library, clarifying initial clean vs post-Phase-0 working tree, acknowledging the new report in the integrity statement, and refining the WCAG 2.1 AA 3:1 non-text contrast criterion description).
2. `docs/QA_PHASE1_SPEC_CONFORMANCE_REPORT.md` (Newly created Phase 1 audit report deliverable).

### Files Preserved Unchanged
- **Zero application source files** in `src/` or `include/` were modified.
- **Zero test implementation files** in `tests/` were modified.
- **Zero build configuration files** (`CMakeLists.txt`, `CMakePresets.json`) were altered.
- **Zero frozen architectural specifications** (`docs/BitForge_Architecture_Specification_v1.0.md` through `v1.7.md` and `docs/decisions/ADR-001-stack-pointer-convention.md`) were changed.
- **Zero assembly examples** in `examples/` were modified.
- **No changes were committed, staged, or pushed to any remote repository.**
