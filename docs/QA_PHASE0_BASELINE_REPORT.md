# BitForge Phase 0 — Repository Baseline, Test Inventory & Risk Assessment Report

- **Date of Baseline Audit:** October 20, 2026
- **Auditor:** Antigravity AI QA Assistant (Pair Programming & Verification Audit)
- **Target Repository:** `D:\BitForge`
- **Target Branch:** `master`
- **Document Version:** 1.1 (Phase 0 Deliverable — Reconciled & Reviewed)
- **Primary Architecture Reference:** `docs/BitForge_Architecture_Specification_v1.7.md` and `docs/decisions/ADR-001-stack-pointer-convention.md`

---

## 1. Executive Summary

BitForge is an educational 8-bit computer emulator, two-pass assembler, system bus, command-line interface (CLI), interactive debugger, and Qt 6 desktop graphical workbench implementing an instruction-set simulator with a 64 KiB memory space and 31 canonical instruction mnemonics.

This Phase 0 audit establishes the initial, verified, evidence-backed quality baseline of the BitForge repository prior to initiating the multi-phase final QA roadmap. The assessment was conducted via an Antigravity-assisted automated and static QA inspection. In strict accordance with Phase 0 non-destructive mandates, no application source files, test implementations, build configuration scripts, assembly examples, or frozen architectural specifications were modified.

### Key Baseline Findings

1. **Git State & Workspace Reconciliation:** At the commencement of Phase 0, the repository working tree was verified completely clean on branch `master` at HEAD commit `5a88e11b7833a119ed847733bcc4db2bc4cbbbfa` (`fix(gui,ux): finalize design token geometry and audit evidence`), ahead of `origin/master` by 1 commit with zero unstaged, staged, or untracked files. Following the non-destructive Phase 0 investigation and report generation, the working tree contains exactly one new, intended deliverable — the untracked baseline report `docs/QA_PHASE0_BASELINE_REPORT.md` — with all pre-existing tracked files strictly preserved.
2. **Preset Build & Execution Parity (100% Pass):** All four established CMake presets (`ci`, `debug`, `gui`, `gui-release`) configure and compile with zero errors and zero compiler warnings. All registered test targets execute with 100% pass rates:
   - **`ci` (Strict CI Build, `-Werror`):** 1/1 CTest target passed (`BitForgeTests`: 233/233 unit tests, 0.14s).
   - **`debug` (Debug Headless):** 1/1 CTest target passed (`BitForgeTests`: 233/233 unit tests, 0.13s).
   - **`gui` (Debug Desktop GUI):** 2/2 CTest targets passed (`BitForgeTests`: 233/233 unit tests in 0.14s; `BitForgeGuiTests`: 83/83 QtTest entries in 12.83s).
   - **`gui-release` (Release Desktop GUI):** 2/2 CTest targets passed (`BitForgeTests`: 233/233 unit tests in 0.06s; `BitForgeGuiTests`: 83/83 QtTest entries in 12.17s).
3. **Exact Test Inventory:**
   - **Core Test Suite (`bitforge_tests.exe`):** Exactly **233 test cases** across 11 test suites in `tests/unit/*.cpp`.
   - **GUI Test Suite (`bitforge_gui_tests.exe`):** Exactly **81 test methods** plus **2 test fixture slots** (`initTestCase`, `cleanupTestCase`) totaling **83 QtTest execution slots** in `tests/gui/test_gui.cpp`.
   - **Combined Execution Total:** **314 test methods** (or **316 test checks** including fixture setup/teardown slots) executed under the `gui` and `gui-release` presets.
4. **Standalone Portable Package Verification:** The Windows x64 distribution generator (`scripts/package_windows.ps1 -Validate`) completes end-to-end execution, building release binaries, deploying 23 required runtime DLLs and Qt platform plugins, staging all 51 assembly examples, archiving into `dist/BitForge-Portable-Windows-x64.zip`, and successfully passing smoke validation under an isolated sanitized PATH (`C:\Windows\System32;C:\Windows;C:\Windows\System32\Wbem`) and a temporary filesystem path containing spaces (`dist/Test Space with Spaces`).
5. **Initial Risk Identification:** While existing regression suites are comprehensive, static inspection revealed several critical and high-priority edge-case gaps requiring rigorous investigation in subsequent phases:
   - 4-byte instruction wraparound across the 64 KiB boundary (`0xFFFF -> 0x0000`).
   - Missing stack lower boundary checks on `POP` and `RET` if `SP` is corrupted below `0xF000`.
   - Unbounded memory consumption in `SystemBus` memory-write event journaling under high-iteration CLI runs.
   - Silent default to entry address `0x0000` when `.ENTRY` is omitted in programs originating at `.ORG > 0x0000`.
   - Watch panel inability to resolve symbolic assembly labels.
   - Complete reliance on Qt offscreen QPA plugin for automated GUI testing, leaving physical desktop High-DPI scaling and window manager focus interactions unverified.

---

## 2. Repository Identity and Initial Git State

Prior to initiating build and test sequences, the repository environment and Git version-control status were captured:

| Property | Value | Evidence / Command Output |
|:---|:---|:---|
| **Working Directory** | `D:\BitForge` | `Get-Location` |
| **Git Repository Root** | `D:/BitForge` | `git rev-parse --show-toplevel` |
| **Active Branch** | `master` | `git status --short --branch` |
| **Tracking Status** | `## master...origin/master [ahead 1]` | 1 local commit ahead of origin |
| **HEAD Commit Hash** | `5a88e11b7833a119ed847733bcc4db2bc4cbbbfa` | `git show --stat 5a88e11` |
| **HEAD Commit Author** | DaviPiyu148 `<piyushpatre16@gmail.com>` | Commit timestamp: Tue Oct 20 16:31:43 2026 +0530 |
| **HEAD Commit Message** | `fix(gui,ux): finalize design token geometry and audit evidence` | Commit log |
| **Remote Origin** | `https://github.com/DaviPiyu148/BitForge.git` | `git remote -v` |
| **Initial Working Tree** | Clean (0 modified, 0 staged, 0 untracked files) | `git status` -> `nothing to commit, working tree clean` |
| **Post-Phase-0 Working Tree** | Exactly 1 untracked file (`docs/QA_PHASE0_BASELINE_REPORT.md`), 0 modified, 0 staged | `git status --short` -> `?? docs/QA_PHASE0_BASELINE_REPORT.md` |

