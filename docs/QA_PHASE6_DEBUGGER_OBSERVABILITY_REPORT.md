# BitForge Quality Assurance Audit — Phase 6: CPU Execution and Debugger Observability Audit

- **Document Version:** 1.3 (Phase 6.3 Final Sign-Off)
- **Audit Date:** October 2026
- **Auditor:** Quality Assurance & Verification Audit Team (Antigravity-assisted)
- **Repository:** `D:\BitForge`
- **Git HEAD Commit:** `5a88e11`
- **Primary Authorities:**
  - `docs/BitForge_Architecture_Specification_v1.7.md` (Frozen)
  - `docs/decisions/ADR-001-stack-pointer-convention.md` (Frozen)
  - `docs/PHASE11_TERMINOLOGY_GLOSSARY.md`
  - `docs/QA_PHASE0_BASELINE_REPORT.md`
  - `docs/QA_PHASE1_SPEC_CONFORMANCE_REPORT.md`
  - `docs/QA_PHASE2_CPU_EDGE_CASE_REPORT.md` (Version 1.3)
  - `docs/QA_PHASE3_MEMORY_BUS_STACK_REPORT.md` (Version 1.2)
  - `docs/QA_PHASE4_ASSEMBLER_DIRECTIVES_REPORT.md` (Version 1.3)
  - `docs/QA_PHASE5_CLI_HEADLESS_STRESS_REPORT.md` (Version 1.3)

---

## 1. Executive Summary & Audit Scope

Phase 6 of the BitForge Final QA Roadmap evaluates the interactive execution engine and debugger observability subsystems (`bitforge::debugger`). The audit covers breakpoint placement, lifecycle, and hit resolution, single-step execution, CPU machine state transitions, subroutine stepping, `ExecutionDelta` fidelity, bounded deque-backed trace history mechanics, memory/stack inspection safety, fault atomicity under interactive debugging, and GUI/debugger model integration.

All audit procedures are evidence-led, non-destructive, and strictly respect the frozen architecture specifications v1.0–v1.7 and ADR-001.

### Key Audit Findings

1. **Breakpoint Engine Resolution, Boundaries & Lifecycle:**
   - **Instruction Boundary vs. Operand Bytes:** Breakpoints check `current_pc` at the start of each instruction fetch. Setting a breakpoint on an operand byte (e.g. `0x0201` of a 3-byte `LDI`) is never hit during sequential instruction execution, as the fetch cursor advances directly across multi-byte instructions (`0x0200 -> 0x0203`). However, if an instruction branches or jumps directly into the operand byte address, the breakpoint fires predictably before fetch.
   - **Boundary Addresses (`0x0000`, `0xFFFF`, `0x0200`, `HALT`):** Address breakpoints operate cleanly across the complete 16-bit address space. Breakpoints at boundary addresses `0x0000` and `0xFFFF` register and retrieve in ascending order. Breakpoint at entry point (`0x0200`) stops immediately upon `run()` at step 0 (`steps_executed == 0`) with zero state mutation. Breakpoint at `HALT` stops *before* `HALT` executes (`state == Running`).
   - **Lifecycle & Persistence:** Breakpoints can be dynamically added, removed, or cleared (`clear_breakpoints()`). Breakpoint sets are preserved across session and GUI `reset()` calls by documented design. Breakpoint disabling/re-enabling without removal is unsupported by the BitForge API (`LIMIT-P6-002`).
   - **Branch and Subroutine Trapping:** Breakpoints function reliably across sequential execution, taken branches, subroutine calls (`CALL`), and return sites (`RET`). Untaken conditional branches do not trigger breakpoints located at branch targets.
2. **Machine State Transitions & Non-Continuation:**
   - Transitions between `Ready`, `Running`, `Paused`, `Halted`, and `Faulted` operate deterministically.
   - When the CPU is `Halted`, subsequent calls to `step()` or `run()` immediately return `StopReason::AlreadyHalted` with 0 steps executed.
   - When the CPU is `Faulted`, subsequent calls to `step()` or `run()` immediately return `StopReason::AlreadyFaulted` with 0 steps executed.
   - `reset()` reliably restores `state == Ready`, restores $PC$ to the entry point, initializes $SP = 0xFFFF$, clears all flags, clears the active fault record, clears `last_delta`, and purges the trace buffer while preserving active breakpoints.
3. **Stepping Controls Classification & Glossary Reconciliation:**
   - **`step()` (Single-Instruction Step):** Fully implemented and verified. Steps into subroutines on `CALL`, advances $PC$ to subroutine entry, and commits big-endian return address to stack memory per ADR-001.
   - **Glossary Reconciliation (`Step (F10)`):** `docs/PHASE11_TERMINOLOGY_GLOSSARY.md` Section 8 explicitly classifies "Step Into / Step Over" as a *Prohibited / Deprecated Term* in favor of the concise, approved UI term `Step (F10)`, reflecting BitForge's single-instruction execution model.
   - **Step Over and Step Out:** Confirmed **unimplemented in the BitForge core debugger API (`DebuggerSession`) and GUI (`ExecutionController`, `MainWindow`)**. Because the frozen architecture specifications v1.0–v1.7 neither mandate nor explicitly exclude them, they are classified as **unimplemented features / known limitations (`LIMIT-P6-001`)** rather than an architectural exclusion.
