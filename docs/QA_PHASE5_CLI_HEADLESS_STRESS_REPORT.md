# BitForge Quality Assurance Audit — Phase 5: CLI, Virtual I/O, Headless Execution, and Event-Journal Stress Audit

- **Document Version:** 1.3 (Phase 5.3 Final Reconciliation Closeout)
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
  - `docs/QA_PHASE4_ASSEMBLER_DIRECTIVES_REPORT.md` (Version 1.3)

---

## 1. Executive Summary & Audit Scope

Phase 5 of the BitForge Final QA Roadmap evaluates the command-line interface (`bitforge::cli`), headless CPU execution loop, instruction execution-budget enforcement, process exit-code conformance, headless virtual peripheral I/O contracts, and the lifecycle and memory growth of `SystemBus` event journals under prolonged execution workloads (`RISK-P3-001`).

All audit procedures are test-led, evidence-backed, and non-destructive. No frozen architecture specifications were modified. A targeted, minimal production fix was applied to `src/cli/runner.cpp` to drain unconsumed debugger-only write and I/O event journals during headless execution loops, eliminating the unbounded memory growth identified in `RISK-P3-001` while strictly preserving virtual peripheral buffers, register dumps, execution counts, and debugger/GUI delta observability.

### Key Audit Findings

1. **CLI Subcommand & Argument Parsing Conformance:** All supported commands (`help`, `version`, `assemble`, `run`) and option flags (`--max-steps`, `--dump-regs`) parse deterministically. Positional syntax for `assemble` is strictly `assemble <source.asm>` (the CLI runner does not implement an `-o` flag; passing extraneous arguments triggers usage error Exit Code `1`). Missing positional arguments, unexpected extra arguments, unknown subcommands, unknown options, non-numeric budgets, zero budgets, negative budgets, and numeric overflows are strictly rejected with informative error diagnostics.
2. **Process Exit-Code Contract Conformance (Codes 0..4):** The documented exit-code contract was empirically verified against real process execution of `bitforge.exe`:
   - `0` (Success): Verified for `--help`, `--version`, valid file assembly, and normal program halt.
   - `1` (Usage Error): Verified for 12 distinct CLI argument errors, missing options, and malformed budgets.
   - `2` (File / Assembly Error): Verified for non-existent files and assembly syntax/boundary errors.
   - `3` (CPU Fault): Verified for invalid opcodes, stack underflow at reset (`POP R0` when `SP = 0xFFFF`), stack overflow, and invalid port accesses.
   - `4` (Budget Exceeded): Verified for infinite loops and step budgets expiring before `HALT`.
3. **Execution Budget & Boundary Step Invariants:** Boundary testing on a finite 5-instruction program confirmed that:
   - Budget 4 yields Exit Code `4` (`InstructionBudgetExceeded`).
   - Budget 5 yields Exit Code `0` (`HALT` wins on the exact step boundary).
   - Budget 6 yields Exit Code `0` (`HALT` wins cleanly with surplus budget).
4. **Register Dump Correctness (`--dump-regs`):** Register dumping was verified against known architectural states, confirming accurate hexadecimal formatting for all registers ($R_0..R_3$, $IX$, $PC$, $SP$), binary flag bitmasks ($Z, C, N, V$), machine states, stop reasons, LED values, and exact executed step counts.
5. **Headless Virtual I/O Contracts:**
   - **Port 0x00 (Input FIFO):** Empty FIFO reads return `0x00` and increment underflow counters without triggering a CPU fault.
   - **Port 0x01 (Text Console):** Printable ASCII is streamed to stdout; non-printable characters are hex-escaped (`\xHH`); raw bytes are captured.
   - **Port 0x02 (Numeric Console):** Values are formatted as space-separated unsigned decimals.
   - **Port 0x10 (Virtual LEDs):** Final LED state is presented in hex and 8-bit binary bitmask format.
   - **Invalid Port Directions:** Writing to Port `0x00` or reading from Ports `0x01`, `0x02`, or `0x10` is rejected by `SystemBus` and causes an immediate CPU fault (`CpuFaultType::InvalidPort`, Exit Code `3`).
6. **Resolution of `RISK-P3-001` (Event-Journal Memory Growth):**
   - **Problem:** In headless execution (`cli::execute_run`), `SystemBus::write_byte` and `write_io` appended `MemoryWriteEvent` and `IoEvent` records without drainage. Over 1,000,000 steps, undrained vectors accumulated 500,000 event records in the event vectors, causing process memory commit to grow by up to +7.06 MB over baseline.
   - **Intervention:** Added `bus.clear_write_events();` inside `cli::execute_run`'s step loop.
   - **Verification:** Source proof confirms `SystemBus::clear_write_events()` explicitly clears both `memory_write_events_` and `io_events_`. Multi-trial empirical profiling across 100k, 250k, 500k, and 1,000,000 steps demonstrates bounded process memory, with post-fix private-commit differences ranging from -0.008 MB to +0.008 MB relative to the compute baseline and no sustained growth comparable to the pre-fix results. Direct unit assertion in `test_cli.cpp` verifies that event vectors are drained to size 0.
   - **Preservation:** `DebuggerSession::execute_single_step` retains its own per-step event harvesting for `ExecutionDelta`, ensuring debugger/GUI functionality is completely untouched.
