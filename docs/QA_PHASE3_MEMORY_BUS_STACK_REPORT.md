# BitForge Quality Assurance Audit — Phase 3: Memory Model, System Bus, and Stack Integrity Audit

- **Document Version:** 1.2 (Phase 3.2 Closeout Deliverable)
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

---

## 1. Executive Summary & Audit Scope

Phase 3 of the BitForge Final QA Roadmap evaluates the integrity, correctness, and specification conformance of the 64 KiB memory subsystem, system bus memory/IO delegation, virtual peripheral interface, and stack/subroutine control flow under ADR-001. Phase 3.1 and Phase 3.2 execute narrow verification and documentation reconciliation passes to strengthen test evidence, align stack-range terminology with frozen specifications, and audit SystemBus event journal lifecycles.

All audit procedures are test-led, evidence-backed, and non-destructive. No frozen architecture specifications were modified. The production implementation was verified against formal invariants without requiring code alterations, as the underlying C++17 core demonstrably satisfies all examined normative requirements for the 64 KiB Memory model, System Bus delegation, virtual I/O peripherals, and the ADR-001 stack discipline.

### Key Audit Highlights

1. **Memory Model Contract:** The flat 64 KiB address space (`Memory`) is verified across all physical and architectural boundaries (`0x0000`, `0x7FFF`, `0x8000`, `0xEFFF`, `0xF000`, `0xFFFE`, `0xFFFF`). Neighboring byte isolation is confirmed across all boundary interfaces. Big-endian 16-bit word operations conform strictly to §5.4, including boundary spanning across 32 KiB halves (`0x7FFF`/`0x8000`), RAM-to-stack transition (`0xEFFF`/`0xF000`), and physical 64 KiB wraparound (`0xFFFF`/`0x0000`).
2. **Strengthened Storage Reset:** The memory reset contract is verified by populating all 65,536 addresses with a deterministic non-zero byte pattern `(i % 255) + 1` (strictly in $[1, 255]$), invoking `Memory::reset()`, and asserting that every address from `0x0000` to `0xFFFF` reads `0x00`.
3. **Segment Loading & Atomicity:** Segment boundary loading via `load_segment` accurately accepts exact-fit segments at `0xFFFE` (length 2) and `0xFFFF` (length 1), while atomically rejecting 1-byte overruns without mutating memory. Integer overflow attacks against segment length calculations are completely protected by remaining capacity comparisons.
4. **ADR-001 Stack Subsystem Conformance:** The CPU stack pointer and subroutine call/return mechanisms satisfy all eight normative rules in ADR-001. The report maintains a strict distinction between the architectural reserved stack memory region (`0xF000..0xFFFF`, 4,096 bytes allocated per v1.7 §5.2), valid stack-storage addresses (`0xF000..0xFFFE`, 4,095 bytes maximum capacity per ADR-001 §3), and the empty-stack sentinel (`SP = 0xFFFF`, which never stores stack data per ADR-001 §1). `PUSH` pre-validation halts at `SP = 0xF000`; `POP` pre-validation halts at `SP = 0xFFFF`; `CALL` pre-validation halts at `SP < 0xF002`; and `RET` pre-validation halts at `SP > 0xFFFD`. Subroutine return addresses follow big-endian byte placement (`ret_hi` at `SP - 2`, `ret_lo` at `SP - 1`).
5. **Non-Architectural State Analysis:** A formal reachability analysis establishes that the condition `SP < 0xF000` is impossible to produce via valid BitForge ISA execution. The private encapsulation of `registers_` and absence of an `LD SP, ...` instruction guarantees that `SP` remains strictly within $[0xF000, 0xFFFF]$ throughout all CPU lifecycles.
6. **System Bus & Virtual I/O Audit:** The system bus interface (`SystemBus`) delegates memory operations transparently to `Memory` and logs write events. Port permissions and virtual devices (Input FIFO `0x00`, Text Console `0x01`, Numeric Console `0x02`, LEDs `0x10`) conform to specification. An exhaustive 256-port scan confirms that all 252 unmapped ports are rejected without side effects. `reset_devices()` clears all virtual peripheral buffers while strictly preserving underlying RAM storage.
7. **Test Suite Expansion & Preset Pass Distribution:** The BitForge core test suite expanded from 254 unit tests to 265 unit tests (+11 new tests, +4.33%). All 265 core unit tests passed under all four CMake presets (`ci`, `debug`, `gui`, `gui-release`), while all 83 Qt GUI test execution entries (81 test methods + 2 fixture slots) passed under the GUI presets (`gui` and `gui-release`).

---

## 2. Memory Model Contract & Boundary Verification

The BitForge memory subsystem (`bitforge::Memory`) provides a flat, byte-addressable 64 KiB physical address space represented as an internal contiguous array:

$$\text{storage\_} \in \mathbb{B}^{65536}, \quad \text{where } \mathbb{B} = [0x00, 0xFF]$$

### 2.1 Boundary Mapping and Neighbor Byte Isolation

The architecture specification (§5.2) defines several key structural boundaries within the 16-bit address space:

