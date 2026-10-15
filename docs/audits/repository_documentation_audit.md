# BitForge — Repository-Wide Documentation Consistency Audit

**Audit Date:** 2026-10-16  
**Audited Commit / HEAD:** `2b40401 docs(audit): audit instruction encodings and virtual I/O port documentation`  
**Repository Working Directory:** `D:\BitForge`  
**Auditor:** Antigravity (Google DeepMind)  
**Status:** Complete — Pending User Review and Approval for Proposed Edits

---

## A. Executive Summary

A comprehensive repository-wide documentation consistency audit was performed on BitForge across all tracked documentation files, architecture specifications, ADRs, build configurations, launcher scripts, public headers, CLI help manuals, and assembly examples.

### Key Audit Findings:
1. **Core Architectural & ISA Documentation is High Quality and Consistent:**
   The 31-instruction canonical ISA, instruction encodings, operand formats, memory mapping (`0x0000`–`0xFFFF`), reserved stack boundaries (`0xF000`–`0xFFFE`, `SP=0xFFFF`), and virtual I/O port assignments (`0x00` FIFO, `0x01` text, `0x02` numeric, `0x10` LEDs; `0x03` unmapped) are consistently implemented in the core engine, assembler, CLI runner, debugger session, and Qt 6 GUI workbench.