### Recent Git Commit History

```text
5a88e11 (HEAD -> master) fix(gui,ux): finalize design token geometry and audit evidence
6340e60 (origin/master) feat(gui,ux): reconcile design tokens, adopt shared styles, and verify visual hierarchy (Phase 12.5.1)
a2205ed feat(gui,ux): unify visual design system, token architecture, and UI consistency (Phase 12.5)
6094caa moved arch spec md files
f69132b fix(gui,ux): enlarge tab close button cross icon and hit area
b9fd5e5 docs(test,ux): reconcile core test counts, confirm font warning fix, and document packaging limits (Phase 12.4.1)
a9e7a17 fix(gui,ux): fix invisible tab close button, spinbox up button, and font warning (Phase 12.4)
86c82f1 feat(gui,ux): complete icon integration, tab close spacing, device display layout, and library audit (Phase 12.3)
```

---

## 3. Toolchain and Environment

The project requires ISO C++17 compilation, CMake 3.20+, Ninja, and Qt 6. The host system environment provides modern toolchain components installed via MSYS2 UCRT64:

| Component | Detected Version | Executable / Source Path | Architectural Role & Configuration |
|:---|:---|:---|:---|
| **Operating System** | Windows 11 Home / Pro (x86_64) | Microsoft Windows NT 10.0 | Host execution OS |
| **C++ Compiler** | GCC 16.2.0 (Rev6, MSYS2 UCRT64) | `C:\msys64\ucrt64\bin\c++.exe` | Primary C++17 compiler via `-std=c++17` (enforced via CMake `CMAKE_CXX_STANDARD 17` and `CMAKE_CXX_EXTENSIONS OFF`) |
| **Build System Generator** | CMake 4.4.4 | `C:\msys64\ucrt64\bin\cmake.exe` | Project configuration & preset engine |
| **Build Tool** | Ninja 1.13.2 | `C:\msys64\ucrt64\bin\ninja.exe` | Fast parallel compilation engine |
| **Test Runner** | CTest 4.4.4 | `C:\msys64\ucrt64\bin\ctest.exe` | Automated test harness orchestrator |
| **GUI Framework** | Qt 6.11.2 (Release, dynamic) | `C:\msys64\ucrt64\lib\cmake\Qt6` | Widgets, GUI, Core, and Test modules |
| **Qt Meta-Object Tools** | QMake 3.1, moc, rcc, uic | `C:\msys64\ucrt64\bin\qmake.exe` | C++ Qt AUTOMOC / AUTORCC pipeline |
| **Windows Deployment** | windeployqt 6.11.2 | `C:\msys64\ucrt64\bin\windeployqt.exe`| Runtime DLL dependency packager |
| **Coverage Tool** | gcov 16.2.0 | `C:\msys64\ucrt64\bin\gcov.exe` | Available for instrumentation; inactive in Phase 0 |

> [!NOTE]
> The active compiler configuration uses `-std=c++17` with GCC flags `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wnon-virtual-dtor` (and `-Werror` in preset `ci`). The `/std:c++17` flag syntax is specific to MSVC when building with Visual Studio.
>
> In raw Windows PowerShell sessions, `C:\msys64\ucrt64\bin` is not on the persistent machine-wide `PATH`. Consistent with repository scripts (`launch_bitforge.bat` and `scripts/package_windows.ps1`), prepend `$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:PATH"` when executing toolchain commands.

---

## 4. Architecture and Source Inventory

BitForge enforces strict modular decoupling across its CMake targets and 25,082 lines of ISO C++17 code:

```
                              +-------------------------------------------+
                              |          bitforge_gui (.exe)              |
                              |   (Qt 6 Desktop Workbench & Panels)       |
                              +-------------------------------------------+
                                        |                       |
                                        v                       |
   +------------------------------------------+                 |
   |              bitforge (.exe)             |                 |
   |             (CLI Executable)             |                 |
   +------------------------------------------+                 |
             |                                                  |
             v                                                  v
   +--------------------+                             +--------------------+
   |   bitforge_cli     |                             | bitforge_debugger  |
   |  (Static Library)  |                             |  (Static Library)  |
   |  - runner.cpp      |                             |  - debugger_session|
   +--------------------+                             +--------------------+
             \                                                  /
              \-----------------------+------------------------/
                                      |
                                      v
                      +-------------------------------+
                      |        bitforge_core          |
                      |       (Static Library)        |
                      | - CPU Pipeline & ALU          |
                      | - 64 KiB Memory Storage       |
                      | - SystemBus & Virtual I/O     |
                      | - Two-Pass Assembler & Lexer  |
                      |   (lexer.cpp, assembler.cpp)  |
                      +-------------------------------+
```

### 4.1 Target & Source File Breakdown

In `CMakeLists.txt`, the project defines the following primary libraries, executables, and test targets:

| Target Name | CMake Target Type | Member Source Files | Key Header Files | Link Dependencies |
|:---|:---|:---|:---|:---|
| **`bitforge_core`** | Static Library (`add_library`) | `src/memory/memory.cpp`<br>`src/core/alu.cpp`<br>`src/core/cpu.cpp`<br>`src/bus/system_bus.cpp`<br>`src/assembler/lexer.cpp`<br>`src/assembler/assembler.cpp` | `include/bitforge/core/*.hpp`<br>`include/bitforge/memory/*.hpp`<br>`include/bitforge/bus/*.hpp`<br>`include/bitforge/assembler/*.hpp` | *None (Self-contained root)* |
| **`bitforge_cli`** | Static Library (`add_library`) | `src/cli/runner.cpp` | `include/bitforge/cli/runner.hpp` | `bitforge_core` |
| **`bitforge`** | Executable (`add_executable`) | `src/cli/main.cpp`<br>`resources/bitforge.rc` (WIN32) | N/A | `bitforge_cli` |
| **`bitforge_debugger`** | Static Library (`add_library`) | `src/debugger/debugger_session.cpp` | `include/bitforge/debugger/*.hpp` | `bitforge_core` |
| **`bitforge_gui`** | Executable (`add_executable`) | `src/gui/main.cpp`<br>`src/gui/*.cpp` (20 panel/widget files)<br>`resources/bitforge.qrc`<br>`resources/bitforge.rc` (WIN32) | `include/bitforge/gui/*.hpp` (20 panel/widget headers) | `bitforge_debugger`<br>`bitforge_core`<br>`Qt6::Widgets`<br>`Qt6::Gui`<br>`Qt6::Core` |
| **`bitforge_tests`** | Test Executable (`add_executable`) | `tests/test_main.cpp`<br>`tests/unit/*.cpp` (11 unit test suites) | `tests/test_harness.hpp` | `bitforge_core`<br>`bitforge_cli`<br>`bitforge_debugger` |
| **`bitforge_gui_tests`** | Test Executable (`add_executable`) | `tests/gui/test_gui.cpp`<br>`src/gui/*.cpp` (20 panel/widget files)<br>`resources/bitforge.qrc` | N/A | `bitforge_debugger`<br>`bitforge_core`<br>`Qt6::Widgets`<br>`Qt6::Gui`<br>`Qt6::Core`<br>`Qt6::Test` |
| **Assembly Examples** | Examples Library | 51 programs in `examples/**/*.asm` | `examples/README.md` | Tested by `test_example_library.cpp` |

> [!NOTE]
> **Assembler Library Membership:** In `CMakeLists.txt`, the two-pass assembler (`src/assembler/lexer.cpp` and `src/assembler/assembler.cpp`) is directly compiled into the `bitforge_core` static library rather than residing in a separate CMake library target. All core, CLI, debugger, and GUI targets link against `bitforge_core` to access assembler services.

---

## 5. Existing Test Inventory

The project utilizes two distinct automated test harnesses:
1. **Custom Zero-Dependency Unit Test Runner (`tests/test_harness.hpp`):** Powers `bitforge_tests.exe`. Uses explicit test registration via `TEST(Suite, Name)` macros and evaluates runtime assertions (`EXPECT_EQ`, `EXPECT_TRUE`, `EXPECT_FALSE`).
2. **QtTest Framework (`QTest`):** Powers `bitforge_gui_tests.exe`. Runs as an offscreen Qt application (`-platform offscreen`), evaluating GUI widgets, actions, docking, and editor lifecycles with `QCOMPARE`, `QVERIFY`, and `QTRY_COMPARE_WITH_TIMEOUT`.

### 5.1 Comprehensive Test Suite Breakdown

| Test Suite / File | Harness | Executable | Registered Checks | Description / Focus Areas |
|:---|:---|:---|:---:|:---|
| `test_alu.cpp` | Custom | `bitforge_tests.exe` | **8** | ALU addition, subtraction, comparison, bitwise logic, increment/decrement carry preservation, signed overflow. |
| `test_assembler.cpp` | Custom | `bitforge_tests.exe` | **34** | Lexer tokenization, directives (`.ORG`, `.ENTRY`, `.BYTE`, `.WORD`, `.ASCII`), label resolution, overlapping segments rejection, stack collision prevention, diagnostics. |
| `test_bus.cpp` | Custom | `bitforge_tests.exe` | **6** | SystemBus memory delegation, 64 KiB boundary word wrapping, side-effect-free I/O permission queries, memory/I/O journaling. |
| `test_cli.cpp` | Custom | `bitforge_tests.exe` | **25** | CLI arguments, `--help`, `--version`, assemble subcommand, run subcommand, `--max-steps`, `--dump-regs`, exit codes (0, 1, 2, 3, 4). |
| `test_cpu_instructions.cpp`| Custom | `bitforge_tests.exe` | **28** | All 31 instruction opcodes, register encoding validation, exhaustive invalid opcode rejection, fault atomicity, PC wraparound, state transitions. |
| `test_debugger.cpp` | Custom | `bitforge_tests.exe` | **39** | Single-stepping, address breakpoints, continue bypass, source mapping, register/memory/stack inspection, bounded trace history, fault atomicity deltas. |
| `test_example_library.cpp` | Custom | `bitforge_tests.exe` | **52** | Automated execution of all 51 physical examples + Bubble Sort ADS verification with instruction budgets. |
| `test_io_instructions.cpp` | Custom | `bitforge_tests.exe` | **11** | FIFO input consumption, underflow handling, text console ASCII and escape formatting, numeric stream, LED output, invalid port faults. |
| `test_memory.cpp` | Custom | `bitforge_tests.exe` | **11** | 64 KiB byte/word reads and writes, 16-bit word big-endian layout, memory reset, segment loading, boundary overrun protections. |
| `test_stack_instructions.cpp`| Custom | `bitforge_tests.exe` | **9** | PUSH/POP LIFO ordering, stack overflow (`SP <= 0xF000`), stack underflow (`SP >= 0xFFFF`), CALL/RET return address storage, nested calls. |
| `test_types.cpp` | Custom | `bitforge_tests.exe` | **10** | Integer types, widths, endianness helpers (`make_word`, `high_byte`, `low_byte`), opcode table constants, flag structures, enum strings. |
| **Subtotal Core Unit Tests**| | | **233** | **100% PASS** in ~0.14 seconds |
| `test_gui.cpp` (Slots 1–81)| QtTest | `bitforge_gui_tests.exe` | **81** | Full desktop GUI workflow: editor tabs, dirty indicators, assemble, execution controller, breakpoints, layout persistence, themes, token scale. |
| `test_gui.cpp` (Fixtures) | QtTest | `bitforge_gui_tests.exe` | **2** | `initTestCase()` and `cleanupTestCase()` settings sandboxing. |
| **Subtotal GUI Test Slots** | | | **83** | **100% PASS** in ~12.8 seconds |
| **Grand Total Executed Checks**| | | **316** | **100% PASS across all suites** |