7. **Test Suite Expansion & Multi-Preset Pass Distribution:** The core unit test suite expanded from 272 to 276 tests (+4 new CLI unit tests in `test_cli.cpp`, bringing `test_cli.cpp` to 29 comprehensive fixtures). All 276 core unit tests passed under all four CMake presets (`ci`, `debug`, `gui`, `gui-release`), while all 83 Qt GUI test slots passed under `gui` and `gui-release`.

---

## 2. Initial Git State & Preserved Pre-Existing Work

The audit began from a verified starting state on branch `master` at HEAD commit `5a88e11b7833a119ed847733bcc4db2bc4cbbbfa`.

All pre-existing tracked modifications and untracked deliverables from earlier phases were strictly preserved:
- `tests/unit/test_alu.cpp` (Phase 2 tests)
- `tests/unit/test_cpu_instructions.cpp` (Phase 2 tests)
- `tests/unit/test_memory.cpp` (Phase 3 tests)
- `tests/unit/test_bus.cpp` (Phase 3 tests)
- `tests/unit/test_stack_instructions.cpp` (Phase 3 tests)
- `tests/unit/test_assembler.cpp` (Phase 4 tests)
- `docs/QA_PHASE0_BASELINE_REPORT.md`
- `docs/QA_PHASE1_SPEC_CONFORMANCE_REPORT.md`
- `docs/QA_PHASE2_CPU_EDGE_CASE_REPORT.md` (Version 1.3)
- `docs/QA_PHASE3_MEMORY_BUS_STACK_REPORT.md` (Version 1.2)
- `docs/QA_PHASE4_ASSEMBLER_DIRECTIVES_REPORT.md` (Version 1.3)

---

## 3. CLI Interface and Exit-Code Conformance

### 3.1 CLI Invocation Syntax

The BitForge command-line interface provides four subcommands:
```text
Usage:
  bitforge help | -h | --help
  bitforge version | -v | --version
  bitforge assemble <source.asm>
  bitforge run <source.asm> [--max-steps N] [--dump-regs]
```

*Note on `assemble` syntax:* The `assemble` command accepts exactly one argument: the source file path. It does not accept an `-o` output flag; passing flags or extra arguments to `assemble` produces an unexpected argument error and exits with Exit Code `1`.

### 3.2 Argument Parsing & Validation Matrix

The argument parsing engine in `src/cli/runner.cpp` (`parse_arguments`) was audited against valid and invalid permutations:

| Command / Invocation | Description | Expected Outcome | Actual Exit Code | Output Signature | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `bitforge --help` | Help option flag | Help text, Exit 0 | `0` | `"Usage: ..."` | **Conformant** |
| `bitforge -h` | Short help flag | Help text, Exit 0 | `0` | `"BitForge ..."` | **Conformant** |
| `bitforge help` | Help subcommand | Help text, Exit 0 | `0` | `"Exit Codes: ..."` | **Conformant** |
| `bitforge --version` | Version option flag | Version text, Exit 0 | `0` | `"BitForge 0.1.0 ..."` | **Conformant** |
| `bitforge -v` | Short version flag | Version text, Exit 0 | `0` | `"BitForge 0.1.0 ..."` | **Conformant** |
| `bitforge version` | Version subcommand | Version text, Exit 0 | `0` | `"BitForge 0.1.0 ..."` | **Conformant** |
| `bitforge` | No arguments | Usage error, Exit 1 | `1` | `"Error: No command specified."` | **Conformant** |
| `bitforge unknown_cmd` | Unknown subcommand | Usage error, Exit 1 | `1` | `"Error: Unknown command 'unknown_cmd'."` | **Conformant** |
| `bitforge assemble` | Missing source path | Usage error, Exit 1 | `1` | `"Error: 'assemble' requires a source file path."` | **Conformant** |
| `bitforge assemble file.asm extra` | Extra positional argument | Usage error, Exit 1 | `1` | `"Error: Unexpected argument 'extra' for 'assemble'."` | **Conformant** |
| `bitforge run` | Missing source path | Usage error, Exit 1 | `1` | `"Error: 'run' requires a source file path."` | **Conformant** |
| `bitforge run file.asm --bogus` | Unknown option | Usage error, Exit 1 | `1` | `"Error: Unknown option '--bogus'."` | **Conformant** |
| `bitforge run file.asm --max-steps` | Missing option value | Usage error, Exit 1 | `1` | `"Error: Option '--max-steps' requires a value."` | **Conformant** |
| `bitforge run file.asm --max-steps -5`| Negative step count | Usage error, Exit 1 | `1` | `"Error: Invalid value for --max-steps: '-5'."` | **Conformant** |
| `bitforge run file.asm --max-steps 0` | Zero step count | Usage error, Exit 1 | `1` | `"Error: Invalid value for --max-steps: '0'."` | **Conformant** |
| `bitforge run file.asm --max-steps 10x`| Non-numeric suffix | Usage error, Exit 1 | `1` | `"Error: Invalid value for --max-steps: '10x'."` | **Conformant** |
| `bitforge run file.asm --max-steps 999..`| 64-bit integer overflow | Usage error, Exit 1 | `1` | `"Error: Invalid value for --max-steps: '999...'."` | **Conformant** |
| `bitforge run f1.asm f2.asm` | Multiple source files | Usage error, Exit 1 | `1` | `"Error: Multiple source files specified for 'run'."` | **Conformant** |