2. **Flag Conventions in Certain Example Comments & Catalog Tables are Inconsistent:**
   Four arithmetic examples (`01_addition_subtraction.asm`, `04_comparison_flags.asm`, `05_carry_borrow_boundaries.asm`, and `06_signed_overflow_boundaries.asm`) contain discrepancies in their expected `C` (carry/no-borrow) flag comments or catalog entries in [`examples/README.md`](file:///d:/BitForge/examples/README.md). Per `BitForge_Architecture_Specification_v1.7.md` Section 6.2 and `Alu::sub`, BitForge follows the 6502/ARM convention where `C = (a >= b)`. Subtractions that require no borrow leave `C = 1`, and comparisons where left < right leave `C = 0`.
3. **Stale Project Status and Test Counts in Root `README.md`:**
   [`README.md`](file:///d:/BitForge/README.md) still indicates "Current Status: Phase 6 Complete" and reports "169 unit and regression tests plus 11 automated headless GUI integration tests". Following Phase 6.1 (GUI lifecycle audit), Phase 7 (51-program example library), and Phase 7.1 (ISA/device consistency audit), the test suite now comprises **220 unit tests** in `bitforge_tests` and **29 test methods** (31 QTest execution slots) in `bitforge_gui_tests`.
4. **Missing Windows Launcher Documentation in Primary Guides:**
   [`launch_bitforge.bat`](file:///d:/BitForge/launch_bitforge.bat) provides essential automated path setup for MSYS2 UCRT64 Qt runtime DLLs and one-click execution with drag-and-drop assembly file loading. While cataloged in [`examples/README.md`](file:///d:/BitForge/examples/README.md), it is completely omitted from the root [`README.md`](file:///d:/BitForge/README.md) and [`BUILDING.md`](file:///d:/BitForge/BUILDING.md).
5. **Historical Architectural Specification Discrepancies:**
   Section 13 of `BitForge_Architecture_Specification_v1.7.md` projected directory paths `include/bitforge/devices/` and `src/devices/`, and root examples `02_linear_search.asm`, `03_bubble_sort.asm`, `04_factorial.asm`. The implementation consolidated devices into `include/bitforge/bus/system_bus.hpp` and created a comprehensive 51-file categorized library under `examples/`. Because v1.7 is a frozen historical contract, it must not be edited, but the relationship is formally documented herein.

**Overall Assessment:** The project documentation is **mostly consistent with a few targeted corrections required**. No production source code, CPU emulation semantics, or architecture specifications require changes; only targeted documentation and example comment alignments are recommended.

---

## B. Repository Documentation Inventory

### Reviewed Files & Artifacts
| File / Directory | Type / Scope | Verification Method | Status |
| :--- | :--- | :--- | :--- |
| [`README.md`](file:///d:/BitForge/README.md) | Primary project overview, CLI manual, GUI overview, quickstart | Full textual inspection against implementation and presets | Inspected — 3 findings |
| [`BUILDING.md`](file:///d:/BitForge/BUILDING.md) | Platform setup, toolchain requirements, presets, build/test workflows | Full textual inspection against CMakeLists.txt and toolchains | Inspected — 2 findings |
| [`examples/README.md`](file:///d:/BitForge/examples/README.md) | 31-instruction matrix, example catalog, expected registers/flags/ports | Full textual inspection and cross-validation with tests | Inspected — 3 findings |
| [`docs/decisions/ADR-001-stack-pointer-convention.md`](file:///d:/BitForge/docs/decisions/ADR-001-stack-pointer-convention.md) | Architectural decision record for stack pointer semantics | Full textual inspection against `Cpu`, `Alu`, and tests | Inspected — 100% Consistent |
| `BitForge_Architecture_Specification_v1.0.md` – `v1.6.md` | Historical architecture specifications (v1.0 through v1.6) | Checked presence and preservation | Preserved without modification |
| `BitForge_Architecture_Specification_v1.7.md` | Active frozen architectural baseline contract | Cross-referenced against codebase APIs, types, and ISA | Inspected — 1 historical note |
| [`launch_bitforge.bat`](file:///d:/BitForge/launch_bitforge.bat) | Windows batch launcher for GUI workbench | Shell inspection, environment PATH analysis, argument handling | Inspected — 1 finding |
| [`CMakeLists.txt`](file:///d:/BitForge/CMakeLists.txt) | Root build definition, targets, compiler flags, Qt 6 integration | Code inspection against preset configurations and target outputs | Inspected — 100% Consistent |
| [`CMakePresets.json`](file:///d:/BitForge/CMakePresets.json) | Configure, build, and test presets (`debug`, `release`, `ci`, `gui`) | JSON schema and binary directory validation | Inspected — 100% Consistent |
| [`.github/workflows/ci.yml`](file:///d:/BitForge/.github/workflows/ci.yml) | Multi-platform CI workflow definition | Matrix, compiler, and step inspection | Inspected — 100% Consistent |
| `include/bitforge/**/*.hpp` (37 files) | Public API header comments, docstrings, and invariants | Inspected for API consistency, contracts, and comments | Inspected — 100% Consistent |
| `examples/**/*.asm` (51 files) | 51 assembly source files across 10 directories and root | Header comments, directives, instructions, and assertions | Inspected — 2 comment findings |
| `tests/**/*.cpp` (14 files) | Unit and GUI automated test suites | Test assertions, test names, and test counts | Inspected & Executed |

### Areas Not Inspected
- Private temporary sandbox directories outside Git control.
- Git objects inside `.git/` database internals (standard Git metadata only).

---

## C. Findings Table

| ID | Severity | Category | File and Location | Description & Evidence | Recommended Correction | Verification Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **DOC-001** | **Medium** | Stale Documentation | [`README.md:24`](file:///d:/BitForge/README.md#L24) | **Stale test counts:** Claims "169 unit and regression tests plus 11 automated headless GUI integration tests (`BitForgeGuiTests`)". Actual suite currently executes **220 unit tests** in `bitforge_tests` and **29 test methods** (31 QTest execution slots) in `bitforge_gui_tests`. | Update test metrics to 220 unit tests and 31 GUI test slots, or replace static counts with command reproduction guidance via CTest. | Confirmed via test execution. |
| **DOC-002** | **Low** | Stale Documentation | [`README.md:9`](file:///d:/BitForge/README.md#L9) | **Stale phase status:** Header states "Current Status: Phase 6 Complete (Qt 6 Desktop GUI Workbench)". Does not mention Phase 7 (Comprehensive Assembly Example Library) or Phase 7.1 (Consistency Audit). | Update header to reflect Phase 7 completion and add a bullet for the 51-file example library in Section 1. | Confirmed via git history and repo status. |
| **DOC-003** | **High** | Wrong Technical Fact | [`examples/arithmetic/01_addition_subtraction.asm:13`](file:///d:/BitForge/examples/arithmetic/01_addition_subtraction.asm#L13), [`examples/README.md:110`](file:///d:/BitForge/examples/README.md#L110) | **Carry flag value on subtraction without borrow:** Expected flags documented as `[Z=0, C=0, N=0, V=0]` after `(50 + 25) - 15 = 60`. Per v1.7 Section 6.2 and `Alu::sub`, `C = (a >= b)`. Since $75 \ge 15$, `C = 1` (no borrow). `test_example_library.cpp:108` asserts `EXPECT_TRUE(ctx.session->flags().c)`. | Correct expected flags to `[Z=0, C=1, N=0, V=0]` in both `01_addition_subtraction.asm` and `examples/README.md`. | Confirmed via ALU code and unit test. |
| **DOC-004** | **High** | Wrong Technical Fact | [`examples/README.md:113`](file:///d:/BitForge/examples/README.md#L113) | **Borrow flag value on comparison in catalog:** Catalog table lists expected flags for `04_comparison_flags.asm` as `Z=0 C=1 N=1 V=0`. But program tests $10 < 20$, which requires an unsigned borrow, setting `C = 0`. The example source comment correctly states `Flags: [Z=0, C=0, N=1, V=0]`, and `test_example_library.cpp:149` asserts `EXPECT_FALSE(ctx.session->flags().c)`. | Correct catalog table entry in `examples/README.md` to `Z=0 C=0 N=1 V=0`. | Confirmed via example source and unit test. |
| **DOC-005** | **High** | Wrong Technical Fact | [`examples/README.md:114`](file:///d:/BitForge/examples/README.md#L114) | **Borrow flag value on borrow boundary in catalog:** Catalog table lists expected flags for `05_carry_borrow_boundaries.asm` as `Z=0 C=1 N=1 V=0`. The final operation in the program is `4 - 6 = -2 (0xFE)`, where $4 < 6$, so borrow requires `C = 0`. Example source comment correctly states `C=0`, and `test_example_library.cpp:162` asserts `EXPECT_FALSE(ctx.session->flags().c)`. | Correct catalog table entry in `examples/README.md` to `Z=0 C=0 N=1 V=0`. | Confirmed via example source and unit test. |
| **DOC-006** | **High** | Wrong Technical Fact | [`examples/arithmetic/06_signed_overflow_boundaries.asm:15`](file:///d:/BitForge/examples/arithmetic/06_signed_overflow_boundaries.asm#L15), [`examples/README.md:115`](file:///d:/BitForge/examples/README.md#L115) | **Carry flag value on signed overflow subtraction:** Header comment and catalog table record expected flags as `[Z=0, C=0, N=0, V=1]`. The final operation is `0x80 (-128) - 1 = 0x7F (+127)`. In unsigned terms, $128 \ge 1$, so no borrow occurs and `C = 1`. `V = 1` from signed underflow. Actual final flags are `[Z=0, C=1, N=0, V=1]`. | Correct expected flags to `[Z=0, C=1, N=0, V=1]` in `06_signed_overflow_boundaries.asm` and `examples/README.md`. | Confirmed via ALU code and execution trace. |
| **DOC-007** | **Medium** | Missing Information | [`README.md:147-162`](file:///d:/BitForge/README.md#L147-L162), [`BUILDING.md:211-223`](file:///d:/BitForge/BUILDING.md#L211-L223) | **Omission of Windows launcher script:** Neither `README.md` nor `BUILDING.md` documents `launch_bitforge.bat`. Directly launching `.\build\gui\bitforge_gui.exe` without MSYS2 UCRT64 Qt runtime DLLs on PATH fails on Windows. `launch_bitforge.bat` resolves this automatically. | Document `launch_bitforge.bat` in `README.md` (GUI launch) and `BUILDING.md` Section 6.3 for Windows users. | Confirmed via launcher code inspection. |
| **DOC-008** | **Low** | Build Instructions / Consistency | [`README.md:83, 86`](file:///d:/BitForge/README.md#L83), [`examples/README.md:89, 92`](file:///d:/BitForge/examples/README.md#L89) | **Inconsistent CLI path assumption:** CLI examples use `.\build\gui\bitforge.exe run ...`, which exists only if the GUI preset was built. If the user followed the default headless debug workflow (`cmake --preset debug`), the binary is located at `.\build\debug\bitforge.exe` (as in `BUILDING.md:166-168`). | Note both preset paths or standardize on `.\build\debug\bitforge.exe` for headless CLI examples. | Confirmed via `CMakePresets.json`. |
| **DOC-009** | **Informational** | Architectural Discrepancy | `BitForge_Architecture_Specification_v1.7.md:799, 806, 816-819` | **Historical specification layout divergence:** Section 13 projected `include/bitforge/devices/`, `src/devices/`, and root examples `02_linear_search.asm`, `03_bubble_sort.asm`, `04_factorial.asm`. The implementation integrated devices into `bus/system_bus` and built a comprehensive 51-file library under `examples/`. | Do not modify v1.7 (frozen historical specification). Document this architectural evolution in this report. | Confirmed via filesystem inspection. |

---

## D. Consistency Matrix

| Topic / Area | `README.md` | `BUILDING.md` | `examples/README.md` | `ADR-001` | Architecture Spec v1.7 | Implementation & Tests | Status |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **31 Canonical Mnemonics** | 31 | 31 | 31 | 31 | 31 | 31 | **Consistent** |
| **Opcode Byte Encodings** | Implicit | N/A | Exact Grouped | Exact Grouped | Exact Grouped | Exact Grouped | **Consistent** |
| **Virtual Ports (0x00, 0x01, 0x02, 0x10)** | Documented | N/A | Documented | N/A | Documented | Implemented & Tested | **Consistent** |
| **Port 0x03 Unmapped Status** | Implicit | N/A | Documented | N/A | Documented | Reject Tested | **Consistent** |
| **Stack Bounds (`0xF000`–`0xFFFE`, `SP=0xFFFF`)** | Documented | N/A | Documented | Documented | Documented | Implemented & Tested | **Consistent** |
| **Borrow Flag (`C = (a >= b)`)** | N/A | N/A | Partial Typo (DOC-003..006) | N/A | Documented | Implemented & Tested | **Typo in Examples** |
| **CMake Preset Names (`debug`, `ci`, `gui`)** | Consistent | Consistent | Consistent | N/A | Proposed CI | Implemented & Tested | **Consistent** |
| **GUI Shortcuts (F5, F6, F7, F8, F10)** | Consistent | N/A | Consistent | N/A | N/A | Implemented & Tested | **Consistent** |
| **Windows Launcher (`launch_bitforge.bat`)** | Missing | Missing | Documented | N/A | N/A | Implemented & Tested | **Missing in Docs** |
| **Current Test Metrics** | Stale (169+11) | Generic | N/A | N/A | N/A | Actual (220+31) | **Stale in README** |

---

## E. Build and Test Verification Results

All tests and presets were executed during this audit using the established local MSYS2 UCRT64 toolchain:
`$env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH`

### Execution Summary
| Preset / Command | Configuration | Output / Status | Test Duration | Verdict |
| :--- | :--- | :--- | :---: | :--- |
| `cmake --build --preset debug` | Debug profile, assertions enabled | `ninja: no work to do.` (Exit Code 0) | — | **PASS** |
| `ctest --preset debug --output-on-failure` | CTest Debug suite (`BitForgeTests`) | `1/1 Test #1: BitForgeTests ... Passed` (220 tests passed, 0 failed) | 0.07s | **PASS** |
| `cmake --build --preset ci` | Strict CI profile, `-Werror` / `/WX` | `ninja: no work to do.` (Exit Code 0) | — | **PASS** |
| `ctest --preset ci --output-on-failure` | CTest CI suite (`BitForgeTests`) | `1/1 Test #1: BitForgeTests ... Passed` (220 tests passed, 0 failed) | 0.06s | **PASS** |
| `cmake --build --preset gui` | Qt 6 desktop GUI + GUI test targets | `ninja: no work to do.` (Exit Code 0) | — | **PASS** |
| `ctest --preset gui --output-on-failure` | Core + Qt 6 headless tests | `2/2 tests passed` (220 unit + 31 GUI test slots passed) | 3.04s | **PASS** |

### Environment Limitations
- Automated GUI tests (`BitForgeGuiTests`) run under `-platform offscreen` in headless console sessions. Native visual rendering on desktop requires interactive user launch via `launch_bitforge.bat`.

---

## F. Suggested Repair Sequence

Upon user authorization, the findings should be repaired in the following logical sequence:

### Group 1: Incorrect Technical Information (High Priority)
1. In [`examples/arithmetic/01_addition_subtraction.asm:13`](file:///d:/BitForge/examples/arithmetic/01_addition_subtraction.asm#L13) and [`examples/README.md:110`](file:///d:/BitForge/examples/README.md#L110), update expected flags from `[Z=0, C=0, N=0, V=0]` to `[Z=0, C=1, N=0, V=0]`.
2. In [`examples/README.md:113`](file:///d:/BitForge/examples/README.md#L113) (`04_comparison_flags.asm`), update expected flags from `Z=0 C=1 N=1 V=0` to `Z=0 C=0 N=1 V=0`.
3. In [`examples/README.md:114`](file:///d:/BitForge/examples/README.md#L114) (`05_carry_borrow_boundaries.asm`), update expected flags from `Z=0 C=1 N=1 V=0` to `Z=0 C=0 N=1 V=0`.
4. In [`examples/arithmetic/06_signed_overflow_boundaries.asm:15`](file:///d:/BitForge/examples/arithmetic/06_signed_overflow_boundaries.asm#L15) and [`examples/README.md:115`](file:///d:/BitForge/examples/README.md#L115), update expected flags from `[Z=0, C=0, N=0, V=1]` to `[Z=0, C=1, N=0, V=1]`.

### Group 2: Broken / Incomplete Launch Instructions (Medium Priority)
1. In [`README.md:159-161`](file:///d:/BitForge/README.md#L159-L161) and [`BUILDING.md:211-223`](file:///d:/BitForge/BUILDING.md#L211-L223), add instructions explaining `launch_bitforge.bat` on Windows (including automatic PATH setup and drag-and-drop file support).
2. In [`README.md:83, 86`](file:///d:/BitForge/README.md#L83) and [`examples/README.md:89, 92`](file:///d:/BitForge/examples/README.md#L89), clarify CLI binary paths under `build/debug/bitforge.exe` versus `build/gui/bitforge.exe`.

### Group 3: Stale Project Status & Test Metrics (Low Priority)
1. In [`README.md:9`](file:///d:/BitForge/README.md#L9), update status to reflect Phase 7 completion and the 51-example library.
2. In [`README.md:24`](file:///d:/BitForge/README.md#L24), update test metrics to 220 unit tests and 31 GUI test slots (or provide reproducible CTest guidance).

---

## G. Final Assessment

**Rating:** **Mostly consistent with a few targeted corrections required.**

The core architecture, engine code, CMake presets, CI workflows, and public C++ APIs are in excellent alignment with the frozen specification `BitForge_Architecture_Specification_v1.7.md`. No architectural redesign or code fixes are needed. The recommended repairs are strictly confined to correcting flag values in four example comments/catalog rows, documenting the Windows batch launcher in the primary guides, and updating stale test metrics in `README.md`.