### 5.2 Test Inventory Reconciliation History

Historical reports in early phases (e.g., Phase 11, 12.1, 12.3) cited "220 core tests". An erratum documented in `docs/PHASE12_4_1_ACCEPTANCE_REPORT.md` confirmed that `tests/unit/*.cpp` has contained exactly 233 tests continuously since Phase 10.2 (commit `1680914`). Early templates misreported 220 because `ctest` collapses test suites into a single summary line. Running `bitforge_tests.exe` directly confirms:
`Test Summary: 233 passed, 0 failed out of 233 total tests.`

---

## 6. Baseline Build/Test Results for all Four Presets

All four CMake presets defined in `CMakePresets.json` were configured, compiled, and tested sequentially:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:PATH"
cmake --preset <preset>
cmake --build --preset <preset>
ctest --preset <preset> --output-on-failure
```

### 6.1 Preset Verification Matrix

| Preset Name | Build Type | Flags / GUI Status | Build Result | Registered CTest Targets | Executed Test Cases | Test Result | Real Execution Time | Limitations / Notes |
|:---|:---|:---|:---:|:---|:---:|:---:|:---:|:---|
| **`ci`** | `Debug` | `BITFORGE_WARNINGS_AS_ERRORS=ON`<br>`BITFORGE_BUILD_GUI=OFF` | **Clean** (0 warnings, 0 errors) | `BitForgeTests` (1/1) | 233 core tests | **PASSED** (100%) | 0.14 s | Headless only; GUI targets excluded by preset definition. |
| **`debug`** | `Debug` | `BITFORGE_WARNINGS_AS_ERRORS=OFF`<br>`BITFORGE_BUILD_GUI=OFF` | **Clean** (0 warnings, 0 errors) | `BitForgeTests` (1/1) | 233 core tests | **PASSED** (100%) | 0.13 s | Fast developer iteration target. |
| **`gui`** | `Debug` | `BITFORGE_WARNINGS_AS_ERRORS=OFF`<br>`BITFORGE_BUILD_GUI=ON` | **Clean** (0 warnings, 0 errors) | `BitForgeTests`<br>`BitForgeGuiTests` (2/2) | 233 core tests<br>83 GUI test slots | **PASSED** (100%) | 12.97 s | GUI tests execute under Qt `-platform offscreen`. |
| **`gui-release`** | `Release` | `BITFORGE_WARNINGS_AS_ERRORS=OFF`<br>`BITFORGE_BUILD_GUI=ON` | **Clean** (0 warnings, 0 errors) | `BitForgeTests`<br>`BitForgeGuiTests` (2/2) | 233 core tests<br>83 GUI test slots | **PASSED** (100%) | 12.23 s | Release optimization baseline for packaging. |

### 6.2 Standalone Portable Windows Packaging Validation

The portable packaging script was validated directly:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\package_windows.ps1 -Validate
```

**Outcome:**
1. Built `gui-release` binaries (`bitforge_gui.exe`, `bitforge.exe`, `bitforge_cli.exe`).
2. Staged runtime dependencies into `dist/BitForge-Portable-Windows-x64`:
   - 23 required GCC/UCRT runtime DLLs (including `libstdc++-6.dll`, `libwinpthread-1.dll`, `libgcc_s_seh-1.dll`, `libicuin78.dll`, `zlib1.dll`, `libpcre2-16-0.dll`).
   - Qt 6 libraries (`Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`, `Qt6Network.dll`).
   - Platform plugins in `platforms/` (`qwindows.dll`, `qoffscreen.dll`, `qminimal.dll`).
   - All 51 assembly examples, launcher batch script `launch_bitforge.bat`, `QUICKSTART.md`, `LICENSE.txt`, and application icon.
3. Compressed into standalone archive `dist/BitForge-Portable-Windows-x64.zip` (34.9 MB).
4. Validation run in `dist/Test Space with Spaces` under sanitized PATH (`C:\Windows\System32;C:\Windows;C:\Windows\System32\Wbem`):
   - `bitforge_cli.exe --version` returned `BitForge 0.1.0 (Architecture Specification v1.7)` (Exit code 0).
   - `bitforge_cli.exe --help` returned usage instructions (Exit code 0).
   - `bitforge_cli.exe run examples\01_arithmetic.asm --max-steps 1000 --dump-regs` executed and produced expected register dumps (Exit code 0).
   - `bitforge_gui.exe -platform offscreen --smoke-test` returned `[BitForge Smoke Test] SUCCESS: MainWindow and all panels initialized cleanly` (Exit code 0).

---

## 7. Warnings, Failures, Skips and Blocked Checks

### 7.1 Compiler & Linker Warnings
- **Result:** **Zero (0) compiler warnings** across all 4 presets.
- **Evidence:** The `ci` preset enforces `-Werror` (and MSVC `/WX`), failing on any warning. The build completed with exit code 0.