### 3.3 Exit-Code Contract Verification

The documented process exit codes (v1.7 §10.3) were verified via automated unit testing (`test_cli.cpp`) and real Windows process executions of `build/ci/bitforge.exe`:

| Exit Code | Specification Requirement | Real Process Trigger Scenario | Process Exit Status | Output Verification |
| :---: | :--- | :--- | :---: | :--- |
| **`0`** | Successful assembly or normal program halt | `bitforge run examples/01_arithmetic.asm` | **`0`** | Clean execution to `Halted` |
| **`1`** | Usage error or invalid arguments | `bitforge run prog.asm --max-steps 0` | **`1`** | `"Error: Invalid value for --max-steps ..."` |
| **`2`** | Source file read failure or assembly error | `bitforge run non_existent_file.asm`<br>`bitforge run bad_syntax.asm` | **`2`** | `"Error: Unable to open source file ..."`<br>`"Assembly failed with ... error(s)"` |
| **`3`** | CPU execution fault | `bitforge run bad_opcode.asm` (`0xFF`)<br>`bitforge run stack_underflow.asm` (`POP R0` at reset `SP = 0xFFFF`, violating `SP < 0xFFFF` per ADR-001)<br>`bitforge run unmapped_port.asm` (`IN R0, 0x05`) | **`3`** | `"CPU Fault: InvalidOpcode ..."`<br>`"CPU Fault: StackUnderflow ..."`<br>`"CPU Fault: InvalidPort ..."` |
| **`4`** | Instruction execution budget exceeded | `bitforge run loop.asm --max-steps 10` | **`4`** | `"Error: Instruction budget exceeded (10 steps)"` |

---

## 4. Execution Budget and Termination Invariants

### 4.1 Boundary Step Analysis

To verify boundary step behavior between normal program termination (`HALT`) and budget expiration, an exact 5-instruction program was executed under varying `--max-steps` values:
```asm
.ORG 0x0200
.ENTRY start
start:
    LDI R0, 10         ; Step 1
    LDI R1, 20         ; Step 2
    ADD R0, R1         ; Step 3
    ST [0x8000], R0    ; Step 4
    HALT               ; Step 5
```

The execution loop in `src/cli/runner.cpp`:
```cpp
while ((cpu.state() == MachineState::Ready || cpu.state() == MachineState::Running) &&
       steps_executed < options.max_steps) {
    StepResult step_res = cpu.step();
    ++steps_executed;
    bus.clear_write_events();
    if (step_res.faulted || cpu.state() == MachineState::Faulted) {
        stop_reason = "Faulted";
        break;
    }
    if (step_res.halted || cpu.state() == MachineState::Halted) {
        stop_reason = "Halted";
        break;
    }
}
```

Boundary test results (`Cli.InstructionBudgetBoundaryExactSteps`):
- **Budget = 4:** `steps_executed` reaches 4 after `ST [0x8000], R0`. The condition `steps_executed < options.max_steps` evaluates to `false`. Loop terminates with `cpu.state() == MachineState::Running`. Returns **Exit Code `4`** (`BudgetExceeded`).
- **Budget = 5:** Step 5 executes `HALT`. `step_res.halted` evaluates to `true` and `cpu.state() == MachineState::Halted`. The loop breaks immediately with `stop_reason = "Halted"`. Returns **Exit Code `0`** (`Success`). HALT strictly wins at the exact step boundary.
- **Budget = 6:** Step 5 executes `HALT` and loop breaks with `stop_reason = "Halted"`. Step 6 is never executed. Returns **Exit Code `0`** (`Success`).

### 4.2 Deterministic Loop Termination

A self-looping program (`loop: JMP loop`) was executed under budgets of 10, 100, 1,000, 100,000, and 1,000,000 steps. In all cases, execution halted deterministically when `steps_executed == max_steps`, returning Exit Code `4` without hangs or unbounded recursion.

---

## 5. Register Dump Accuracy (`--dump-regs`)

When `--dump-regs` is enabled, `dump_registers()` in `src/cli/runner.cpp` formats the architectural CPU state.

Verification of register dump output was conducted using a test program loading distinct values into every architectural register:
```asm
.ORG 0x0200
.ENTRY start
start:
    LDI R0, 0x11
    LDI R1, 0x22
    LDI R2, 0x33
    LDI R3, 0x44
    LDI16 IX, 0x5566
    HALT
```

Captured register dump output:
```text
==================================================
BitForge Register Dump
==================================================
State:          Halted
Stop Reason:    Halted
Steps Executed: 6
PC: 0x0210   SP: 0xFFFF   IX: 0x5566
R0: 0x11     R1: 0x22     R2: 0x33     R3: 0x44
Flags: [Z=0, C=0, N=0, V=0]
LEDs:  0x00 (0b00000000)
==================================================
```

All fields strictly match the architectural specification:
- `PC: 0x0210`: Initial `0x0200` + 3(LDI) + 3(LDI) + 3(LDI) + 3(LDI) + 3(LDI16) + 1(HALT) = `0x0210`.
- `SP: 0xFFFF`: Reset stack pointer sentinel unchanged (no stack operations).
- `IX: 0x5566`: Exact 16-bit word loaded.
- `R0..R3`: Exact 8-bit values (`0x11`, `0x22`, `0x33`, `0x44`).
- `Flags`: Flag values reflect final non-affecting instructions.
- `Steps Executed`: Exactly 6 steps recorded.