| Boundary Address | Architectural Designation | Left Neighbor ($A - 1$) | Right Neighbor ($A + 1$) | Verification Status |
| :--- | :--- | :--- | :--- | :--- |
| `0x0000` | Physical lower bound, reset entry point default | `0xFFFF` (Modulo Wrap) | `0x0001` | **Conformant** |
| `0x7FFF` | Upper boundary of lower 32 KiB half | `0x7FFE` | `0x8000` | **Conformant** |
| `0x8000` | Lower boundary of upper 32 KiB half | `0x7FFF` | `0x8001` | **Conformant** |
| `0xEFFF` | General RAM upper bound (adjacent to reserved stack region) | `0xEFFE` | `0xF000` | **Conformant** |
| `0xF000` | Lower bound of reserved stack space (`kStackReservedStart`) and lowest valid stack-storage byte | `0xEFFF` | `0xF001` | **Conformant** |
| `0xFFFE` | Highest valid stack-storage byte (`kStackResetValue - 1`) | `0xFFFD` | `0xFFFF` | **Conformant** |
| `0xFFFF` | Physical upper bound; Empty Stack Sentinel (`kStackResetValue`) | `0xFFFE` | `0x0000` (Modulo Wrap) | **Conformant** |

In `tests/unit/test_memory.cpp`, `MemoryTest.BoundaryReadWriteAndNeighborIsolation` executes two exhaustive test passes:
1. **Isolated Boundary Write:** Each boundary is written with a unique non-zero byte while all remaining memory is zeroed. The test asserts that the boundary byte is correctly read back, while immediate neighboring bytes ($A-1$ and $A+1$) remain strictly `0x00`.
2. **Simultaneous Boundary Population:** All seven boundaries are populated simultaneously with distinct markers (`0x11` through `0x77`). The test verifies that each boundary retains its designated value and that non-boundary neighbors remain undisturbed.

### 2.2 16-Bit Word Operations and Modulo-65536 Wraparound

Per v1.7 §5.4, 16-bit word operations follow network/big-endian order:

$$\text{word\_value} = (\text{storage}[A] \ll 8) \mid \text{storage}[(A + 1) \bmod 65536]$$

`MemoryTest.WordAccessAcrossAllArchitecturalBoundaries` verifies word access across all critical partitions:
- **`0x0000`:** High byte at `0x0000`, low byte at `0x0001`.
- **`0x7FFF`:** Crosses 32 KiB half: high byte at `0x7FFF`, low byte at `0x8000`.
- **`0x8000`:** High byte at `0x8000`, low byte at `0x8001`.
- **`0xEFFF`:** Crosses general RAM into stack space: high byte at `0xEFFF`, low byte at `0xF000`.
- **`0xF000`:** High byte at `0xF000`, low byte at `0xF001`.
- **`0xFFFE`:** High byte at `0xFFFE`, low byte at `0xFFFF`.
- **`0xFFFF`:** Physical wraparound boundary: high byte stored at `0xFFFF`, low byte wrapped to `0x0000`. Full round-trip readback produces the original 16-bit word without truncating or faulting.

### 2.3 Segment Loading, Overflow Rejection, and Mutation Atomicity

`Memory::load_segment` provides bulk binary transfer into memory. Its bounds-checking logic was audited against potential integer overflow vulnerabilities:

```cpp
const std::size_t start_idx = static_cast<std::size_t>(start_address);
const std::size_t remaining_capacity = kMemorySize - start_idx;
if (length > remaining_capacity) {
    return false;
}
```

Since `start_idx` $\le 65535$, `remaining_capacity` is strictly within $[1, 65536]$. The comparison `length > remaining_capacity` operates on `std::size_t` and is immune to 16-bit or 32-bit integer wraparound.

`MemoryTest.LoadSegmentBoundaryExactFitAndOverflowAtomicity` tests boundary edge cases:
- **Exact-Fit at `0xFFFE`:** Length 2 fills `0xFFFE` and `0xFFFF`. Returns `true`.
- **1-Byte Overrun at `0xFFFE`:** Length 3 requires 3 bytes (`0xFFFE`, `0xFFFF`, `0x10000`). Returns `false`. Pre-existing data at `0xFFFE` and `0xFFFF` is completely preserved (demonstrating mutation atomicity).
- **Exact-Fit at `0xFFFF`:** Length 1 fills `0xFFFF`. Returns `true`.
- **1-Byte Overrun at `0xFFFF`:** Length 2 requires `0xFFFF` and `0x10000`. Returns `false`. Existing byte at `0xFFFF` is preserved and `0x0000` is untouched.
- **Null Pointer & Arithmetic Overflow Protection:** Non-zero length with `nullptr` returns `false`. Length of `std::numeric_limits<std::size_t>::max()` returns `false` at any address.

### 2.4 Strengthened Memory Reset Completeness

`Memory::reset()` executes `storage_.fill(0x00)`. In Phase 3.1, `MemoryTest.ResetComprehensiveStorageClear` was strengthened to populate every single address across the entire 64 KiB address space (65,536 bytes) with a deterministic non-zero pattern `(i % 255) + 1` (values strictly in $[1, 255]$). The test verifies non-zero presence across boundaries before invoking `reset()`, then iterates through all 65,536 addresses confirming that every address reads strictly `0x00`.

---

## 3. Stack Subsystem & ADR-001 Conformance

Architecture Decision Record `ADR-001` resolved historical drafting ambiguity regarding stack pointer semantics. The audit verified full conformance against all eight codified rules.

### 3.1 Stack Pointer Operational Model and Terminology

The architecture maintains an explicit distinction between three stack-related concepts:
1. **Architectural Reserved Stack Memory Region:** `0xF000..0xFFFF` (4,096 bytes total memory partition per v1.7 §5.2).
2. **Valid Stack-Storage Addresses:** `0xF000..0xFFFE` (4,095 bytes maximum capacity available for stack data per ADR-001 §3).
3. **Empty-Stack Sentinel:** `SP = 0xFFFF` (unoccupied stack pointer sentinel per ADR-001 §1; never stores stack data).