### 7.2 Test Failures and Skips
- **Failures:** **Zero (0) test failures** across all unit and GUI test runs.
- **Skips:** **Zero (0) skipped or disabled tests**. Grepping for `DISABLED_`, `QSKIP`, or preprocessor exclusions `#if` in `tests/` yielded zero occurrences.

### 7.3 Runtime Diagnostic Warnings Captured During Testing
Executing `bitforge_gui_tests.exe` with verbose diagnostic output emitted the following expected runtime notices:

1. **Qt Offscreen Platform Notices:**
   `QWARN : ... This plugin does not support propagateSizeHints()`
   `QWARN : ... This plugin does not support raise()`
   - *Cause:* Standard behavior of Qt's `offscreen` QPA platform plugin when dock widgets and layouts recalculate geometry in headless mode. These are not defects.
2. **Intentional Negative Font Point Size Verification:**
   `QWARN : GuiTest::testNoNegativePointSizeWarningOnFaultOrStartup() QFont::setPointSize: Point size <= 0 (-1), must be greater than 0`
   - *Cause:* Test lines 3553–3557 explicitly invoke `test_font.setPointSize(-1)` to verify that the active diagnostic message interceptor successfully captures font warnings without crash or data loss.

### 7.4 Blocked Checks in Phase 0
- **Native Display Server & High-DPI Scaling:** All GUI automated testing executes offscreen (`-platform offscreen`). Physical window manager compositing, multi-monitor dragging, fractional DPI scaling (125%, 150%, 200%), and hardware font rendering cannot be verified offscreen and must be investigated in Phase 8 and Phase 9.
- **Interactive Hardware Mouse Dragging:** Manual mouse selection across hex grid rows in `MemoryViewer` and splitter movement between docks were tested via synthetic QtTest events, but physical pointer ergonomics remain unverified.

---

## 8. Source-to-Test Coverage Matrix

| Area | Existing Tests | Tested Behaviours | Known Gaps | Initial Risk |
|:---|:---|:---|:---|:---:|
| **CPU Core & ALU** | `test_cpu_instructions.cpp`<br>`test_alu.cpp`<br>`test_types.cpp` | 31 opcodes, ALU arithmetic/logic/flags, carry preservation in INC/DEC, modulo 65,536 PC/IX wrapping, fault atomicity, state transitions. | 4-byte instruction wraparound across `0xFFFF/0x0000`; self-modifying code execution; chained flag state transitions under deep loops. | **High** |
| **Memory & Bus** | `test_memory.cpp`<br>`test_bus.cpp`<br>`test_stack_instructions.cpp` | 64 KiB read/write, big-endian word layout, word wraparound at 0xFFFF, segment load safety, stack push/pop/call/ret bounds, I/O permissions. | `POP` and `RET` lower stack bound checks if `SP < 0xF000`; memory write-event journaling memory bounds under long CLI runs. | **High** |
| **Assembler & Loader** | `test_assembler.cpp` | 2-pass assembly, directives (`.ORG`, `.ENTRY`, `.BYTE`, `.WORD`, `.ASCII`), symbol table, label forward references, overlapping segments rejection, stack collisions. | Omitting `.ENTRY` on non-zero `.ORG` programs defaults silently to `0x0000`; syntax adversarial fuzzing (deeply nested or malformed tokens); multiple labels on same address. | **Medium** |
| **Virtual I/O & CLI** | `test_io_instructions.cpp`<br>`test_cli.cpp` | Input FIFO pop/underflow, Text console ASCII/escapes, Numeric console decimal formatting, LED state, invalid port faults, CLI subcommands and exit codes. | CLI provides no command-line argument to preload the Input FIFO (Port 0x00); CLI write-event memory consumption during 10M+ steps. | **Medium** |
| **Debugger Engine** | `test_debugger.cpp` | Single step, breakpoints, continue bypass, source mapping, delta generation, bounded trace history, fault atomicity deltas, register/memory inspection. | Breakpoint placed on an operand byte rather than an instruction opcode; debugger reset when program image is absent; large memory inspection range queries. | **Medium** |
| **Editor & Document Lifecycle** | `test_gui.cpp` (Tabs tests) | Multi-document tabs, untitled tab generation, dirty state tracking, save/save-as/reload cycles, unsaved changes confirmation dialog branches, tab path canonicalization. | File modification on disk outside BitForge while open in editor; rapid concurrent tab closure; very large files (100k+ lines) syntax highlighting latency. | **Medium** |
| **GUI Panels & Widgets** | `test_gui.cpp` (Panels tests) | Registers, BitView, MemoryViewer, StackViewer, DevicesPanel, 7-Segment display, Breakpoints, Trace, WatchPanel, Welcome dashboard, Tutorials. | WatchPanel does not resolve assembly symbols/labels; physical High-DPI font clipping; rapid start/pause/reset GUI button hammering. | **High** |
| **Design System & Theme** | `test_gui.cpp` (Theme tests) | Reconciled tokens: radii scale (`kRadiusNone`..`kRadiusPill`), control metrics (28px vs 26px toolbar), button styles, badge styles, contrast ratios. | System dark/light theme switching during runtime (dynamic palette update); contrast on custom OS high-contrast themes. | **Low** |
| **Examples & Curriculum** | `test_example_library.cpp`<br>`test_gui.cpp` | Execution of all 51 assembly examples, Bubble Sort ADS algorithm verification, guided tour walkthrough, lesson catalog loading. | Example error handling when user modifies example in-place and tries to revert; instructional accuracy of lesson text vs architecture spec. | **Low** |
| **Packaging & Distribution**| `package_windows.ps1 -Validate` | Release binary staging, windeployqt deployment, runtime DLL gathering, ZIP archiving, sanitized PATH smoke verification. | Missing DLLs on clean Windows installations without recent VC/UCRT runtimes; lack of Linux/macOS packaging scripts. | **Medium** |

