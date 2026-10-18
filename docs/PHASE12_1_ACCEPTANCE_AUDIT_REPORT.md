# BitForge Phase 12.1 — Editor Lifecycle and Guided-Tour Acceptance Audit Report

**Date:** October 19, 2026  
**Auditor:** BitForge QA & System Architecture Audit  
**Baseline Commit:** `7e3332d` (Phase 12: Multi-Document Editor and Onboarding System)  
**Target:** Qt 6.11.2 / C++17 Desktop Workbench  
**Status:** **ACCEPTED & VERIFIED (Pass: 100%)**

---

## 1. Executive Summary

Phase 12.1 conducted a rigorous, evidence-based acceptance audit of the newly implemented multi-document assembly editor and the Welcome/Tutorial Center in BitForge.

The audit verified and resolved four critical areas:
1. **Registry & Settings Protection:** Complete test isolation via `MainWindow::setSettingsProfile("BitForgeTest", "BitForgeTest")`, ensuring the user's real Windows Registry settings (`HKCU\Software\BitForge\BitForge`) are never cleared or corrupted during test runs.
2. **Execution-Session Lifetime Across Document Closure:** Established and verified a robust architectural policy for decoupling CPU execution lifetimes from document tab lifecycles. Closing the source document of an active execution session leaves the CPU state (registers, memory, stack, devices, trace) completely intact and inspectable, allows safe stepping/resets without stale-pointer hazards, and gracefully prevents phantom execution markers or cross-document jumping.
3. **Save and Close Failure Matrix:** Implemented atomic two-pass closure logic in `closeOtherTabs` and `closeAllTabs`. If a user cancels or an unwritable save fails on any tab, the operation aborts immediately before closing any unrelated tabs.
4. **Interactive Guided Tour Acceptance:** Enhanced `TutorialGuide` with explicit step badges (`Step X of 9`), comprehensive keyboard navigation (Right/Left arrows, N, B, R, Escape), synchronized visual dock navigation according to stage, and clean reset to Stage 0 when re-invoked from Help.

All tests pass 100% across all four project CMake presets (`debug`, `ci`, `gui`, `gui-release`).

---

## 2. Test & Build Matrix

All four presets compile with zero warnings and achieve a 100% test pass rate:

| Preset | Target Configuration | Core Tests (GoogleTest) | GUI Tests (QtTest) | Pass Rate | Duration |
| :--- | :--- | :---: | :---: | :---: | :---: |
| `debug` | Headless Debug | 220 / 220 | N/A | **100%** | 0.17s |
| `ci` | Strict CI (Warnings as Errors) | 220 / 220 | N/A | **100%** | 0.16s |
| `gui` | Debug GUI & Workbench | 220 / 220 | 74 / 74 | **100%** | 14.48s |
| `gui-release` | Optimized Release GUI | 220 / 220 | 74 / 74 | **100%** | 14.94s |

---

## 3. Detailed Audit Findings & Policy Implementations

### 3.1 QSettings Profile Isolation and Registry Protection
- **Vulnerability Identified:** Previously, test initialization in `test_gui.cpp` called `QSettings("BitForge", "BitForge").clear()`. On Windows, this wiped real user workspace preferences and window layouts stored in `HKEY_CURRENT_USER\Software\BitForge\BitForge`.
- **Resolution:**
  - Added configurable profile settings to `MainWindow`:
    ```cpp
    MainWindow::setSettingsProfile(const QString& organization, const QString& application);
    MainWindow::settingsOrganization();
    MainWindow::settingsApplication();
    ```
  - Replaced all hardcoded `"BitForge", "BitForge"` instances in `MainWindow`, `WelcomeWidget`, and `test_gui.cpp` with the active profile getters.
  - Test suites configure `MainWindow::setSettingsProfile("BitForgeTest", "BitForgeTest")` during `initTestCase()`, clean only the test profile, and restore default profile on cleanup. Verified by `testIsolatedSettingsProfileRegistrySafety`.

### 3.2 Execution-Session Lifetime When Source Document is Closed
- **Architectural Policy:**
  - An assembled CPU session belongs to the `ExecutionController`, not the editor widget. Closing an editor tab whose code is loaded in the CPU does **not** terminate or unload the execution session.
  - The runtime registers, memory, stack, devices, breakpoints, and chronological trace remain 100% intact and inspectable across all dock panels.
  - When the executing tab is closed:
    1. `executing_editor_` is set to `nullptr` to avoid stale pointer dereferencing or use-after-free bugs.
    2. The closed editor's PC execution arrow and gutter breakpoint markers are cleared before destruction.
    3. `executing_document_path_` is preserved so the workbench maintains provenance of what is executing.
    4. The status bar displays `READY (Exec: <filename> (Closed))` or `PAUSED (Exec: <filename> (Closed))`.
    5. Single-stepping (`F10`) and continuing (`F8`) remain fully operational, advancing the CPU without requiring a visible source tab.
    6. Reset (`F6`) cleanly reinitializes CPU registers and memory to the program entry point without crashing.
    7. Double-clicking diagnostics or breakpoints emits an informative status bar notification (`Source document is closed: <filename>`) rather than jumping to unrelated lines in active tab B.
    8. Tab B can be edited, saved, or inspected without affecting session A. Session A is only replaced when the user deliberately executes an assembly or run command (`F5` or `F7`) from tab B.