4. **Execution Delta Accuracy:**
   - Pre-step journal clearance (`clear_write_events()`) and post-step harvesting (`take_memory_write_events()`, `take_io_events()`) prevent cross-step event contamination.
   - Multi-mutation instructions accurately capture all side effects: `CALL` logs $SP$ changes, $PC$ changes, and 2 memory write events; `ADD` with signed overflow captures register mutation and flags ($N, V$); `OUT` and `IN` capture structured `IoEvent` records.
   - Pure non-mutating steps (e.g. `NOP`) report only $PC$ progression with empty flag, memory, and I/O change vectors.
5. **Bounded Deque-Backed Trace History:**
   - Backed by a bounded `std::deque<TraceEntry>` with configurable capacity (`kDefaultTraceCapacity = 100`) providing ring-buffer-like retention semantics.
   - Monotonically increasing `step_number` sequence is maintained across capacity overflows.
   - Chronological ordering (oldest to newest) is strictly preserved upon wraparound.
   - Dynamically adjusting capacity (`set_trace_capacity`) cleanly trims older records, and `clear_trace()` resets the buffer without affecting CPU execution.
6. **Stack & Memory Inspection Safety:**
   - `inspect_stack()` strictly conforms to ADR-001: reports empty stack when $SP = 0xFFFF$ (`depth = 0`), calculates depth as $0xFFFF - SP$, traverses occupied bytes down to $0xFFFE$, and flags corrupt/out-of-range stack pointers.
   - `read_memory_range()` wraps cleanly modulo 65,536 near the 64KiB boundary without memory overruns.
7. **Test Suite Expansion & Multi-Preset Pass Distribution:**
   - The core unit test suite expanded from 276 to **286 tests** (+10 comprehensive debugger audit fixtures in `tests/unit/test_debugger.cpp`, including Fixture 49 for breakpoint lifecycle and boundary verification).
   - GUI integration test suite expanded with prohibited continuation coverage across termination states in `tests/gui/test_gui.cpp`.
   - All 286 core unit tests and 83 Qt GUI test slots passed across all four CMake presets (`ci`, `debug`, `gui`, `gui-release`).

---

## 2. Initial Git State & Preserved Pre-Existing Work

The audit began from a verified starting state on branch `master` at HEAD commit `5a88e11b7833a119ed847733bcc4db2bc4cbbbfa`.

All pre-existing tracked modifications and untracked deliverables from earlier phases were strictly preserved:
- `src/cli/runner.cpp` (Phase 5 fix)
- `tests/unit/test_cli.cpp` (Phase 5 tests)
- `tests/unit/test_alu.cpp` (Phase 2 tests)
- `tests/unit/test_assembler.cpp` (Phase 4 tests)
- `tests/unit/test_bus.cpp` (Phase 3 tests)
- `tests/unit/test_cpu_instructions.cpp` (Phase 2 tests)
- `tests/unit/test_memory.cpp` (Phase 3 tests)
- `tests/unit/test_stack_instructions.cpp` (Phase 3 tests)
- `docs/QA_PHASE0_BASELINE_REPORT.md`
- `docs/QA_PHASE1_SPEC_CONFORMANCE_REPORT.md`
- `docs/QA_PHASE2_CPU_EDGE_CASE_REPORT.md` (Version 1.3)
- `docs/QA_PHASE3_MEMORY_BUS_STACK_REPORT.md` (Version 1.2)
- `docs/QA_PHASE4_ASSEMBLER_DIRECTIVES_REPORT.md` (Version 1.3)
- `docs/QA_PHASE5_CLI_HEADLESS_STRESS_REPORT.md` (Version 1.3)

---

## 3. Subsystem Architecture & Inspected Source Files