---

## 6. Headless Virtual I/O Conformance

Headless CLI execution relies on `bitforge::SystemBus` for virtual peripheral interaction:

### 6.1 Mapped Peripheral Behavior

| Port | Peripheral Designation | Direction | Specification Contract | Headless Verification Result | Status |
| :---: | :--- | :---: | :--- | :--- | :--- |
| **`0x00`** | Input FIFO | Read-Only | Popping an empty FIFO returns `0x00`, logs an underflow event, and **does not** fault the CPU. | Verified in `Cli.VirtualIoEmptyFifoUnderflowNoFault`. `IN R0, 0x00` on empty FIFO yields `R0 = 0x00`, Exit Code `0`. | **Conformant** |
| **`0x01`** | Text Console | Write-Only | Printable ASCII streamed directly; non-printable bytes formatted as `\xHH`. Raw bytes preserved. | Verified via `examples/02_io_demo.asm`. Streams `"HI!"` with newline handling. | **Conformant** |
| **`0x02`** | Numeric Console | Write-Only | Unsigned integer values printed as space-separated decimals. | Verified via `examples/02_io_demo.asm`. Outputs `"42"`. | **Conformant** |
| **`0x10`** | Virtual LEDs | Write-Only | 8-bit latch; updates LED state. Output displayed in hex and binary format. | Verified via `examples/02_io_demo.asm`. Outputs `LED State: 0x55 (0b01010101)`. | **Conformant** |

### 6.2 Port Direction Violation & Unmapped Port Faults

The specification strictly forbids writing to input-only ports or reading from output-only or unmapped ports. When a direction violation occurs, `SystemBus::read_io` returns `std::nullopt` and `SystemBus::write_io` returns `false`, which causes `Cpu` to fault immediately with `CpuFaultType::InvalidPort`.

Verified in `Cli.VirtualIoInvalidPortDirectionFaults`:
1. `OUT 0x00, R0` (Write to Input FIFO) $\rightarrow$ Exit Code `3` (`CPU Fault: InvalidPort`).
2. `IN R0, 0x01` (Read from Text Console) $\rightarrow$ Exit Code `3` (`CPU Fault: InvalidPort`).
3. `IN R0, 0x02` (Read from Numeric Console) $\rightarrow$ Exit Code `3` (`CPU Fault: InvalidPort`).
4. `IN R0, 0x10` (Read from Virtual LEDs) $\rightarrow$ Exit Code `3` (`CPU Fault: InvalidPort`).
5. `IN R0, 0x05` / `OUT 0x05, R0` (Unmapped port) $\rightarrow$ Exit Code `3` (`CPU Fault: InvalidPort`).

---

## 7. SystemBus Event-Journal Stress & Memory Profiling (`RISK-P3-001`)

### 7.1 Problem Definition & Lifecycle Audit

In Phase 3, `RISK-P3-001` identified that `SystemBus` records memory writes and I/O interactions into internal vectors:
```cpp
void SystemBus::write_byte(word_t address, byte_t value) {
    ...
    memory_write_events_.push_back(MemoryWriteEvent{address, old_val, value});
}

bool SystemBus::write_io(port_t port, byte_t value) {
    ...
    io_events_.push_back(IoEvent{IoDirection::Write, port, value});
    ...
}
```

- **Debugger Session:** In `src/debugger/debugger_session.cpp` (`execute_single_step`), `sys_bus->clear_write_events()` is called before stepping, and `sys_bus->take_memory_write_events()` / `take_io_events()` are called after stepping to build `ExecutionDelta`. These event queues are drained on every single step.
- **Headless CLI Runner:** In `src/cli/runner.cpp` (`execute_run`), the step loop previously called `cpu.step()` repeatedly without ever draining `memory_write_events_` or `io_events_`.
- **Consumer Isolation:** The CLI runner never reads or inspects `memory_write_events_` or `io_events_`. It formats output solely from `text_output_`, `numeric_output_`, and `led_state_`. Consequently, accumulating these records during headless execution consumed heap memory for data that was never used.

### 7.2 Profiling Methodology and Metrics

Memory measurements were collected on Windows 11 using the Windows NT process status API (`psapi.dll`, `GetProcessMemoryInfo` with `PROCESS_MEMORY_COUNTERS_EX`). Two OS-level metrics were tracked:
- **`PeakWorkingSetSize` (`PeakWorkingSet64`):** The maximum physical RAM pages resident in the process working set, in bytes. Note that working set is managed dynamically by the Windows memory manager (page trimming, caching) and reflects physical page residency rather than raw C++ heap allocations.
- **`PeakPagefileUsage`:** The peak Commit Charge value in bytes for the process during its lifetime. Per MSDN PSAPI specification, Commit Charge represents the total amount of private memory that the OS memory manager has committed for the running process.
- **Measurement Limitation:** Natively on Windows MSYS2 UCRT64, direct heap-profiling instrumentation tools (such as Valgrind Massif) are not available. These metrics are process-level OS measurements, not direct C++ heap profiler traces. Empirical results across the tested workloads and step counts show bounded resource usage and no sustained growth, but do not mathematically establish $O(1)$ memory consumption.