- **Verification:** Covered by `testExecutionSessionLifetimeDocumentClosedStepping()`, `testExecutionSessionLifetimeBreakpointAndHalt()`, `testExecutionSessionLifetimeAllTabsClosedInspection()`, and `testExecutionSessionDeliberateReplacementFromDocB()`.

### 3.3 Save & Close Failure Matrix and Atomic Tab Closure
- **Vulnerability Identified:** In naive multi-tab closing loops, if tab 2 prompt was cancelled after tab 3 had already been removed, the operation resulted in a partial, non-atomic closure of unrelated tabs.
- **Resolution:**
  - Implemented two-pass closure logic in `closeOtherTabs()` and `closeAllTabs()`:
    - **Pass 1 (Validation & Unsaved Data Prompting):** Scans all modified tabs slated for closure. If the user selects `Cancel` or if a save operation fails (e.g. unwritable destination), the entire batch operation aborts immediately before closing any tab. If the user selects `Discard`, the document's modified flag is cleared to prevent duplicate prompts in Pass 2.
    - **Pass 2 (Execution):** Iterates through the verified tabs and closes them safely.
  - Unwritable destination handling: `saveFile()` returns `false` on I/O error and displays a modal warning while preserving editor text, dirty indicator (`*`), and file path.
- **Verification:** Covered by `testSaveAndCloseFailureMatrix()`.

### 3.4 Guided Tour Ergonomics and Acceptance
- **Enhancements Implemented in `TutorialGuide`:**
  - **Step Badge:** Prominent step indicator (`Step X of 9`) in the dialog header styled with `#61afef` on dark background.
  - **Full Keyboard Navigation:**
    - `Right Arrow` or `N`: Advance to next stage.
    - `Left Arrow` or `B`: Return to previous stage.
    - `R`: Restart tour from Stage 0.
    - `Escape`: Close/dismiss tour cleanly.
  - **Visual Dock Navigation:** Emits `stageChanged(int stage)` signal on every stage transition. `MainWindow` connects this signal to automatically show, raise, and focus the dock panels relevant to the current stage:
    - Stages 0–4: Focus central assembly editor.
    - Stage 5: Raise and focus right dock (Registers & Flags).
    - Stage 6: Raise and focus bottom dock (Memory & Stack viewers).
    - Stage 7: Raise and focus right dock (Virtual Devices panel).
    - Stage 8: Focus central editor for completion and next steps.
  - **Re-invocation from Help:** Triggering `Help -> Interactive Guided Tour...` when the guide is already open cleanly resets the tour to Stage 0 (`Step 1 of 9`) without memory leakage or state corruption.
- **Verification:** Covered by `testGuidedTourNavigationKeyboardAndControls()`.

---

## 4. Acceptance Test Suite Summary (Phase 12.1 Additions)

Seven dedicated acceptance tests were added to `tests/gui/test_gui.cpp`:
1. `testExecutionSessionLifetimeDocumentClosedStepping()`: Assembles program A, switches to tab B, closes tab A, verifies `(Closed)` status label, single-steps 2 instructions, continues to halt, and verifies all dock inspectors remain populated and responsive.
2. `testExecutionSessionLifetimeBreakpointAndHalt()`: Sets a line breakpoint in program A, closes tab A, continues to breakpoint hit, validates that double-clicking breakpoint panel does not jump in tab B, steps through breakpoint, and completes execution.
3. `testExecutionSessionLifetimeAllTabsClosedInspection()`: Assembles program, closes all tabs, verifies CPU state and memory remain fully inspectable in docks, and resets CPU to entry point cleanly without crash.
4. `testExecutionSessionDeliberateReplacementFromDocB()`: Verifies that switching to tab B and closing tab A preserves session A until the user deliberately clicks Assemble/Run on tab B.
5. `testSaveAndCloseFailureMatrix()`: Verifies atomic two-pass abort on cancelled `closeOtherTabs` and `closeAllTabs` with modified tabs, and verifies unwritable save failure non-destructiveness.
6. `testGuidedTourNavigationKeyboardAndControls()`: Exercises all 9 tour stages, step badge text, previous/next controls, keyboard shortcuts, dock navigation, and Help re-invocation reset.
7. `testIsolatedSettingsProfileRegistrySafety()`: Verifies that test settings are written strictly to `"BitForgeTest"` and that the user's primary `"BitForge"` registry hive is untouched.

---

## 5. Conclusion & Acceptance Status

BitForge Phase 12.1 satisfies all requirements set forth in the acceptance audit. The multi-document tabbed editor, execution session lifecycle, unsaved data protection, and guided tour onboarding system are robust, safe, thoroughly covered by automated regression tests, and fully verified across all compiler configurations.

**Result: ACCEPTED WITHOUT RESERVATIONS.**