The audit examined the following implementation and test components:
- [`include/bitforge/debugger/debugger_session.hpp`](file:///D:/BitForge/include/bitforge/debugger/debugger_session.hpp): Core session coordinator interface.
- [`src/debugger/debugger_session.cpp`](file:///D:/BitForge/src/debugger/debugger_session.cpp): Stepping engine, breakpoint dispatch, trace buffer management, and memory/stack inspection.
- [`include/bitforge/debugger/execution_delta.hpp`](file:///D:/BitForge/include/bitforge/debugger/execution_delta.hpp): State delta computation and change serialization.
- [`include/bitforge/debugger/execution_result.hpp`](file:///D:/BitForge/include/bitforge/debugger/execution_result.hpp): Stop reason enumerations and execution outcome records.
- [`include/bitforge/debugger/breakpoint.hpp`](file:///D:/BitForge/include/bitforge/debugger/breakpoint.hpp): Breakpoint structure and operation results.
- [`include/bitforge/debugger/trace.hpp`](file:///D:/BitForge/include/bitforge/debugger/trace.hpp): Trace entry data definitions.
- [`include/bitforge/debugger/inspection.hpp`](file:///D:/BitForge/include/bitforge/debugger/inspection.hpp): Stack frame and memory inspection data structures.
- [`src/gui/execution_controller.cpp`](file:///D:/BitForge/src/gui/execution_controller.cpp): GUI execution worker, timed slices, and debugger integration.
- [`tests/unit/test_debugger.cpp`](file:///D:/BitForge/tests/unit/test_debugger.cpp): Core unit test suite for the debugger engine.
- [`tests/gui/test_gui.cpp`](file:///D:/BitForge/tests/gui/test_gui.cpp): Qt GUI integration test suite.

---

## 4. Breakpoint Engine Auditing & Boundary Verification

### 4.1 Instruction Start vs. Operand Byte Resolution

In BitForge, instructions vary in length from 1 to 4 bytes. `DebuggerSession::run_internal` checks active breakpoints against `current_pc` immediately prior to fetching the instruction at that address:
```cpp
const word_t current_pc = cpu_.registers().pc;
if (has_breakpoint(current_pc)) {
    if (bypass_initial_breakpoint) {
        bypass_initial_breakpoint = false;
    } else {
        last_breakpoint_hit_ = current_pc;
        last_stop_reason_ = StopReason::BreakpointHit;
        return ExecutionResult{StopReason::BreakpointHit, cpu_.state(), steps_taken, current_pc, current_pc, std::nullopt};
    }
}
```

#### Empirical Test Cases (`Audit_BreakpointOnOperandByteNotHitSequentially`):
1. **Sequential Stepping Over Operand Byte:**
   - Assembly: `LDI R0, 42` (3 bytes: `0x0200` opcode, `0x0201` register index, `0x0202` immediate byte) followed by `HALT` (`0x0203`).
   - Breakpoint set at operand byte `0x0201`.
   - Result: `run()` executes from `0x0200` to completion (`Halted` at `0x0204`). The breakpoint at `0x0201` is **never hit** because the CPU fetch cursor advances directly from `0x0200` to `0x0203`.
2. **Direct Jump Into Operand Byte:**
   - Assembly: `JMP 0x0205` jumping into the immediate byte of an `LDI` at `0x0203`.
   - Breakpoint set at `0x0205`.
   - Result: `run()` executes `JMP`, updates $PC = 0x0205$, and halts with `StopReason::BreakpointHit` before fetching from `0x0205`.

### 4.2 Breakpoints at Entry Point and HALT

1. **Initial Entry Point (`Audit_BreakpointAtInitialEntryPointStopsAtStepZero`):**
   - Setting a breakpoint on the reset entry address (`0x0200`) and calling `run()` results in an immediate stop with `steps_executed == 0`, `pc == 0x0200`, and all registers unaltered. Calling `continue_execution()` engages the one-time bypass and proceeds normally.
2. **HALT Instruction (`Audit_BreakpointOnHaltStopsBeforeHalt`):**
   - Setting a breakpoint on `HALT` stops with `StopReason::BreakpointHit` while `cpu_.state()` is still `MachineState::Running`. Calling `continue_execution()` executes the `HALT`, transitioning the state to `MachineState::Halted`.

### 4.3 Control Flow Trapping (Calls, Returns, Branches)

1. **Subroutine Calls & Returns (`Audit_BreakpointAcrossSubroutineCallsAndReturns`):**
   - Breakpoints set at subroutine entry (`0x0204`) and caller return site (`0x0203`).
   - First `run()` stops at `0x0204` after executing `CALL` ($SP$ decremented to `0xFFFD`).
   - `continue_execution()` executes subroutine, executes `RET` ($SP$ restored to `0xFFFF`), and stops at return site `0x0203`.
2. **Conditional Branches (`Audit_BreakpointConditionalBranchTakenVsUntaken`):**
   - `JZ taken_target` (with $Z = 1$) jumps to target; breakpoint at `taken_target` is hit.
   - `JZ untaken_target` (with $Z = 0$) falls through; breakpoint at `untaken_target` is **not hit**, executing sequential code cleanly.

### 4.4 Breakpoint Lifecycle, Boundary Addresses & Persistence (`Audit_BreakpointLifecycleBoundariesAndResetPreservation`)

To resolve all remaining lifecycle and address-boundary verification items, Fixture 49 was implemented and verified:
1. **Valid Boundary Addresses (`0x0000` and `0xFFFF`):**
   - Address breakpoints accept `word_t` (`uint16_t`). Breakpoints set at the lowest 16-bit address (`0x0000`) and the highest address (`0xFFFF`) succeed (`success == true`, `duplicate == false`).
   - Querying `breakpoints()` returns addresses sorted in ascending numerical order (`0x0000` followed by `0xFFFF`).
2. **Invalid Address Handling:**
   - For address breakpoints, the parameter type is `word_t`, so all representable values (`0x0000..0xFFFF`) map to valid locations in the 64KiB address space.
   - For source-line breakpoints, invalid lines (unmappable directives, comments, blank lines, labels only, data declarations, and out-of-range line indices) fail with explanatory messages (`Debugger.UnmappableSourceLines`).
   - If a debugger session is initialized without an attached `ProgramImage`, `add_line_breakpoint()` and `remove_line_breakpoint()` fail predictably with `"No program image attached to debugger session."`.
3. **Duplicate Additions and Non-Existent Removals:**
   - Attempting to re-add an already active breakpoint returns `BreakpointResult{success: true, duplicate: true}` without duplicating entries.
   - Attempting to remove an inactive address returns `BreakpointResult{success: false, not_found: true}`.
4. **Breakpoint State Across Reset & Session Recreation:**
   - Calling `DebuggerSession::reset()` reinitializes the CPU registers, flags, fault records, and trace buffer, but **preserves active breakpoints** by documented design. Both unit tests and GUI tests (`GuiTest::testResetDuringExecution`) verify that active breakpoints survive reset.
   - Recreation of a debugger session initializes an empty breakpoint set (`breakpoints().empty() == true`). Similarly, loading a new file in GUI purges prior breakpoints (`GuiTest::testFileSwitchingPurgesAllState`).
5. **Clearing Breakpoints (`clear_breakpoints()`):**
   - Calling `clear_breakpoints()` immediately clears all active breakpoints and resets `last_breakpoint_hit_`. Tested directly in unit test `Debugger.Audit_BreakpointLifecycleBoundariesAndResetPreservation` (Fixture 49) and in GUI test `GuiTest::testADR001StackViewerEmptySentinelAndBreakpointsEmptyState` (which tests `controller()->clearAllBreakpoints()` and breakpoint panel UI refresh). Note: `GuiTest::testWatchPanelInspectionAndHighlighting` exercises watch expressions (`wp->clearWatches()`), not breakpoints.
6. **Disabling and Re-enabling Breakpoints (Unsupported Capability):**
   - `DebuggerSession` stores breakpoints as an active address set (`std::set<word_t> breakpoints_`).
   - Neither `DebuggerSession`, `ExecutionController`, nor the GUI interface support a dormant "disabled" state; breakpoints are either registered or removed.
   - Formally classified as a **currently unsupported debugger API / implementation limitation (`LIMIT-P6-002`)**.

---

## 5. Stepping Semantics & Unimplemented Controls

### 5.1 Supported Stepping Operations
- **`step()` (Single Instruction Step / Step Into):**
  - Executes at most one complete instruction from current $PC$ per the execution model (§3, §7.3).
  - Bypasses any breakpoint at the current $PC$ for that single step.
  - Updates bounded deque-backed trace history and last execution delta.
  - Steps into subroutines on `CALL`, updating $PC$ to subroutine entry and pushing return address.

### 5.2 Step Over and Step Out Feature Classification & Glossary Reconciliation
An exhaustive inspection of the frozen architecture specifications (v1.0–v1.7), ADR-001, design glossaries, and the codebase confirms:
1. **F10 Terminology Glossary Disposition:**
   - `docs/PHASE11_TERMINOLOGY_GLOSSARY.md` Section 8 ("UI Terminology Standards & Prohibitions") defines UI microcopy rules using three explicit columns:
     - **Approved Term:** `**Step (F10)**`
     - **Prohibited / Deprecated Term:** `Step Into / Step Over`
     - **Rationale:** `BitForge operates at single-instruction granularity; "Step" is concise and exact.`
   - This entry does **not** define `Step (F10)` as performing a Step Over or Step Into dual mode. Rather, it explicitly designates the compound label "Step Into / Step Over" as a **prohibited / deprecated term** for user interface buttons and menus, establishing that the UI command label must simply read `Step (F10)` because the platform operates strictly at single-instruction granularity.
2. **Specification Scope:** The frozen architecture specification mandates single-stepping at instruction granularity (§4, §6: "Instruction execution model: One complete instruction per step").
3. **Implementation Scope:** `DebuggerSession` exposes only `step()`, `run()`, and `continue_execution()`. GUI `ExecutionController` and `MainWindow` provide Step (F10), Run (F5), Continue (F6/F8), Pause, and Reset. Neither `step_over()` nor `step_out()` are declared or implemented in the engine or GUI. Single-instruction execution naturally steps into subroutines (`CALL`).
4. **Formal Classification:** The frozen architecture specifications v1.0–v1.7 neither mandate Step Over/Step Out nor explicitly exclude them. Consequently, they are formally classified as **unimplemented features / known limitations (`LIMIT-P6-001`)**, rather than an intentional architectural exclusion. Existing Step Into (`step()`) behavior and all associated regression suites remain fully preserved and functional.

---

## 6. Execution Delta Fidelity & Step Isolation

### 6.1 Multi-Mutation Delta Verification (`Audit_ExecutionDeltaMultiMutationAndIsolation`)

`compute_execution_delta()` compares pre-execution snapshots against post-execution state:
- **`ADD R0, R1` (with signed overflow):**
  - Register changes: captures `R0: 127 -> 128 (0x80)`.
  - Flag changes: captures `N: 0 -> 1` and `V: 0 -> 1`.
- **`CALL addr16`:**
  - Register changes: captures $SP$ decrement (`0xFFFF -> 0xFFFD`) and $PC$ jump.
  - Memory writes: captures 2 `MemoryWriteEvent` records at `0xFFFD` (high byte) and `0xFFFE` (low byte).
- **`RET`:**
  - Register changes: captures $SP$ increment (`0xFFFD -> 0xFFFF`) and $PC$ return.
  - Memory writes: strictly empty (0 writes).
- **`OUT port, Rs`:**
  - Captures `IoEvent{IoDirection::Write, port, val}`.
- **`IN Rd, port`:**
  - Captures `IoEvent{IoDirection::Read, port, val}` and register mutation for `Rd`.

### 6.2 Step Isolation
Immediately before `cpu_.step()`, `DebuggerSession::execute_single_step` calls `sys_bus->clear_write_events()`. Testing confirmed that executing a subsequent `NOP` produces a delta containing only the $PC$ change, with zero residual memory writes, I/O events, or flag mutations from previous instructions.

---

## 7. Bounded Deque-Backed Trace History & Retention Semantics

### 7.1 Deque-Backed Trace History Implementation
Trace history is managed by a bounded `std::deque<TraceEntry> trace_buffer_` in `src/debugger/debugger_session.cpp` exhibiting ring-buffer-like retention semantics:
- Configured capacity defaults to `kDefaultTraceCapacity = 100`.
- When `trace_buffer_.size() >= trace_capacity_`, the oldest entry is evicted from the front via `trace_buffer_.pop_front()`, maintaining bounded memory.
- Appends new steps to the back via `trace_buffer_.push_back(std::move(entry))`.
- Returned via `DebuggerSession::trace()` as a `const std::vector<TraceEntry>&` synchronized via cached vector copy when marked dirty.

### 7.2 Wraparound & Chronology Verification (`Audit_TraceHistoryCapacityWraparoundChronology`)
Tested under capacity limit 5 over a 22-step execution loop with bounded deque retention:
1. `trace().size()` remained exactly 5 after 22 steps.
2. `step_number` sequence was strictly monotonic: `18, 19, 20, 21, 22`.
3. Evicted steps (`1..17`) were cleanly removed without memory leaks.
4. Reducing capacity dynamically to 3 trimmed the deque to `20, 21, 22`.
5. `clear_trace()` purged all entries immediately.

---

## 8. Machine State Transitions & Fault Atomicity

### 8.1 State Transition Matrix

| Initial State | Operation | Trigger Condition | Resulting State | Stop Reason | Steps Recorded |
| :---: | :---: | :--- | :---: | :---: | :---: |
| **`Ready`** | `step()` | Valid instruction | `Running` | `StepCompleted` | 1 |
| **`Running`** | `run()` | Encounter active breakpoint | `Running` | `BreakpointHit` | $N$ |
| **`Running`** | `run()` | Budget limit reached | `Running` | `BudgetExhausted` | `max_steps` |
| **`Running`** | `step()` | Execute `HALT` | **`Halted`** | `Halted` | 1 |
| **`Running`** | `step()` | Invalid opcode / stack violation | **`Faulted`** | `Faulted` | 1 |
| **`Halted`** | `step()` / `run()` | Attempt execution while halted | **`Halted`** | `AlreadyHalted` | 0 |
| **`Faulted`** | `step()` / `run()` | Attempt execution while faulted | **`Faulted`** | `AlreadyFaulted` | 0 |
| **`Faulted`** | `reset()` | User reset request | **`Ready`** | `None` | 0 |

### 8.2 Architectural Fault Atomicity
Under interactive debugging, faults preserve full atomicity:
- **`InvalidOpcode` (0xFF):** $PC$ remains pointing to faulting instruction; registers and memory remain unmutated; delta has 0 writes.
- **`StackUnderflow` (`POP R0` at reset):** $SP$ remains `0xFFFF`; $R_0$ unmutated; delta has 0 writes.
- **`StackOverflow` (`PUSH R0` at $SP = 0xF000$):** $SP$ remains `0xF000`; stack memory unmutated.
- **`InvalidPort` (`OUT 0x00, R0` / `IN R0, 0x01`):** I/O write rejected; delta has 0 writes.

---

## 9. GUI and Debugger Model Integration & Coverage Matrix

### 9.1 Subsystem Architecture & Responsibilities
The GUI execution model links Qt's event loop to `bitforge::debugger::DebuggerSession` via `ExecutionController` (`src/gui/execution_controller.cpp`):
- **Single-Stepping (`step()`):** Executes synchronously on the main thread via `session_->step()`, immediately emitting `stateChanged()` and updating UI displays.
- **Continuous Execution (`run()` / `continueExecution()`):** Uses a recurring `QTimer slice_timer_` batching instructions into time slices based on selected speed mode (`Slow`, `Normal`, `Fast`, `Turbo`), yielding periodically to prevent UI starvation.
- **Bypass Semantics:** Resuming from a breakpoint applies a one-time bypass (`is_continue_mode_ = true`) for the first instruction only.

### 9.2 Requirement-to-Test Coverage Matrix

The following matrix maps each Phase 6 audit requirement to its verifying tests in `tests/unit/test_debugger.cpp` and `tests/gui/test_gui.cpp`, along with empirical verification outcomes:

| Phase 6 Audit Requirement | Unit Test Coverage (`test_debugger.cpp`) | GUI Test Coverage (`test_gui.cpp`) | Empirical Status |
| :--- | :--- | :--- | :---: |
| **1. Pause and Resume Transitions** | - `Audit_StateTransitionsAndFaultNonContinuation`<br>- `SingleStepAfterHaltedAndFaulted` | - `testPauseAndResumeIntegrity`<br>- `testMultiSliceExecutionAcrossBoundaries` | **PASS — listed tests** |
| **2. Single-Step and Continue Without Duplication** | - `SingleStepFromReady`<br>- `SequentialSingleStepAndPcAdvancement`<br>- `ContinueFromBreakpointWithoutTrapping`<br>- `Audit_ExecutionDeltaMultiMutationAndIsolation` | - `testSingleStepRefreshesState`<br>- `testContinueResumesPastBreakpoint`<br>- `testRunFromEntryVsContinueSemantics`<br>- `testRealExample02IoDemoAndNoRunDuplication` | **PASS — listed tests** |
| **3. Breakpoint Placement, Lifecycle & Boundaries** | - `AddressBreakpointStopsBeforeExecution`<br>- `ContinueFromBreakpointWithoutTrapping`<br>- `Audit_BreakpointOnOperandByteNotHitSequentially`<br>- `Audit_BreakpointAtInitialEntryPointStopsAtStepZero`<br>- `Audit_BreakpointAcrossSubroutineCallsAndReturns`<br>- `Audit_BreakpointConditionalBranchTakenVsUntaken`<br>- `Audit_BreakpointOnHaltStopsBeforeHalt`<br>- `Audit_BreakpointLifecycleBoundariesAndResetPreservation` *(Fixture 49)* | - `testBreakpointStopsBeforeExecution`<br>- `testContinueResumesPastBreakpoint`<br>- `testBreakpointPlacementRestrictions`<br>- `testBreakpointErgonomicsAndGutterF9`<br>- `testBreakpointAtSliceBoundaryAndContinue`<br>- `testADR001StackViewerEmptySentinelAndBreakpointsEmptyState` | **PASS — listed tests** |
| **4. Reset and Reload (State & Trace Purging)** | - `Audit_TraceCapacityBoundariesAndDraining`<br>- `Audit_StateTransitionsAndFaultNonContinuation`<br>- `Audit_BreakpointLifecycleBoundariesAndResetPreservation` *(Fixture 49)* | - `testResetDuringExecution`<br>- `testFreshRunMemoryResetAndIoContract`<br>- `testFileSwitchingPurgesAllState`<br>- `testEditingSourceMarksProgramStale`<br>- `testStaleImageCannotBeExecuted` | **PASS — listed tests** |
| **5. Register, PC, Memory & I/O Presentation** | - `RegisterAndFlagInspection`<br>- `ReadOnlyMemoryInspection`<br>- `StackInspection`<br>- `Audit_StackInspectionBoundaries`<br>- `Audit_MemoryInspectionModulo65536Wrapping`<br>- `Audit_ExecutionDeltaMultiMutationAndIsolation` | - `testSingleStepRefreshesState`<br>- `testVirtualDevicesExecution`<br>- `testMemoryAndStackViewersRefresh`<br>- `testADR001StackViewerEmptySentinelAndBreakpointsEmptyState`<br>- `testWatchPanelInspectionAndHighlighting`<br>- `testBitViewWidgetVisualization`<br>- `testInstructionInspectorDecoding` | **PASS — listed tests** |
| **6. Halt/Fault Distinction & Prohibited Continuation** | - `SingleStepOfHalt`<br>- `SingleStepAfterHaltedAndFaulted`<br>- `CpuFaultReportingAndFaultingAddress`<br>- `Audit_BreakpointOnHaltStopsBeforeHalt`<br>- `Audit_StateTransitionsAndFaultNonContinuation`<br>- `FaultAtomicity_*` (Fixtures 35–39) | - `testHaltAndFaultVisualDistinction`<br>- `testFreshRunMemoryResetAndIoContract`<br>- `testBudgetExhaustionVsHaltAtBoundary`<br>- `testSingleStepRefreshesState` *(Strengthened)* | **PASS — listed tests** |

### 9.3 Phase 6.1 GUI Regression Test Additions
To guarantee that GUI controllers strictly reject execution after CPU termination, two existing GUI test fixtures were strengthened with explicit post-termination step assertions:
1. **`GuiTest::testSingleStepRefreshesState`:**
   - After stepping through `HALT` into `MachineState::Halted`, a subsequent call to `window.onStep()` was asserted.
   - Verified that `lastStopReason()` returns `StopReason::AlreadyHalted`, `state()` remains `MachineState::Halted`, $PC$ remains `0x0207`, and `total_steps_executed()` does not increase.
2. **`GuiTest::testHaltAndFaultVisualDistinction`:**
   - After executing an invalid opcode into `MachineState::Faulted`, a subsequent call to `window.onStep()` was asserted.
   - Verified that `lastStopReason()` returns `StopReason::AlreadyFaulted`, `state()` remains `MachineState::Faulted`, and `total_steps_executed()` does not increase.

### 9.4 Phase 6.2 Core Unit Test Additions (`Audit_BreakpointLifecycleBoundariesAndResetPreservation`)
To systematically validate all breakpoint lifecycle states, edge boundaries, and persistence rules, Fixture 49 was added to `tests/unit/test_debugger.cpp`:
1. Valid boundary address addition: `0x0000` and `0xFFFF` added successfully.
2. Sorted retrieval: `breakpoints()` verified to return `{0x0000, 0xFFFF}` in ascending numerical order.
3. Duplicate add handling: duplicate at boundary `0x0000` returns `duplicate == true`, `success == true`, without double-counting.
4. Non-existent removal: removing unmapped address `0x0001` returns `not_found == true`, `success == false`.
5. Boundary removal: removing `0x0000` leaves `0xFFFF` active.
6. Persistence across reset: adding `0x0200` and calling `DebuggerSession::reset()` confirms breakpoints remain active across reset.
7. Clearing: `clear_breakpoints()` purges all breakpoints and leaves the set empty.
8. Unattached image handling: calling `add_line_breakpoint()` or `remove_line_breakpoint()` without an attached image safely returns `success == false` with diagnostic message.

---

## 10. Multi-Preset Regression Test Matrix

Following the addition of the 10 new Phase 6 unit test fixtures and Phase 6.1 GUI prohibited continuation assertions, the complete test suite was executed across all four CMake build presets:

| CMake Build Preset | Build Toolchain | Unit Tests Executed | Unit Tests Passed | GUI Tests Executed | GUI Tests Passed | Total Pass Rate | Real Test Time |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **`ci`** | GCC 14.2.0 (MSYS2 UCRT64, Ninja) | **286** | **286** | N/A (Core Preset) | N/A | **100%** | 0.31s |
| **`debug`** | GCC 14.2.0 (MSYS2 UCRT64, Ninja) | **286** | **286** | N/A (Debug Preset) | N/A | **100%** | 0.31s |
| **`gui`** | GCC 14.2.0 + Qt 6.8.0 (Ninja) | **286** | **286** | **83** | **83** | **100%** | 12.93s |
| **`gui-release`**| GCC 14.2.0 + Qt 6.8.0 (Ninja, `-O3`) | **286** | **286** | **83** | **83** | **100%** | 12.30s |

### Complete Inventory of Debugger Tests (`tests/unit/test_debugger.cpp` — 49 Fixtures)
1. `SingleStepFromReady`
2. `SequentialSingleStepAndPcAdvancement`
3. `SingleStepOfHalt`
4. `SingleStepAfterHaltedAndFaulted`
5. `AddressBreakpointStopsBeforeExecution`
6. `ContinueFromBreakpointWithoutTrapping`
7. `SingleStepAtAddressWithBreakpoint`
8. `BreakpointsAfterBranchesAndJumps`
9. `RunToHalt`
10. `BudgetExhaustionInfiniteLoop`
11. `HaltOnFinalPermittedStep`
12. `CpuFaultReportingAndFaultingAddress`
13. `SourceLineBreakpointResolution`
14. `SourceMappingsAcrossMultipleOrgSegments`
15. `UnmappableSourceLines`
16. `RegisterAndFlagInspection`
17. `ReadOnlyMemoryInspection`
18. `StackInspection`
19. `BoundedTraceRetentionAndOrdering`
20. `MemoryRangeNear64KBoundary`
21. `DuplicateBreakpointsAndRemoval`
22. `ZeroBudgetRun`
23. `Audit_RunZeroBudgetDoesNotMutateState`
24. `Audit_HaltOnFinalPermittedStepVersusNonHalting`
25. `Audit_BreakpointSelfLoopStopsOnNextEncounter`
26. `Audit_BreakpointConditionalBranchLoop`
27. `Audit_BreakpointRemovalClearsHitState`
28. `Audit_SourceLineWithLabelAndInstructionOnSameLine`
29. `Audit_SourceLineResolutionNonContiguousOutOfOrderSegments`
30. `Audit_TraceCapacityBoundariesAndDraining`
31. `Audit_TraceFaultAtomicityAndSourceMapping`
32. `Audit_StackInspectionBoundaries`
33. `Audit_MemoryInspectionModulo65536Wrapping`
34. `ExecutionDeltaCapture`
35. `FaultAtomicity_InvalidOpcode`
36. `FaultAtomicity_StackUnderflow`
37. `FaultAtomicity_StackOverflow_Push`
38. `FaultAtomicity_StackOverflow_Call`
39. `FaultAtomicity_InvalidPort`
40. `Audit_BreakpointOnOperandByteNotHitSequentially` *(New Phase 6)*
41. `Audit_BreakpointAtInitialEntryPointStopsAtStepZero` *(New Phase 6)*
42. `Audit_BreakpointAcrossSubroutineCallsAndReturns` *(New Phase 6)*
43. `Audit_BreakpointConditionalBranchTakenVsUntaken` *(New Phase 6)*
44. `Audit_BreakpointOnHaltStopsBeforeHalt` *(New Phase 6)*
45. `Audit_NestedSubroutineExecutionAndStepping` *(New Phase 6)*
46. `Audit_ExecutionDeltaMultiMutationAndIsolation` *(New Phase 6)*
47. `Audit_TraceHistoryCapacityWraparoundChronology` *(New Phase 6)*
48. `Audit_StateTransitionsAndFaultNonContinuation` *(New Phase 6)*
49. `Audit_BreakpointLifecycleBoundariesAndResetPreservation` *(New Phase 6.2)*

---

## 11. Residual Risks & Technical Debt Assessment

| Risk / Gap ID | Component | Description | Severity | Mitigation / Resolution Status |
| :--- | :--- | :--- | :--- | :--- |
| **`LIMIT-P6-001`** | Stepping Controls | `step_over` and `step_out` are not implemented in the debugger engine or UI. | Low | **Unimplemented Feature / Known Limitation:** The platform operates at single-instruction granularity (`step()` / Step Into). `docs/PHASE11_TERMINOLOGY_GLOSSARY.md` Section 8 explicitly designates "Step Into / Step Over" as a prohibited/deprecated UI term in favor of the approved concise label `Step (F10)`. The frozen specifications neither require nor exclude Step Over/Step Out; they remain unimplemented features. |
| **`LIMIT-P6-002`** | Breakpoint Controls | Disabling and re-enabling individual breakpoints without removing them is unsupported by the BitForge API. | Low | **Currently unsupported debugger API / implementation limitation (`LIMIT-P6-002`):** `DebuggerSession` maintains breakpoints as an active address set (`std::set<word_t>`). Toggling an address adds or erases it. No dormant/disabled state bit exists in the current implementation. |
| **`GAP-P4-001`** | Assembler Test Suite | Diagnostic column coordinates across 31 malformed snippets in `AssemblerTest.MalformedInputComprehensiveMatrix` are asserted for failure and non-empty diagnostics, but not granular column numbers. | Low | **Documented Gap:** Representative line and column precision is asserted in `DiagnosticSourceLocationsPrecision`. |
| **`RISK-P5-001`** | Virtual I/O | Console output buffers (`text_output_`, `numeric_output_`) grow monotonically if a long-running program emits millions of characters. | Low | **Documented Architectural Behavior:** Per v1.7 §7, console output buffers capture cumulative console text and are presented upon execution completion. Not a leak. |

---

## 12. Complete Changed-File Inventory & Git Status

### Tracked Modifications:
1. [`src/cli/runner.cpp`](file:///D:/BitForge/src/cli/runner.cpp): Minimal 1-line event journal drainage in `execute_run` (Phase 5).
2. [`tests/gui/test_gui.cpp`](file:///D:/BitForge/tests/gui/test_gui.cpp): Strengthened post-termination execution rejection assertions in `testSingleStepRefreshesState` and `testHaltAndFaultVisualDistinction` (Phase 6.1).
3. [`tests/unit/test_cli.cpp`](file:///D:/BitForge/tests/unit/test_cli.cpp): 4 new fixtures + direct event drainage assertion (Phase 5).
4. [`tests/unit/test_debugger.cpp`](file:///D:/BitForge/tests/unit/test_debugger.cpp): 10 new Phase 6 audit fixtures (total 49 debugger tests, 286 core tests).
5. Pre-existing test modifications from Phases 2–4 (`test_alu.cpp`, `test_assembler.cpp`, `test_bus.cpp`, `test_cpu_instructions.cpp`, `test_memory.cpp`, `test_stack_instructions.cpp`).

### Untracked Reports:
- `docs/QA_PHASE0_BASELINE_REPORT.md`
- `docs/QA_PHASE1_SPEC_CONFORMANCE_REPORT.md`
- `docs/QA_PHASE2_CPU_EDGE_CASE_REPORT.md` (v1.3)
- `docs/QA_PHASE3_MEMORY_BUS_STACK_REPORT.md` (v1.2)
- `docs/QA_PHASE4_ASSEMBLER_DIRECTIVES_REPORT.md` (v1.3)
- `docs/QA_PHASE5_CLI_HEADLESS_STRESS_REPORT.md` (v1.3)
- `docs/QA_PHASE6_DEBUGGER_OBSERVABILITY_REPORT.md` (v1.3)

### Current Git Status (`git status --short --branch`):
```text
## master...origin/master [ahead 1]
 M src/cli/runner.cpp
 M tests/gui/test_gui.cpp
 M tests/unit/test_alu.cpp
 M tests/unit/test_assembler.cpp
 M tests/unit/test_bus.cpp
 M tests/unit/test_cli.cpp
 M tests/unit/test_cpu_instructions.cpp
 M tests/unit/test_debugger.cpp
 M tests/unit/test_memory.cpp
 M tests/unit/test_stack_instructions.cpp
?? docs/QA_PHASE0_BASELINE_REPORT.md
?? docs/QA_PHASE1_SPEC_CONFORMANCE_REPORT.md
?? docs/QA_PHASE2_CPU_EDGE_CASE_REPORT.md
?? docs/QA_PHASE3_MEMORY_BUS_STACK_REPORT.md
?? docs/QA_PHASE4_ASSEMBLER_DIRECTIVES_REPORT.md
?? docs/QA_PHASE5_CLI_HEADLESS_STRESS_REPORT.md
?? docs/QA_PHASE6_DEBUGGER_OBSERVABILITY_REPORT.md
```

---

## 13. Phase 6 Sign-Off Readiness Conclusion

Based on source inspection, empirical test execution, requirement-to-test mapping, and multi-preset verification:
1. **Supported Behaviors Directly Tested:**
   - Single-stepping (`step()`), execution budgeting (`run()`), and breakpoint bypass resumption (`continue_execution()`) faithfully mirror CPU execution and machine state transitions.
   - Prohibited continuation when halted (`AlreadyHalted`) or faulted (`AlreadyFaulted`) is strictly enforced across both CLI and GUI controller interfaces.
   - Breakpoint placement, boundary addresses (`0x0000`, `0xFFFF`, entry point `0x0200`, `HALT`), duplicate addition, non-existent removal, clearing (`clear_breakpoints()`), and session reset persistence passed their listed test suites.
   - Execution deltas reliably isolate and capture multi-mutation operations without cross-step leakage.
   - Bounded deque-backed trace history maintains strict monotonicity and chronological ordering across capacity overflows.
2. **Known Unimplemented Features:**
   - Step Over (`step_over`) and Step Out (`step_out`) remain unimplemented in the debugger engine and UI (`LIMIT-P6-001`). The platform operates at single-instruction granularity per frozen architecture specification (§4, §6) and `docs/PHASE11_TERMINOLOGY_GLOSSARY.md`.
3. **Currently Unsupported API Operations:**
   - Disabling and re-enabling individual breakpoints without removing them is unsupported by the BitForge API and data structures (`std::set<word_t>`) (`LIMIT-P6-002`).
4. **Regression Stability:**
   - All 286 core unit tests and 83 Qt GUI tests pass across all four CMake build presets (`ci`, `debug`, `gui`, `gui-release`).

**Phase 6 is hereby certified COMPLETE and formally SIGNED OFF.**