Controlled benchmark workloads were evaluated across 3 repeatable trials per configuration:
1. **Compute Baseline:** `loop: JMP loop` (0 memory writes, 0 I/O operations).
2. **Memory-Write Loop:** `loop: ST [0x1000], R0; JMP loop` (1 memory write every 2 instructions).
3. **I/O-Write Loop:** `loop: OUT 0x10, R0; JMP loop` (1 I/O write every 2 instructions).
4. **Console-Text Workload:** `loop: OUT 0x01, R0; JMP loop` (1 character written to virtual text console every 2 instructions; demonstrates intentionally retained output buffers).

### 7.3 Multi-Trial Benchmark Results: Before Fix vs. After Fix

All measurements below report mean values across 3 repeated runs with identical binary compile options (`-O3`, Release preset `ci`):

#### Baseline Deficit (Before Fix — Un-drained Journals):

| Workload | Step Budget | Executed Steps | Peak Working Set (MB) | Peak Commit (`PeakPagefileUsage`) | Commit Difference vs. Compute Baseline |
| :--- | :--- | :--- | :---: | :---: | :---: |
| **Compute Baseline** | 100,000 | 100,000 | 5.655 MB | 1.056 MB | Baseline |
| **Memory-Write Loop** | 100,000 | 100,000 | 5.811 MB | 1.568 MB | **+0.512 MB** |
| **I/O-Write Loop** | 100,000 | 100,000 | 6.430 MB | 2.089 MB | **+1.033 MB** |
| **Compute Baseline** | 250,000 | 250,000 | 5.659 MB | 1.064 MB | Baseline |
| **Memory-Write Loop** | 250,000 | 250,000 | 6.510 MB | 2.091 MB | **+1.027 MB** |
| **I/O-Write Loop** | 250,000 | 250,000 | 7.283 MB | 3.109 MB | **+2.045 MB** |
| **Compute Baseline** | 500,000 | 500,000 | 5.658 MB | 1.061 MB | Baseline |
| **Memory-Write Loop** | 500,000 | 500,000 | 7.197 MB | 3.117 MB | **+2.056 MB** |
| **I/O-Write Loop** | 500,000 | 500,000 | 8.182 MB | 5.145 MB | **+4.084 MB** |
| **Compute Baseline** | 1,000,000 | 1,000,000 | 5.663 MB | 1.065 MB | Baseline |
| **Memory-Write Loop** | 1,000,000 | 1,000,000 | 8.188 MB | 5.111 MB | **+4.046 MB** |
| **I/O-Write Loop** | 1,000,000 | 1,000,000 | 10.173 MB | 8.129 MB | **+7.064 MB** |

The data confirms that un-drained event journals caused process commit to grow monotonically by up to +7.06 MB over baseline at 1,000,000 steps, verifying `RISK-P3-001`.

#### Post-Fix Verification (With `bus.clear_write_events()` on Every Step):

| Workload | Step Budget | Executed Steps | Peak Working Set (MB) | Peak Commit (`PeakPagefileUsage`) | Commit Difference vs. Compute Baseline |
| :--- | :--- | :--- | :---: | :---: | :---: |
| **Compute Baseline** | 100,000 | 100,000 | 5.663 MB | 1.061 MB | Baseline |
| **Memory-Write Loop** | 100,000 | 100,000 | 5.661 MB | 1.061 MB | **0.000 MB** |
| **I/O-Write Loop** | 100,000 | 100,000 | 5.663 MB | 1.065 MB | **+0.004 MB** |
| **Compute Baseline** | 250,000 | 250,000 | 5.659 MB | 1.060 MB | Baseline |
| **Memory-Write Loop** | 250,000 | 250,000 | 5.664 MB | 1.060 MB | **0.000 MB** |
| **I/O-Write Loop** | 250,000 | 250,000 | 5.660 MB | 1.064 MB | **+0.004 MB** |
| **Compute Baseline** | 500,000 | 500,000 | 5.659 MB | 1.064 MB | Baseline |
| **Memory-Write Loop** | 500,000 | 500,000 | 5.658 MB | 1.056 MB | **-0.008 MB** |
| **I/O-Write Loop** | 500,000 | 500,000 | 5.665 MB | 1.068 MB | **+0.004 MB** |
| **Compute Baseline** | 1,000,000 | 1,000,000 | 5.654 MB | 1.057 MB | Baseline |
| **Memory-Write Loop** | 1,000,000 | 1,000,000 | 5.658 MB | 1.055 MB | **-0.002 MB** |
| **I/O-Write Loop** | 1,000,000 | 1,000,000 | 5.665 MB | 1.065 MB | **+0.008 MB** |

Across the tested workloads and instruction budgets ranging from 100,000 to 1,000,000 steps, post-fix private-commit differences relative to the compute baseline ranged from **-0.008 MB to +0.008 MB**, with no sustained growth comparable to the monotonic pre-fix growth of up to +7.06 MB. These values represent OS-level process status measurements (`PeakPagefileUsage` / `PeakWorkingSet64`), rather than direct heap profiling or mathematical proof of $O(1)$ memory usage.

#### Retained Console Output Workload (Contrast Case):