```
Address        Contents / Usage
0xFFFF         [Empty-Stack Sentinel] (Never holds stack data; SP = 0xFFFF when stack empty)
0xFFFE         Highest valid stack-storage byte (First PUSH writes here; SP becomes 0xFFFE)
  ...          Stack grows downward
0xF001         Second lowest valid stack-storage byte
0xF000         Lowest valid stack-storage byte (Last valid PUSH writes here; SP becomes 0xF000)
----------------------------------------------------------------------------------------
0xEFFF         General Purpose RAM (Stack access strictly forbidden; triggers StackOverflow)
```

### 3.2 ADR-001 Rule Audit Matrix

| Rule # | Requirement | Specification Clause | Implementation Tracing | Audit Verification Result |
| :--- | :--- | :--- | :--- | :--- |
| **Rule 1** | Empty Stack Sentinel | Initialized to `0xFFFF` on reset; never stores stack data | `Cpu::reset()` sets `registers_.sp = 0xFFFF` | **Pass:** Verified by `StackFixture` initialization |
| **Rule 2** | Occupied TOS Semantics | When `SP < 0xFFFF`, `SP` points directly to occupied byte | `bus_.write_byte(registers_.sp, ...)` after decrement | **Pass:** Verified by `PushAndPopSingleByte` |
| **Rule 3** | Valid Stack-Storage Range | Confined to `0xF000..0xFFFE` (4,095 bytes maximum capacity); `0xFFFF` reserved as empty sentinel | `kStackReservedStart = 0xF000` | **Pass:** Verified across all boundary tests |
| **Rule 4** | One-Byte `PUSH` | Requires `SP > 0xF000`. If `SP <= 0xF000`, `StackOverflow`. Decrements `SP`, writes `memory[SP] = Rs` | Pre-check `sp <= 0xF000` in `cpu.cpp` line 410 | **Pass:** Verified by `ExactBoundaryPushAtF001AndOverflowAtF000` |
| **Rule 5** | One-Byte `POP` | Requires `SP < 0xFFFF`. If `SP >= 0xFFFF`, `StackUnderflow`. Reads `memory[SP]`, increments `SP`, writes `Rd` | Pre-check `sp >= 0xFFFF` in `cpu.cpp` line 428 | **Pass:** Verified by `EmptyStackUnderflowFault` |
| **Rule 6** | Two-Byte `CALL` | Requires `SP >= 0xF002`. Return address stored big-endian: `ret_hi` at `SP-2`, `ret_lo` at `SP-1`. `SP = SP - 2` | Pre-check `sp < 0xF002` in `cpu.cpp` line 442 | **Pass:** Verified by `ExactBoundaryCallAtF002AndOverflowAtF001AndF000` |
| **Rule 7** | Two-Byte `RET` | Requires `SP <= 0xFFFD`. If `SP > 0xFFFD`, `StackUnderflow`. Loads `ret_hi` from `SP`, `ret_lo` from `SP+1`. `SP = SP + 2` | Pre-check `sp > 0xFFFD` in `cpu.cpp` line 462 | **Pass:** Verified by `RetUnderflowFault` |
| **Rule 8** | Fault Atomicity & PC Preservation | PC remains at faulting instruction; registers and memory unchanged | `record_fault()` restores PC and aborts write | **Pass:** Verified by `StackFaultAtomicityAndRegisterPreservation` |

### 3.3 Exact-Boundary Push and Call Execution

To verify stack boundaries without reliance on non-deterministic recursion, two dedicated deterministic tests were engineered in `tests/unit/test_stack_instructions.cpp`:

1. **`ExactBoundaryPushAtF001AndOverflowAtF000`:**
   - Preloads $R_0 = 0x5A$ and executes an in-memory loop of `PUSH R0; JMP 0x0200` for 4,094 iterations.
   - Advances `SP` from `0xFFFF` down to `0xF001` ($0xFFFF - 4094 = 0xF001$).
   - Asserts that memory at `0xF001` holds `0x5A`, `0xF000` is `0x00`, and `0xEFFF` is `0x00`.
   - Executes the final valid `PUSH R0` from `SP = 0xF001`: `SP` transitions to `0xF000`, `memory[0xF000]` becomes `0x5A`, and `memory[0xEFFF]` remains strictly `0x00` (neighbor isolation).
   - Executes a subsequent `PUSH R0` from `SP = 0xF000`: CPU immediately halts with `FaultType::StackOverflow`, `registers_.pc` is preserved at `0x0200`, `registers_.sp` remains `0xF000`, and `memory[0xEFFF]` is untouched.
2. **`ExactBoundaryCallAtF002AndOverflowAtF001AndF000`:**
   - Advances `SP` to `0xF002` via 4,093 pushes.
   - Executes `CALL 0x0300`: succeeds, updates `SP = 0xF000`, stores big-endian return address at `0xF000` (`0x02`) and `0xF001` (`0x03`), and leaves `memory[0xEFFF]` untouched.
   - Subsequent `CALL 0x0400` from `SP = 0xF000` faults with `StackOverflow`, preserving PC at `0x0300` and SP at `0xF000`.
   - Re-tests `CALL 0x0300` from `SP = 0xF001`: faults immediately with `StackOverflow`, preserving PC at `0x0200`, SP at `0xF001`, and leaving `memory[0xF000]` untouched.