---

## 9. Preliminary Specification-to-Test Mapping

The following table maps mandatory architectural requirements from `BitForge_Architecture_Specification_v1.7.md` and `ADR-001` to existing test coverage, identifying areas slated for Phase 1 and subsequent deep verification:

| Specification Requirement | Spec Section | Architectural Rule | Current Test Coverage | Conformance Status | Phase Slated for Deep Verification |
|:---|:---|:---|:---|:---:|:---:|
| **31 Canonical Instructions** | §6.1 | Exactly 31 defined mnemonics across 4 functional groups. | `test_types.cpp`, `test_cpu_instructions.cpp`, `test_assembler.cpp` | **Covered** | Phase 2 |
| **Flag Semantics (Z, C, N, V)** | §6.2 | Z (zero), C (carry/unsigned borrow), N (sign bit), V (two's complement overflow). INC/DEC preserve C. | `test_alu.cpp`, `test_cpu_instructions.cpp` | **Covered** | Phase 2 |
| **Memory Dimensions & Wraparound** | §5.1, §5.4 | 64 KiB address space (`0x0000..0xFFFF`), addresses wrap modulo 65,536 without exceptions. | `test_memory.cpp`, `test_bus.cpp`, `test_cpu_instructions.cpp` | **Covered** (4-byte instruction wrap gap) | Phase 3 |
| **Stack Bounds & ADR-001 Convention**| §5.5, ADR-001 | Empty sentinel `SP = 0xFFFF`; valid storage `0xF000..0xFFFE`. PUSH decrements before write; POP reads before increment. CALL/RET 2-byte big-endian. | `test_stack_instructions.cpp`, `test_debugger.cpp` | **Covered** (`SP < 0xF000` gap on POP) | Phase 3 |
| **CPU Fault Model & Atomicity** | §9.1–§9.3 | Any fault halts in `MachineState::Faulted`, preserving starting `PC`, registers, flags, memory, and devices. | `test_cpu_instructions.cpp`, `test_debugger.cpp`, `test_io_instructions.cpp` | **Covered** | Phase 1 & Phase 2 |
| **Assembler 2-Pass & Directives** | §8.1–§8.3 | Directives `.ORG`, `.ENTRY`, `.BYTE`, `.WORD`, `.ASCII`. Rejection of emission at `>= 0xF000`. | `test_assembler.cpp` | **Covered** (unspecified `.ENTRY` gap) | Phase 4 |
| **Virtual I/O Port Contracts** | §7.1, §7.2 | Port 0x00 (FIFO input), 0x01 (Text), 0x02 (Numeric), 0x10 (LEDs). Empty FIFO returns 0x00 without faulting. Side-effect-free queries. | `test_bus.cpp`, `test_io_instructions.cpp` | **Covered** | Phase 5 |
| **CLI Exit Codes & Usage** | §10.3 | Exit codes: 0 (Success), 1 (Usage error), 2 (Source/assembly error), 3 (CPU fault), 4 (Budget exceeded). | `test_cli.cpp` | **Covered** | Phase 5 |
| **Debugger Observability** | §12.1 | Single-stepping, breakpoints, bounded trace history, execution delta snapshots, instruction budget enforcement. | `test_debugger.cpp` | **Covered** | Phase 6 |
| **Multi-Document Editor Lifecycle** | GUI Spec | Independent document tabs, dirty state indicators, save/discard/cancel prompts, execution decoupled from active tab. | `test_gui.cpp` | **Covered** | Phase 7 & Phase 8 |
| **ADS Bubble Sort Verification** | §1.1, §12.3 | Signature demonstration sorting 8-element array at `0x8000` within 100,000 steps. | `test_example_library.cpp` | **Covered** | Phase 10 |
| **Portable Distribution Independence**| §11.4 | Standalone package operates under sanitized PATH without development tools. | `package_windows.ps1 -Validate` | **Covered** | Phase 11 |

---

## 10. Static Inspection Observations

Static analysis of the code base revealed several subtle implementation characteristics and potential risk indicators:

### Observation 1: 4-Byte Instruction Wraparound Straddling 64 KiB
- **File:** `src/core/cpu.cpp` (lines 71–95, 114–200)
- **Detail:** In `step()`, operands are fetched using `cursor = static_cast<word_t>((cursor + 1) & 0xFFFF)`. For a 4-byte instruction (`LD Rd, [addr16]` or `ST [addr16], Rs`) located at address `0xFFFD`, `0xFFFE`, or `0xFFFF`, the operand bytes straddle across the 64 KiB boundary into `0x0000`, `0x0001`, and `0x0002`. While the transient cursor arithmetic is mathematically bounded modulo 65,536, `MultibyteInstructionStraddling64KBoundary` in `test_cpu_instructions.cpp` only tests a 3-byte `LDI` instruction starting at `0xFFFE`. 4-byte straddling must be specifically verified in Phase 2.

### Observation 2: Stack Lower Boundary Check on POP and RET
- **File:** `src/core/cpu.cpp` (lines 427–438, 460–472)
- **Detail:** `PUSH` strictly requires `registers_.sp > kStackReservedStart` (`0xF000`). However, `POP` only checks `registers_.sp >= kStackResetValue` (`0xFFFF`), and `RET` only checks `registers_.sp > kStackResetValue - 2` (`0xFFFD`). If `SP` is externally modified or corrupted to an address below `0xF000` (e.g. `0x1000`), a subsequent `POP` or `RET` will read from that non-stack address without triggering a fault. Dedicated adversarial verification is required in Phase 3.

### Observation 3: Default Entry Address on Non-Zero .ORG Programs
- **File:** `src/assembler/assembler.cpp` (lines 100, 898–918)
- **Detail:** If an assembly file defines `.ORG 0x0200` but omits the `.ENTRY` directive, `pass2.entry_point_explicitly_set()` evaluates to `false`, and the program image defaults to `entry_point = 0x0000`. When executed, the CPU will execute uninitialized memory from `0x0000` up to `0x0200` (which consists of `0x00` NOPs if memory is reset). The assembler produces no warning for this gap unless `.ENTRY` is explicitly specified. Phase 4 will evaluate whether a warning should be emitted for implicit entry in uninitialized memory gaps.

### Observation 4: Unbounded Memory Write Journaling in SystemBus
- **File:** `src/bus/system_bus.cpp` (lines 15–34)
- **Detail:** Every call to `write_byte` and `write_word` unconditionally appends a `MemoryWriteEvent` to `memory_write_events_`. While `DebuggerSession::execute_single_step()` periodically clears write events, the CLI runner (`src/cli/runner.cpp`) never clears `memory_write_events_` during its execution loop. A program performing 10,000,000 memory writes in a loop will allocate hundreds of megabytes of event logs that are never consumed by the CLI. Phase 5 and Phase 12 will test memory stability under prolonged runs.

### Observation 5: Watch Panel Label Resolution Gap
- **File:** `src/gui/watch_panel.cpp` (lines 84–138)
- **Detail:** `WatchPanel::parseExpression` parses registers (`R0..R3`, `IX`, `PC`, `SP`), flags (`Z`, `C`, `N`, `V`), and numeric memory addresses (`[0x8000]`, `0x8000`). However, it does not query `ProgramImage::symbol_table()` to resolve variable or label names (e.g. `watch "count"`). Users attempting to watch a variable by label name will receive an unparsed expression error. Phase 6 and Phase 8 should assess symbolic watch resolution.

### Observation 6: Offscreen QPA Font Metric Warnings
- **File:** `tests/gui/test_gui.cpp` (lines 3476–3562)
- **Detail:** Under Qt's offscreen plugin, certain font metric inquiries can return default or uninitialized sizes. While Phase 12.4 successfully resolved negative font point size warnings on CPU faults, running GUI tests on platforms without standard system fonts could trigger font fallback warnings.

---

## 11. Ranked QA Backlog for Phases 1–13

The following risk-ranked backlog establishes the comprehensive agenda for the subsequent QA roadmap phases, prioritizing execution safety, CPU correctness, deterministic faults, memory integrity, and user data protection over cosmetic issues:

```
[Phase 0: Baseline] ===> [Phase 1: Spec Conformance] ===> [Phase 2: CPU Edge Cases]
                                                                    |
+-------------------------------------------------------------------+
|
v
[Phase 3: Memory & Stack] ===> [Phase 4: Assembler Adversarial] ===> [Phase 5: I/O & CLI]
                                                                              |
+-----------------------------------------------------------------------------+
|
v
[Phase 6: Debugger Engine] ===> [Phase 7: Editor Lifecycle] ===> [Phase 8: GUI Interaction]
                                                                          |
+-------------------------------------------------------------------------+
|
v
[Phase 9: Usability & Layout] ===> [Phase 10: Examples & ADS] ===> [Phase 11: Packaging]
                                                                             |
+----------------------------------------------------------------------------+
|
v
[Phase 12: Stress & Reliability] ===> [Phase 13: Final Regression & Release Sign-Off]
```

### Backlog Items by Phase

| Phase | Priority | Target Area | Concern / Behavior | Proposed Validation Approach | Prerequisite / Dependencies |
|:---:|:---:|:---|:---|:---|:---|
| **Phase 1** | **Critical** | Architectural Spec Conformance | Verify absolute adherence to v1.7 specification for all 31 opcodes, state transitions, and memory boundaries. | Systematic spec-to-source audit and bidirectional trace mapping against `v1.7.md`. | Phase 0 baseline complete. |
| **Phase 2** | **Critical** | CPU Instruction & Arithmetic Edge Cases | 4-byte instruction wraparound straddling 0xFFFF; signed overflow corner cases in ADD/SUB/INC/DEC; flag preservation sequences. | Synthesize direct-memory CPU execution fixtures placing 4-byte instructions at 0xFFFD..0xFFFF; test exhaustive 8-bit arithmetic permutations. | Phase 1 conformance audit. |
| **Phase 3** | **Critical** | Memory, Bus & Stack Integrity | Stack pointer lower boundary check on POP/RET (`SP < 0xF000`); fault atomicity on stack overflow/underflow; word read/write wrapping. | Unit tests forcing SP to 0x1000..0xEFFF; verify universal PC/register preservation across all stack fault triggers. | Phase 2 CPU verification. |
| **Phase 4** | **High** | Assembler Adversarial Testing | Omitted `.ENTRY` on non-zero `.ORG`; malformed literal prefixes (`0xGHI`, `0b12`); unterminated strings; duplicate labels; max symbol capacity. | Adversarial syntax fuzzing suite with randomized tokens, extreme line lengths, and boundary directives. | Phase 3 memory model verified. |
| **Phase 5** | **High** | Virtual I/O & CLI Behavior | Input FIFO underflow logging; unbounded memory write events during 10M-step runs; CLI exit codes under corrupted input files. | CLI runner memory profiling under high iteration budgets; automated FIFO poll verification. | Phase 4 assembler image verified. |
| **Phase 6** | **High** | CPU Execution & Debugger Observability | Breakpoint placement on operand bytes vs opcode address; trace ring buffer capacity limits; step delta accuracy under faults. | Debugger session tests placing breakpoints on multi-byte instruction offsets; circular trace overflow stress testing. | Phase 5 I/O contracts established. |
| **Phase 7** | **High** | Editor, Files & Multi-Document Lifecycle | Unsaved changes dialog branches (Save, Discard, Cancel); external file modification while open; unwritable file handling; clean closure. | Mock file I/O permissions; simulate dirty tab switching, concurrent file modifications, and close-all operations. | Phase 6 debugger session verified. |
| **Phase 8** | **Medium** | GUI Functionality & Interaction Consistency | Execution controller decoupling across tabs; watch panel symbol resolution; speed mode timer throttling; dock restoration. | QtTest simulation of rapid step/run/reset clicks; watch panel symbol table lookup testing. | Phase 7 editor lifecycle verified. |
| **Phase 9** | **Medium** | Typography, Usability & Responsive Layout | High-DPI font scaling (125%, 150%, 200%); minimum window resizing (1024x768); explicit tiered contrast (WCAG 2.1 AA text contrast 4.5:1/3:1 and 3:1 non-text contrast for visual information required to identify UI components and states against adjacent colors, selective AAA for primary actions); keyboard navigation. | Offscreen and on-screen window resize stress tests; font point size boundary testing across display metrics; contrast matrix audit against WCAG 2.1 AA criteria (evaluating functional boundaries while excluding decorative borders). | Phase 8 GUI interaction verified. |
| **Phase 10** | **Medium** | Examples, Lessons & Educational Correctness | Verification of all 51 assembly examples; Bubble Sort algorithm proof; tutorial guide lesson fidelity against architectural spec. | Automated execution of all catalog examples with strict step budget limits and post-execution memory assertions. | Phase 9 UI layout verified. |
| **Phase 11** | **High** | Packaging & Environment Independence | Standalone portable execution on pristine Windows without development tools; paths with spaces and special characters. | Verification of packaged distribution under sanitized environment on physical Windows host. | Phase 10 examples verified. |
| **Phase 12** | **Critical** | Stress, Performance & Reliability | Extended execution (10M+ steps); memory leak detection; editor syntax highlighter latency on 50,000-line sources. | Long-running headless benchmark runs; memory heap profiling; large source stress tests. | Phase 11 packaging verified. |
| **Phase 13** | **Critical** | Final Regression & Release Acceptance | Full 4-preset regression run; zero compiler warnings; 100% test pass; complete documentation and release sign-off. | Execute complete test matrix across `ci`, `debug`, `gui`, `gui-release`; verify clean Git working tree. | All Phases 1–12 complete. |

---

## 12. Known Environment Limitations

1. **Host PATH Configuration:** The MSYS2 UCRT64 toolchain directory (`C:\msys64\ucrt64\bin`) is not configured in the persistent host system `PATH`. All PowerShell commands must prepend this directory to avoid `command not found` errors.
2. **Headless Execution Environment:** Automated GUI tests run under the Qt `offscreen` platform plugin. True physical desktop interactions (DWM compositing, hardware DPI scaling, physical mouse drag-and-drop) cannot be validated within offscreen automated test runners and require manual verification during Phase 9.
3. **Coverage Tooling:** While `gcov.exe` is present in the MSYS2 UCRT64 distribution, `lcov`, `llvm-cov`, and `gcovr` are not installed on the host. Code coverage reports can be produced using `gcov` directly if coverage flags are enabled in future phases.
4. **Cross-Platform Release Artifacts:** While the core emulator and CLI are portable ISO C++17, packaging automation scripts are currently implemented exclusively for Windows PowerShell (`scripts/package_windows.ps1`). Linux and macOS packaging workflows are currently manual.

---

## 13. Exact Scope of Work Completed

During Phase 0, the following activities were performed:
1. Checked and recorded the exact repository state, active branch, HEAD commit hash, and remote tracking status.
2. Verified that the working tree was completely clean prior to and following all operations, with the Phase 0 report as the only intended new deliverable.
3. Inspected all build configuration files (`CMakeLists.txt`, `CMakePresets.json`, `launch_bitforge.bat`, `scripts/package_windows.ps1`).
4. Identified and verified all installed build tools (CMake 4.4.4, Ninja 1.13.2, GCC 16.2.0, Qt 6.11.2).
5. Mapped the complete project source structure (25,082 lines across `include/`, `src/`, `tests/`, `examples/`).
6. Documented the complete test inventory (233 unit tests, 81 GUI test methods + 2 fixture slots = 83 QtTest entries, 316 combined checks).
7. Reconciled historical test-count discrepancies (confirming continuous 233 core tests since Phase 10.2).
8. Built and executed all four established CMake presets (`ci`, `debug`, `gui`, `gui-release`), confirming 100% pass rates.
9. Executed portable Windows package generation and automated validation under sanitized PATH and spaced paths.
10. Performed a non-destructive static inspection of the CPU, Memory, Bus, Assembler, CLI, Debugger, and GUI sources.
11. Constructed a preliminary specification-to-test mapping against `v1.7.md` and `ADR-001`.
12. Formulated a ranked 13-phase QA backlog for the remaining roadmap.
13. Authored this Phase 0 Baseline Report at `docs/QA_PHASE0_BASELINE_REPORT.md`.

---

## 14. Integrity Affirmation

It is explicitly confirmed that:
1. **Zero application source files** in `src/` or `include/` were modified or reformatted.
2. **Zero test implementation files** in `tests/` were modified or disabled.
3. **Zero build configuration files** (`CMakeLists.txt`, `CMakePresets.json`) were altered.
4. **Zero frozen architectural specifications** (`docs/BitForge_Architecture_Specification_v1.0.md` through `v1.7.md` and `docs/decisions/ADR-001-stack-pointer-convention.md`) were changed.
5. **Zero pre-existing documentation or assembly example files** were modified, while acknowledging this newly authored baseline deliverable (`docs/QA_PHASE0_BASELINE_REPORT.md`) as the sole intended new file.
6. **No changes were committed or pushed to any remote repository.**
7. The only new file in the repository working tree is this baseline deliverable: `docs/QA_PHASE0_BASELINE_REPORT.md` (untracked, uncommitted, not pushed).