For `console_text` (writing characters to Port `0x01`), the text console buffer intentionally retains output for terminal display at completion. Under identical 3-trial benchmarking:
- 100,000 steps: Peak Commit = 1.227 MB (+0.166 MB vs. baseline)
- 250,000 steps: Peak Commit = 1.701 MB (+0.641 MB vs. baseline)
- 500,000 steps: Peak Commit = 2.201 MB (+1.137 MB vs. baseline)
- 1,000,000 steps: Peak Commit = 3.227 MB (+2.170 MB vs. baseline)

This contrast demonstrates that while `text_output_` intentionally accumulates console text as required by specification, the unconsumed debugger journals (`memory_write_events_`, `io_events_`) are completely drained and exhibit zero growth.

### 7.4 Implementation Proof of Drainage and Vector Capacity Semantics

In `src/bus/system_bus.cpp`:
```cpp
void SystemBus::clear_write_events() noexcept {
    memory_write_events_.clear();
    io_events_.clear();
}
```
Source inspection confirms that `clear_write_events()` explicitly calls `.clear()` on **both** `memory_write_events_` and `io_events_`.

#### Vector Capacity and Clearing Semantics:
In C++, `std::vector::clear()` destroys all stored elements and sets `size()` to 0; it does not deallocate the underlying heap buffer capacity (`capacity()` remains unchanged). In the headless execution loop of `cli::execute_run`:
- Each CPU step generates at most 1 to 2 event records before `clear_write_events()` is executed.
- Because `clear_write_events()` executes on every single step, the vectors never accumulate entries across steps.
- Existing unit tests in `tests/unit/test_cli.cpp` assert that `bus.memory_write_events().empty()` and `bus.io_events().empty()` evaluate to true (i.e. size is 0). They assert empty state rather than inspecting numeric capacity.
- Rather than continually reallocating new storage across millions of steps, the vectors reuse their existing buffer capacity for the 1 to 2 records generated per step, preventing the unbounded allocation growth that occurred prior to the fix.

#### Regression-Test Scope and Architectural Boundary:
Direct unit assertions in `tests/unit/test_cli.cpp` (`Cli.HeadlessExecutionJournalDrainageAndOutputPreservation`) verify that `SystemBus::clear_write_events()` empties both event vectors (`size() == 0`). In `cli::execute_run`, `SystemBus` is constructed as a function-local stack object and is not exposed publicly or via polymorphic dependency injection. To avoid intrusive test hooks or altering the public architecture, the evidence that `clear_write_events()` is executed on every step in the headless loop is provided by static source inspection of `execute_run` (line 267) and by the empirical before-and-after process profiling benchmarks across 100,000 to 1,000,000 steps.

---

## 8. Test Matrix & Multi-Preset Regression Results

The BitForge core test suite expanded from 272 unit tests to 276 unit tests (+4 tests, +1.47%). All tests were executed across all four CMake build presets configured in `CMakePresets.json`:

| CMake Build Preset | Build Toolchain | Unit Tests Executed | Unit Tests Passed | GUI Tests Executed | GUI Tests Passed | Total Pass Rate |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **`ci`** | GCC 14.2.0 (MSYS2 UCRT64, Ninja) | 276 | 276 | N/A (CLI/Core Preset) | N/A | **100%** |
| **`debug`** | GCC 14.2.0 (MSYS2 UCRT64, Ninja) | 276 | 276 | N/A (Core Debug Preset) | N/A | **100%** |
| **`gui`** | GCC 14.2.0 + Qt 6.8.0 (Ninja) | 276 | 276 | 83 test entries | 83 | **100%** |
| **`gui-release`**| GCC 14.2.0 + Qt 6.8.0 (Ninja, `-O3`) | 276 | 276 | 83 test entries | 83 | **100%** |

### Breakdown of CLI Test Suite (`tests/unit/test_cli.cpp`)

The CLI test suite comprises 29 comprehensive test fixtures (25 baseline fixtures from earlier phases plus 4 dedicated fixtures added in Phase 5):
1. `HelpFlag`: Validates `--help` text and exit code 0.
2. `HelpShortFlag`: Validates `-h` text and exit code 0.
3. `HelpSubcommand`: Validates `help` text and exit code 0.
4. `VersionFlag`: Validates `--version` text and exit code 0.
5. `VersionShortFlag`: Validates `-v` text and exit code 0.
6. `VersionSubcommand`: Validates `version` text and exit code 0.
7. `UsageErrorsNoArguments`: Rejection of empty CLI invocation (exit 1).
8. `UsageErrorsUnknownCommand`: Rejection of unknown subcommands (exit 1).
9. `UsageErrorsAssembleMissingSource`: Rejection of `assemble` without file (exit 1).
10. `UsageErrorsAssembleUnexpectedArgument`: Rejection of `assemble` extra arguments (exit 1).
11. `UsageErrorsRunMissingSource`: Rejection of `run` without file (exit 1).
12. `UsageErrorsRunMissingMaxStepsValue`: Rejection of `--max-steps` without value (exit 1).
13. `UsageErrorsRunInvalidMaxStepsNonNumeric`: Rejection of non-numeric step counts (exit 1).
14. `UsageErrorsRunInvalidMaxStepsZero`: Rejection of zero step count (exit 1).
15. `UsageErrorsRunUnknownOption`: Rejection of unknown flags (exit 1).
16. `UsageErrorsRunMultipleFiles`: Rejection of multiple input files (exit 1).
17. `AssembleNonExistentFile`: Rejection of missing source file (exit 2).
18. `AssembleSyntaxError`: Diagnostics and exit code 2 on assembly error.
19. `AssembleSuccess`: Successful assembly segment reporting and exit code 0.
20. `RunSuccessBaseline`: Successful end-to-end execution and exit code 0.
21. `RunSuccessWithDumpRegs`: Register dump presentation and formatting.
22. `RunVirtualIo`: Presentation of text console, numeric console, and LED state.
23. `RunCpuFault`: CPU execution fault reporting and exit code 3.
24. `RunBudgetExceeded`: Budget exhaustion reporting and exit code 4.
25. `RunBudgetExceededEqualsSyntax`: Support for `--max-steps=N` syntax.
26. `InstructionBudgetBoundaryExactSteps` *(New)*: Exact 5-instruction program boundary testing across budgets 4, 5, and 6.
27. `VirtualIoEmptyFifoUnderflowNoFault` *(New)*: Empty FIFO reads return `0x00` without faulting CPU.
28. `VirtualIoInvalidPortDirectionFaults` *(New)*: Fault verification across all invalid port directions.
29. `HeadlessExecutionJournalDrainageAndOutputPreservation` *(New)*: Direct assertion of event queue drainage and preservation of final peripheral output and registers.