### 3.4 Sentinel Address `0xFFFF` vs. RAM Independence

`StackTest.SentinelAddress0xFFFFGeneralMemoryIndependence` preloads RAM address `0xFFFF` with `0x7E`. While `SP = 0xFFFF`, a `PUSH` instruction executes, decrementing `SP` to `0xFFFE` and writing data to `0xFFFE`. The test proves:
- `memory.read_byte(0xFFFF)` remains `0x7E` throughout the push operation.
- Normal memory loads and stores (`LD`, `ST`) can read and write `0xFFFF` without perturbing the stack pointer.
- Subsequent `POP` restores `SP` to `0xFFFF` while leaving `memory[0xFFFF]` intact.

---

## 4. System Bus Delegation & Virtual I/O Audit

The `SystemBus` class mediates all memory and virtual I/O access between the CPU, memory, and peripherals.

### 4.1 Bus Memory Delegation and Event Journaling

The bus implements `IBus` and transparently routes memory operations to `Memory`:
- `bus.read_byte(addr)` $\rightarrow$ `memory.read_byte(addr)`
- `bus.write_byte(addr, val)` $\rightarrow$ `memory.write_byte(addr, val)` + logs `MemoryWriteEvent{addr, old_val, new_val}`
- `bus.read_word(addr)` $\rightarrow$ `memory.read_word(addr)`
- `bus.write_word(addr, val)` $\rightarrow$ `memory.write_word(addr, val)` + logs two sequential `MemoryWriteEvent` entries

`BusTest.BusBoundaryMemoryDelegationAndJournaling` verifies byte and word operations across all seven boundaries (`0x0000`, `0x7FFF`, `0x8000`, `0xEFFF`, `0xF000`, `0xFFFE`, `0xFFFF`). For a word write at `0xFFFF` with value `0xA1B2`, the bus journals two distinct events:
1. Address `0xFFFF`: old value `0x00`, new value `0xA1`
2. Address `0x0000`: old value `0x00`, new value `0xB2`

### 4.2 Virtual I/O Mapping & Permission Enforcement

The BitForge architecture specifies four mapped virtual I/O ports. `BusTest.UnmappedPortRejectionAndAtomicityMatrix` executes an exhaustive scan across all 256 possible 8-bit port IDs:

| Port Address | Device Name | Direction | Readable | Writable | Audit Verification |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `0x00` | Input FIFO Queue | Input | **Yes** | No | `can_read_io=true`, `can_write_io=false` |
| `0x01` | Text Console | Output | No | **Yes** | `can_read_io=false`, `can_write_io=true` |
| `0x02` | Numeric Console | Output | No | **Yes** | `can_read_io=false`, `can_write_io=true` |
| `0x10` | Virtual LEDs | Output | No | **Yes** | `can_read_io=false`, `can_write_io=true` |
| `0x03..0x0F` | Unmapped Ports (13 ports) | N/A | No | No | `can_read_io=false`, `can_write_io=false` |
| `0x11..0xFF` | Unmapped Ports (239 ports) | N/A | No | No | `can_read_io=false`, `can_write_io=false` |

**Verification Details:**
- For all 252 unmapped ports, `can_read_io` and `can_write_io` return `false`.
- Calls to `read_io(p)` return `std::nullopt` and generate zero I/O events.
- Calls to `write_io(p, val)` return `false` and generate zero I/O events.
- Reading an empty FIFO on port `0x00` returns `0x00`, logs an underflow event, increments `input_underflow_count_`, and does **not** fault the CPU.
- Text console output correctly renders printable ASCII characters (`0x20..0x7E`) and formats non-printable control bytes as hexadecimal escapes (`\xNN`).
- Numeric console output formats bytes as space-delimited unsigned decimal integers.
- LED register stores and returns 8-bit bitmasks.

### 4.3 Device Reset Separation of Concerns

`BusTest.BusResetDevicesClearsPeripheralsAndPreservesMemory` verifies `SystemBus::reset_devices()`:
- Clears input FIFO and resets underflow counters.
- Clears text output buffers and raw byte history.
- Clears numeric output lists and strings.
- Resets LED state to `0x00`.
- Clears event journals (`take_memory_write_events()`, `take_io_events()`).
- **RAM Preservation:** The test confirms that data previously written to RAM (`0x2000`, `0x3000`) remains strictly intact, proving that device reset does not accidentally wipe memory storage.

---

## 5. Fault Atomicity & Non-Architectural State Analysis

### 5.1 Universal Fault Atomicity

The BitForge architecture enforces fault atomicity across all CPU execution paths (§3.2):

> "Any instruction that encounters an unrecoverable condition must immediately record a fault, transition the machine state to Faulted, and preserve the program counter at the start address of the faulting instruction without committing partial state updates."

The audit verified fault atomicity across all stack and I/O error modes:
- **`StackUnderflow`:** When `POP` executes with `SP = 0xFFFF`, destination register $R_d$ retains its previous value; flags are unchanged; `SP` remains `0xFFFF`; `PC` points to the `POP` opcode.
- **`StackUnderflow` (Short Stack on `RET`):** When `RET` executes with `SP = 0xFFFE` (only 1 byte on stack), execution aborts; `SP` remains `0xFFFE`; `PC` points to the `RET` opcode.
- **`StackOverflow`:** When `PUSH` executes with `SP = 0xF000`, `SP` remains `0xF000`; source register $R_s$ is unchanged; RAM at `0xEFFF` and `0xF000` is unchanged; `PC` points to `PUSH`.
- **`InvalidPort`:** Attempting `IN` on output ports or `OUT` on input/unmapped ports leaves registers, FIFO, and peripheral buffers completely unmodified.
- **`InvalidRegisterEncoding`:** Supplying an invalid register ID aborts prior to stack decrements or memory mutations. Tested with representative invalid register IDs ($R_4, R_5, R_9$) during instruction dispatch, complemented by exhaustive unit verification of the `is_valid_reg()` predicate over the full byte domain $[0, 255]$ in `TypesTest`.