---

## 9. Residual Risks & Technical Debt Assessment

| Risk / Gap ID | Component | Description | Severity | Mitigation / Resolution Status |
| :--- | :--- | :--- | :--- | :--- |
| **`RISK-P3-001`** | `SystemBus` Event Log | In headless CLI execution, `SystemBus::write_byte` and I/O writes accumulate `MemoryWriteEvent` and `IoEvent` records without drainage. | High | **Resolved & Closed:** In Phase 5, `bus.clear_write_events()` was added to `cli::execute_run`. Verified bounded process memory: post-fix private-commit differences ranged from -0.008 MB to +0.008 MB across 1M steps with no sustained growth. |
| **`GAP-P4-001`** | Test Suite | Diagnostic column coordinates across 31 malformed snippets in `AssemblerTest.MalformedInputComprehensiveMatrix` are asserted for failure and non-empty diagnostics, but not granular column numbers. | Low | **Documented Gap:** Representative line and column precision is asserted in `DiagnosticSourceLocationsPrecision`. |
| **`RISK-P5-001`** | Virtual I/O | Console output buffers (`text_output_`, `numeric_output_`) grow monotonically if a long-running program emits millions of characters. | Low | **Documented Architectural Behavior:** Per v1.7 §7, console output buffers capture cumulative console text and are presented upon execution completion. Not a leak. |

---

## 10. Complete Changed-File Inventory & Git Status