### 5.2 Reachability Analysis of $SP < 0xF000$

During architectural analysis, the question arose whether additional pre-validation or clamping is required for the condition $SP < 0xF000$. A formal reachability analysis was conducted:

1. **State Initialization:**
   In `Cpu::reset(entry_point)`, `registers_.sp` is unconditionally initialized to `kStackResetValue` (`0xFFFF`).
2. **Instruction Set Mutation Surface:**
   Of the 31 instruction opcodes in the BitForge ISA, exactly four instructions read or write the stack pointer:
   - `PUSH`: Modifies `SP` only if $SP > 0xF000$. Decrements by 1. Minimum reachable value is $0xF001 - 1 = 0xF000$. If $SP \le 0xF000$, execution faults and $SP$ is unchanged.
   - `POP`: Modifies `SP` only if $SP < 0xFFFF$. Increments by 1. Monotonically increases $SP$ towards $0xFFFF$.
   - `CALL`: Modifies `SP` only if $SP \ge 0xF002$. Decrements by 2. Minimum reachable value is $0xF002 - 2 = 0xF000$. If $SP < 0xF002$, execution faults and $SP$ is unchanged.
   - `RET`: Modifies `SP` only if $SP \le 0xFFFD$. Increments by 2. Monotonically increases $SP$ towards $0xFFFF$.
3. **No Direct SP Load Instruction:**
   The BitForge ISA possesses no `LD SP, ...`, `MOV SP, ...`, or stack pointer arithmetic instruction.
4. **Encapsulation:**
   In class `Cpu`, `registers_` is private and exposed only via `const Registers& registers() const noexcept`. No public mutator exists to inject an arbitrary stack pointer value.

$$\mathbf{Theorem:} \quad \forall t \ge 0, \quad SP(t) \in [0xF000, 0xFFFF]$$

$$\mathbf{Corollary:} \quad \text{The state } SP < 0xF000 \text{ is unreachable under normative execution.}$$

Consequently, existing pre-validations (`SP <= 0xF000` in PUSH and `SP < 0xF002` in CALL) are mathematically necessary and sufficient to prevent stack overflow into general RAM. The implementation is 100% compliant with ADR-001.

---

## 6. Comprehensive Test Matrix & Presets Verification

### 6.1 Presets Verification Results

All four CMake presets were built and executed using the MSYS2 UCRT64 toolchain (GCC 14.2.0, CMake 3.31.6, Ninja 1.13.0):

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:PATH"
cmake --build --preset <preset>
ctest --preset <preset> --output-on-failure
```

*Note on Test Execution Scope:* The core unit test suite (`bitforge_tests.exe`) containing 265 test cases was executed and passed under all four presets (`ci`, `debug`, `gui`, `gui-release`). The Qt GUI test suite (`bitforge_gui_tests.exe`) containing 83 test entries (81 test methods + 2 fixture slots) was built and executed under the GUI presets (`gui` and `gui-release`).

| Preset Name | Build Type | Target Artifacts | Core Unit Tests | GUI Tests | Duration | Result |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **`ci`** | Debug | `bitforge_tests.exe` | 265 / 265 Passed | N/A | 0.40s | **100% PASS** |
| **`debug`** | Debug | `bitforge.exe`, `bitforge_tests.exe` | 265 / 265 Passed | N/A | 0.40s | **100% PASS** |
| **`gui`** | Debug | `bitforge_gui.exe`, `bitforge_gui_tests.exe`, `bitforge_tests.exe` | 265 / 265 Passed | 83 / 83 Passed | 12.77s | **100% PASS** |
| **`gui-release`** | Release | `bitforge_gui.exe`, `bitforge_gui_tests.exe`, `bitforge_tests.exe` | 265 / 265 Passed | 83 / 83 Passed | 11.98s | **100% PASS** |

### 6.2 Phase 3 Unit Tests Inventory

| Test Identifier | Test File | Primary Architectural Focus | Assertions / Invariants Evaluated |
| :--- | :--- | :--- | :--- |
| `MemoryTest.BoundaryReadWriteAndNeighborIsolation` | `tests/unit/test_memory.cpp` | Physical boundary read/write isolation | Neighbor byte preservation at 7 boundaries (isolated & simultaneous) |
| `MemoryTest.WordAccessAcrossAllArchitecturalBoundaries` | `tests/unit/test_memory.cpp` | Big-endian word access & wrapping | Word read/write spanning `0x7FFF`, `0xEFFF`, `0xFFFE`, and `0xFFFF` wrap |
| `MemoryTest.LoadSegmentBoundaryExactFitAndOverflowAtomicity` | `tests/unit/test_memory.cpp` | Segment loader boundary atomicity | Exact-fit vs 1-byte overflow at `0xFFFE` and `0xFFFF`; zero mutation on error |
| `MemoryTest.ResetComprehensiveStorageClear` | `tests/unit/test_memory.cpp` | Complete 64 KiB RAM initialization | All 65,536 addresses populated with deterministic non-zero pattern `(i % 255) + 1`, reset to `0x00` |
| `StackTest.ExactBoundaryPushAtF001AndOverflowAtF000` | `tests/unit/test_stack_instructions.cpp` | ADR-001 §4 PUSH limit verification | 4,094-step loop to `0xF001`, valid push to `0xF000`, overflow at `0xF000` |
| `StackTest.ExactBoundaryCallAtF002AndOverflowAtF001AndF000` | `tests/unit/test_stack_instructions.cpp` | ADR-001 §6 CALL limit verification | Exact boundary CALL at `0xF002` to `0xF000`, overflow rejection at `0xF000`/`0xF001` |
| `StackTest.SentinelAddress0xFFFFGeneralMemoryIndependence` | `tests/unit/test_stack_instructions.cpp` | ADR-001 §1 Sentinel vs RAM independence | Proves RAM at `0xFFFF` is distinct from stack storage and unperturbed |
| `StackTest.StackFaultAtomicityAndRegisterPreservation` | `tests/unit/test_stack_instructions.cpp` | ADR-001 §8 Fault atomicity | Register & PC preservation on POP underflow and short-stack RET |
| `BusTest.BusResetDevicesClearsPeripheralsAndPreservesMemory` | `tests/unit/test_bus.cpp` | Bus device reset & RAM preservation | Peripheral buffer clearing vs underlying RAM separation of concerns |
| `BusTest.BusBoundaryMemoryDelegationAndJournaling` | `tests/unit/test_bus.cpp` | Bus memory routing & write journaling | Delegation and write event generation across all boundary addresses |
| `BusTest.UnmappedPortRejectionAndAtomicityMatrix` | `tests/unit/test_bus.cpp` | 256-port I/O security scan | Exhaustive rejection of all 252 unmapped ports without event logging |

---

## 7. Specification Conformance Matrix

| Specification Requirement | Section Reference | Concrete Architectural Assertion | Evidence / Test Case | Status |
| :--- | :--- | :--- | :--- | :--- |
| 64 KiB Flat Address Space | v1.7 §5.1 | 65,536 contiguous bytes addressable via 16-bit word | `MemoryTest.InitialStateIsZero`, `ResetComprehensiveStorageClear` | **Conformant** |
| Memory Segmentation Layout | v1.7 §5.2 | RAM `0x0000..0xEFFF`, Reserved Stack Region `0xF000..0xFFFF` | `MemoryTest.BoundaryReadWriteAndNeighborIsolation` | **Conformant** |
| Big-Endian Word Alignment | v1.7 §5.4 | High byte at $A$, low byte at $A+1$ | `MemoryTest.WordAccessAcrossAllArchitecturalBoundaries` | **Conformant** |
| Word Address Wraparound | v1.7 §5.4 | Access at `0xFFFF` wraps low byte to `0x0000` | `MemoryTest.WordBoundaryWrappingAcross64K`, `WordAccessAcrossAllArchitecturalBoundaries` | **Conformant** |
| Segment Loading Bounds | v1.7 §5.1 | Reject segments that overrun 64 KiB capacity | `MemoryTest.LoadSegmentOverrunRejection`, `LoadSegmentBoundaryExactFitAndOverflowAtomicity` | **Conformant** |
| System Bus Memory Routing | v1.7 §5.5 | Bus transparently routes reads/writes to memory | `BusTest.MemoryDelegationByteAndWord`, `BusBoundaryMemoryDelegationAndJournaling` | **Conformant** |
| System Bus Write Journaling | v1.7 §5.5 | Writes record old and new values for debugger delta | `BusTest.MemoryAndIoEventJournaling`, `BusBoundaryMemoryDelegationAndJournaling` | **Conformant** |
| Input FIFO Port `0x00` | v1.7 §5.6 | Readable only; underflow returns `0x00` without fault | `BusTest.IoReadContractDistinguishesEmptyFifoFromInvalidPort`, `IoTest.EmptyFifoUnderflowDoesNotFault` | **Conformant** |
| Text Console Port `0x01` | v1.7 §5.6 | Writable only; ASCII rendering with `\xNN` escape | `IoTest.TextConsolePrintableCharacters`, `TextConsoleNonPrintableEscapes` | **Conformant** |
| Numeric Console Port `0x02`| v1.7 §5.6 | Writable only; unsigned decimal output | `IoTest.NumericConsoleUnsignedDecimalOutput` | **Conformant** |
| Virtual LEDs Port `0x10` | v1.7 §5.6 | Writable only; 8-bit LED bitmask register | `IoTest.VirtualLedWriteAndRead` | **Conformant** |
| Unmapped Port Faulting | v1.7 §5.6 | Accessing undefined ports raises `InvalidPort` | `IoTest.UnmappedPortFaults`, `BusTest.UnmappedPortRejectionAndAtomicityMatrix` | **Conformant** |
| Empty-Stack Sentinel `0xFFFF`| ADR-001 §1 | Stack pointer initialized to `0xFFFF`; never stores stack data | `StackFixture`, `StackTest.PushAndPopSingleByte`, `SentinelAddress0xFFFFGeneralMemoryIndependence` | **Conformant** |
| Occupied TOS Semantics | ADR-001 §2 | Non-empty SP points to currently occupied byte | `StackTest.PushAndPopSingleByte`, `LifoRegisterRestoration` | **Conformant** |
| Valid Stack-Storage Range | ADR-001 §3 | Stack storage strictly confined to `0xF000..0xFFFE` (4,095 bytes); `0xFFFF` reserved as empty sentinel | `StackTest.ExactBoundaryPushAtF001AndOverflowAtF000` | **Conformant** |
| One-Byte PUSH Mechanics | ADR-001 §4 | Require `SP > 0xF000`; decrement then store | `StackTest.PushAndPopSingleByte`, `ExactBoundaryPushAtF001AndOverflowAtF000` | **Conformant** |
| One-Byte POP Mechanics | ADR-001 §5 | Require `SP < 0xFFFF`; read then increment | `StackTest.PushAndPopSingleByte`, `EmptyStackUnderflowFault` | **Conformant** |
| Subroutine CALL Mechanics | ADR-001 §6 | Require `SP >= 0xF002`; push big-endian ret addr | `StackTest.CallAndRetExecution`, `CallBigEndianReturnAddressLayout`, `ExactBoundaryCallAtF002...` | **Conformant** |
| Subroutine RET Mechanics | ADR-001 §7 | Require `SP <= 0xFFFD`; pop big-endian ret addr | `StackTest.CallAndRetExecution`, `RetUnderflowFault` | **Conformant** |
| Stack Fault Atomicity | ADR-001 §8 | PC preserved; registers and memory unchanged | `StackTest.StackFaultAtomicityAndRegisterPreservation` | **Conformant** |

---

## 8. Defect Log & Risk Inventory

### 8.1 Production Code Defects
- **Discovered Defects:** **0**. No specification-violating defects or regressions were found in `Memory`, `SystemBus`, or `Cpu`.
- **Production Code Changes:** **0 lines modified**. The existing production codebase in `src/` and `include/` was preserved in its entirety.

### 8.2 Test Engineering Corrections
During the implementation of Phase 3 test cases, one authoring defect occurred and was immediately resolved:
- **Test Authoring Correction (JMP Opcode Value):** In `tests/unit/test_stack_instructions.cpp`, the assembly loop constructing the 4,094-push chain initially encoded opcode `0x40` (the opcode for `CMP Rd, Rs`) instead of canonical opcode `0x50` (`JMP addr16`). This resulted in executing `CMP` instructions that advanced PC linearly rather than looping. The opcode was corrected to `0x50` in both `ExactBoundaryPushAtF001AndOverflowAtF000` and `ExactBoundaryCallAtF002AndOverflowAtF001AndF000`, following which both tests executed deterministically and passed.

### 8.3 Tracked Risk Items

| Risk Identifier | Component | Description | Current Status | Target Phase |
| :--- | :--- | :--- | :--- | :--- |
| **RISK-P3-001** | `SystemBus` Event Log | In headless CLI execution (`cli::execute_run`), `SystemBus::write_byte` and I/O operations accumulate `MemoryWriteEvent` and `IoEvent` records without drainage in the step loop. In contrast, `DebuggerSession::execute_single_step` drains them on each step. Memory footprint impact under long-running headless workloads has not been measured. | Not yet measured; scheduled for Phase 5 CLI/stress audit. | Phase 5 (CLI & Headless Execution Audit) |
| **RISK-P3-002** | Assembler Stack Collision | Directives or instructions assembled at or above `0xF000` could collide with the stack space if the assembler does not enforce ADR-001 §3. | Documented for validation during Phase 4 assembler audit. | Phase 4 (Assembler & Directives Verification) |

---

## 9. Quality Delta (Phase 2 to Phase 3)

| Metric | Phase 2 Baseline | Phase 3 Audit State | Net Delta | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Tracked Production Files** | Clean Git State | Clean Git State | 0 | Strict non-destructive QA methodology |
| **Tracked Test Files Modified**| 2 files (`test_alu.cpp`, `test_cpu_instructions.cpp`) | 5 files (+ `test_memory.cpp`, `test_stack_instructions.cpp`, `test_bus.cpp`) | +3 files | Added boundary and integration tests |
| **Core Unit Test Cases** | 254 test cases | 265 test cases | +11 test cases (+4.33%) | 4 Memory, 4 Stack, 3 Bus tests added |
| **Unit Test Pass Rate** | 254 / 254 (100%) | 265 / 265 (100%) | 0 regressions | 100% pass across all presets |
| **GUI QtTest Execution Slots** | 83 slots (81 methods + 2 slots) | 83 slots (81 methods + 2 slots) | 0 regressions | Verified in `gui` and `gui-release` |
| **CMake Preset Conformance** | 4 presets passing | 4 presets passing | 0 regressions | 265 core tests passed under `ci`, `debug`, `gui`, `gui-release`; 83 GUI tests passed under `gui` and `gui-release` |
| **Memory Boundary Coverage** | Basic 0x0000/0xFFFF | 7 boundaries + isolation + 4-way word wrapping + 65,536-byte non-zero reset test | Complete | Formal neighbor isolation evidenced |
| **Stack Boundary Coverage** | Recursive call overflow | Exact single-instruction loop to `0xF001`/`0xF000`/`0xF002` | Complete | ADR-001 rules 1–8 evidenced |
| **Virtual I/O Coverage** | Basic smoke tests | Exhaustive 256-port scan matrix | Complete | All 252 unmapped ports verified |

---

## 10. Auditor Attestation & Traceability

1. **Audit Authority & Independence:**
   This audit was conducted using an Antigravity-assisted software quality assurance process adhering strictly to the frozen specifications `docs/BitForge_Architecture_Specification_v1.7.md` and `docs/decisions/ADR-001-stack-pointer-convention.md`.
2. **Preservation of Repository Integrity and Cleanliness:**
   - No Git branches were switched, reset, or created.
   - No commits were staged, committed, or pushed.
   - Pre-existing untracked reports (`QA_PHASE0_BASELINE_REPORT.md`, `QA_PHASE1_SPEC_CONFORMANCE_REPORT.md`, `QA_PHASE2_CPU_EDGE_CASE_REPORT.md`) remain preserved.
   - `git diff --check` was executed on all tracked modified files, confirming zero Git whitespace errors.
   - A direct programmatic scan (PowerShell regex `\s+$`) was executed across all four QA markdown reports in `docs/`, confirming zero trailing whitespace characters across all report files.
3. **Evidence Basis:**
   All assertions in this document are derived from reproducible C++ test executions across all four CMake presets, supported by formal static source inspection and inductive specification proofs (e.g., the reachability analysis of $SP < 0xF000$).

---

## 11. Phase 4 Readiness & Gate Criteria

Phase 3 is officially **COMPLETE**. The Memory model, System Bus, and Stack Subsystem satisfy all architectural quality criteria. The repository is ready to proceed to **Phase 4: Assembler and Directives Verification**.

### Phase 4 Gate Criteria Verification

- [x] Memory subsystem public contract verified across all physical boundaries.
- [x] Modulo-65536 word access and boundary wrapping evidenced.
- [x] Segment loader bounds checking and atomicity verified.
- [x] ADR-001 Stack pointer rules 1 through 8 verified with deterministic boundary tests.
- [x] Unreachable state analysis of $SP < 0xF000$ completed and documented.
- [x] System bus delegation, virtual I/O port permissions, and device reset verified.
- [x] Exhaustive 256-port scan confirms all unmapped ports reject access without side effects.
- [x] Core test suite expanded to 265 unit tests, passing 100% across all 4 CMake presets.
- [x] GUI test suite passing 100% (83 test slots) in both `gui` and `gui-release` presets.
- [x] All frozen specifications and tracked production code intact and unmodified.
- [x] Untracked QA reports from preceding phases preserved.

---

## 12. Phase 3.1 Reconciliation Record

Phase 3.1 performed a narrow verification and reconciliation pass prior to Phase 4:

1. **Strengthened Memory Reset Test:** Replaced the previous 13-boundary non-zero setup in `MemoryTest.ResetComprehensiveStorageClear` with an exhaustive pre-population of all 65,536 addresses using deterministic non-zero pattern `(i % 255) + 1`. Verified that all addresses read `0x00` after reset.
2. **Stack-Range Terminology Precision:** Codified the distinction between the architectural reserved stack memory region (`0xF000..0xFFFF`, 4,096 bytes per v1.7 §5.2), valid stack-storage addresses (`0xF000..0xFFFE`, 4,095 bytes per ADR-001 §3), and the empty-stack sentinel (`SP = 0xFFFF` per ADR-001 §1).
3. **SystemBus Event Journal Lifecycle Audit:** Inspected `src/cli/runner.cpp` and `src/debugger/debugger_session.cpp`. Confirmed that headless CLI execution accumulates `MemoryWriteEvent` and `IoEvent` records without drainage, whereas debugger sessions drain them per step. Updated `RISK-P3-001` status to "Not yet measured; scheduled for Phase 5 CLI/stress audit."
4. **Invalid Register Encoding Scope Clarification:** Clarified that representative invalid register IDs ($R_4, R_5, R_9$) were tested for CPU instruction faulting and atomicity, while the register validation predicate `is_valid_reg()` was exhaustively evaluated across $[0, 255]$ in `TypesTest`.
5. **Attestation Scope Precision:** Updated auditor attestation to explicitly acknowledge the combination of runtime test execution and formal static/inductive analysis.
6. **Verification Across Presets:** Re-verified all four presets (`ci`, `debug`, `gui`, `gui-release`), confirming 265/265 unit tests and 83/83 GUI test slots passing with 0 failures.

---

## 13. Phase 3.2 Documentation Closeout Record

Phase 3.2 executed a final documentation-only closeout prior to Phase 4:

1. **Phase 2 Scope Clarification:** Aligned `docs/QA_PHASE2_CPU_EDGE_CASE_REPORT.md` to explicitly state that Phase 2's principal verification scope was the CPU, ALU, instruction boundaries, and fault behavior, distinguishing preliminary instruction-boundary memory checks from Phase 3's dedicated Memory/Bus/Stack audit.
2. **Test Preset Attribution Clarification:** Clarified across Sections 1, 6, and 9 that all 265 core unit tests passed under all four presets (`ci`, `debug`, `gui`, `gui-release`), while all 83 Qt GUI test entries (81 test methods + 2 fixture slots) passed under `gui` and `gui-release`.
3. **Scoping of Architectural Compliance Claims:** Scoped general compliance phrasing to the components and behaviors actually verified (64 KiB memory model, system bus delegation, virtual I/O, and ADR-001 stack discipline).
4. **Whitespace Attestation Precision:** Explicitly attributed whitespace verification to `git diff --check` for tracked files and direct programmatic regex scanning (`\s+$`) for untracked QA markdown reports.
5. **Preserved Invariants:** Preserved the documented three-way distinction between the reserved stack region, valid stack-storage addresses, and the empty-stack sentinel, and retained `RISK-P3-001` as unmeasured and scheduled for Phase 5.