### Tracked Modifications:
1. [`src/cli/runner.cpp`](file:///D:/BitForge/src/cli/runner.cpp): Added `bus.clear_write_events();` inside `execute_run` loop to drain unconsumed event journals during headless execution.
2. [`tests/unit/test_cli.cpp`](file:///D:/BitForge/tests/unit/test_cli.cpp): Added 4 new unit test fixtures verifying budget step boundaries, empty FIFO reads, invalid port directions, and direct journal drainage.
3. [`tests/unit/test_alu.cpp`](file:///D:/BitForge/tests/unit/test_alu.cpp): Preserved from Phase 2.
4. [`tests/unit/test_assembler.cpp`](file:///D:/BitForge/tests/unit/test_assembler.cpp): Preserved from Phase 4.
5. [`tests/unit/test_bus.cpp`](file:///D:/BitForge/tests/unit/test_bus.cpp): Preserved from Phase 3.
6. [`tests/unit/test_cpu_instructions.cpp`](file:///D:/BitForge/tests/unit/test_cpu_instructions.cpp): Preserved from Phase 2.
7. [`tests/unit/test_memory.cpp`](file:///D:/BitForge/tests/unit/test_memory.cpp): Preserved from Phase 3.
8. [`tests/unit/test_stack_instructions.cpp`](file:///D:/BitForge/tests/unit/test_stack_instructions.cpp): Preserved from Phase 3.

### Untracked Reports:
- `docs/QA_PHASE0_BASELINE_REPORT.md`
- `docs/QA_PHASE1_SPEC_CONFORMANCE_REPORT.md`
- `docs/QA_PHASE2_CPU_EDGE_CASE_REPORT.md`
- `docs/QA_PHASE3_MEMORY_BUS_STACK_REPORT.md`
- `docs/QA_PHASE4_ASSEMBLER_DIRECTIVES_REPORT.md`
- `docs/QA_PHASE5_CLI_HEADLESS_STRESS_REPORT.md`

### Current Git Status (`git status --short --branch`):
```text
## master...origin/master [ahead 1]
 M src/cli/runner.cpp
 M tests/unit/test_alu.cpp
 M tests/unit/test_assembler.cpp
 M tests/unit/test_bus.cpp
 M tests/unit/test_cli.cpp
 M tests/unit/test_cpu_instructions.cpp
 M tests/unit/test_memory.cpp
 M tests/unit/test_stack_instructions.cpp
?? docs/QA_PHASE0_BASELINE_REPORT.md
?? docs/QA_PHASE1_SPEC_CONFORMANCE_REPORT.md
?? docs/QA_PHASE2_CPU_EDGE_CASE_REPORT.md
?? docs/QA_PHASE3_MEMORY_BUS_STACK_REPORT.md
?? docs/QA_PHASE4_ASSEMBLER_DIRECTIVES_REPORT.md
?? docs/QA_PHASE5_CLI_HEADLESS_STRESS_REPORT.md
```

---

## 11. Sign-Off & Recommendations for Phase 6

### 11.1 Phase 5 Audit Sign-Off

The BitForge CLI runner, headless execution engine, execution-budget handling, process exit codes, virtual peripheral I/O interfaces, and event-journal resource management demonstrably satisfy all normative architectural requirements of specification v1.7. `RISK-P3-001` is resolved with verified empirical memory metrics.

**Phase 5 is hereby certified COMPLETE and fully signed off.**

### 11.2 Recommendations for Phase 6 (CPU Execution & Debugger Observability Audit)

Phase 6 will audit interactive debugger observability:
1. **Debugger Breakpoint Placement & Hit Detection:** Breakpoint resolution on opcode addresses vs. operand bytes.
2. **Stepping Semantics:** `step_into`, `step_over`, and `step_out` handling across subroutines and branch instructions.
3. **Trace History Ring Buffer:** Circular buffer capacity limits, overflow behavior, and historical step reconstruction.
4. **Execution Delta Accuracy:** Verification of register, flag, memory write, and I/O event mutations committed during interactive single steps.
5. **Interactive Machine State Transitions:** Transitions between `Ready`, `Running`, `Paused`, `Halted`, and `Faulted` states under interactive debugging.

---

## 12. Phase 5.1, 5.2, and 5.3 Reconciliation Summary

### 12.1 Phase 5.1 Verification and Drainage Proof
1. **Source Proof of Drainage:** Inspected `SystemBus::clear_write_events()` in `src/bus/system_bus.cpp`. Proved that it unconditionally invokes `.clear()` on both `memory_write_events_` and `io_events_`.
2. **Direct Test Assertions:** Augmented `Cli.HeadlessExecutionJournalDrainageAndOutputPreservation` in `tests/unit/test_cli.cpp` with direct assertions verifying that event queue lengths are reduced to 0 upon calling `clear_write_events()`.
3. **CLI Fixture Count Reconciliation:** Reconciled the fixture count in Section 8: `tests/unit/test_cli.cpp` contains exactly 29 fixtures (25 from earlier phases + 4 from Phase 5). Total core tests remain 276.
4. **CLI Syntax Reconciliation:** Clarified that `assemble` takes solely `<source.asm>`. Removed any mention of an unsupported `-o` flag.
5. **Architectural Stack Context:** Confirmed that at CPU reset, $SP$ is initialized to `0xFFFF` per ADR-001 §1. An immediate `POP R0` instruction triggers `FaultType::StackUnderflow` because the pre-validation check requires $SP < 0xFFFF$, leading to process Exit Code `3`.

### 12.2 Phase 5.2 Final Evidence Reconciliation
1. **Reconciliation of Memory Claims:** Corrected all unqualified claims of "0.00 MB" and "Identical" across all sections. Documented that post-fix private-commit differences ranged from **-0.008 MB to +0.008 MB** across workloads and budgets of 100,000 to 1,000,000 steps, showing no sustained growth comparable to pre-fix measurements (+7.06 MB). Emphasized that these are OS process-level metrics, not direct heap allocations or mathematical proofs of $O(1)$ memory consumption.
2. **Vector Clearing vs. Capacity Semantics:** Replaced informal descriptions with precise terminology ("500,000 event records accumulated in the event vector"). Clarified that `std::vector::clear()` resets `size()` to 0 while retaining allocated capacity.
3. **Regression-Test Scope and Limitations:** Clarified the architectural boundary of `Cli.HeadlessExecutionJournalDrainageAndOutputPreservation`: direct assertions test the bus clearing method, while source inspection of `cli::execute_run` and comparative before/after profiling benchmarks verify drainage in the CLI execution loop, preserving production architecture without intrusive test hooks.

### 12.3 Phase 5.3 Closeout and Sign-Off
1. **Windows Memory Metrics Reconciled:** Reconciled API descriptions with Windows PSAPI `PROCESS_MEMORY_COUNTERS_EX`. Specifically documented `PeakWorkingSetSize` and `PeakPagefileUsage` (Peak Commit Charge per MSDN).
2. **Vector Capacity and Test Assertion Scope:** Explicitly noted that unit tests in `test_cli.cpp` assert empty vector sizes (`empty() == true`), not numeric capacity bounds. Removed unsupported numerical capacity claims (`< 128 bytes`).
3. **Repository Inventory Reconciliation:** Corrected Section 10: removed `docs/QA_PHASE4_ASSEMBLER_DIRECTIVES_REPORT.md` from the tracked modifications list, ensuring every file appears in only one category.
4. **Verification Scope:** Phase 5.3 was documentation-only; verified with clean `git diff --check` and 0 trailing whitespace lines. The 276/276 core and 83/83 GUI test results from Phase 5.2 remain authoritative.
5. **Phase 5 Sign-Off:** All Phase 5 audit objectives, fixes, benchmarks, and documentation are complete and verified. **Phase 5 is formally SIGNED OFF.**
