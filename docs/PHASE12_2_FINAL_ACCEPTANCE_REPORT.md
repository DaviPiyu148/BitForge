# BitForge Phase 12.2 — Final Acceptance Report
**Tutorial Scrolling Redesign, Layout Restoration Fix, and Example Library Audit**

**Date:** 2026-10-19  
**Branch:** `master`  
**Baseline Commit:** `b0018c0`  
**Validation Status:** Passed 100% (306/306 automated checks, all 4 CMake presets verified)

---

## 1. Executive Summary

Phase 12.2 resolves two primary user-facing defects identified in the BitForge workbench and validates the complete 51-file example library:

1. **Curriculum Lesson Scrolling & Visual Continuity (`WelcomeWidget`):**
   - Eliminated the recessed "embedded box viewer" appearance in the Curriculum tab by removing internal background colors, nested borders, and redundant frame margins.
   - Applied a custom 8px dark scrollbar (`#1b1d23` track, `#3e4451` rounded thumb, `#4ec9b0` hover, `#528bff` active) matching the BitForge design system.
   - Enforced rich typography and compact table layout via `document()->setDefaultStyleSheet(...)` preventing table clipping.
   - Established the required scroll policy: switching lessons immediately resets scroll position to the top (`setValue(0)`), while scrolling within an open lesson is preserved.
   - Anchored the bottom example action button (`open_lesson_example_btn_`) in a dedicated `#lessonActionBar` frame separated by a subtle top border.

2. **Window Shrinking Bug on "Restore Default Layout" (`MainWindow`):**
   - Discovered and eliminated the unconditional `resize(1240, 820);` in `MainWindow::onRestoreDefaultLayout()`.
   - Replaced it with dock split balancing using `resizeDocks(...)` that respects the active window geometry and maximized window state.
   - Preserved custom window dimensions (e.g., 1400×900, 1600×1000, 4K displays) and maximized states while properly docking, unfloating, and tabifying panels.
   - Added a safe minimum clamp (900×600) only for non-maximized undersized windows.

3. **Curated Example Library & Regression Audit (51 Fixtures):**
   - Confirmed 1:1 exact set equality between the 51 physical `.asm` files on disk and the compile-time catalog `kExampleCatalog` in `include/bitforge/core/example_catalog.hpp`.
   - Verified 100% automated test execution across all 51 fixtures (27 Runnable, 6 DebuggerDemo, 4 RuntimeFault, 14 InvalidAssembly/Syntax Rejection).

---

## 2. Defect 1: Curriculum Lesson Scrolling & Visual Continuity

### 2.1 Problem Analysis
In earlier revisions, the Lesson Details area in the Welcome Curriculum tab suffered from several visual defects:
- An inset background (`#1b1d23`) with an explicit 1px border (`#282c34`) and heavy padding created a confusing "viewer inside a box" look inside `QGroupBox("Lesson Details")`.
- Default platform scrollbars displayed prominent gray tracks and arrow buttons that clashed with the dark theme.
- Lesson 1 tables (`CPU Execution Lifecycle`) overflowed vertically due to unconstrained table cell styling, clipping bottom rows.
- Switching between lessons did not reset the vertical scrollbar, causing long lessons to open midway through the content.

### 2.2 Implemented Design & Architecture
- **Seamless Document Viewer:** In `src/gui/welcome_widget.cpp`, `lesson_content_` is styled with `background-color: transparent; border: none; padding: 0px 4px 2px 0px;`.
- **Custom Themed Scrollbar:** Applied slim 8px vertical scrollbar styling:
  ```css
  QScrollBar:vertical {
    border: none;
    background: #1b1d23;
    width: 8px;
    margin: 0px;
    border-radius: 4px;
  }
  QScrollBar::handle:vertical {
    background: #3e4451;
    min-height: 24px;
    border-radius: 4px;
  }
  QScrollBar::handle:vertical:hover { background: #4ec9b0; }
  QScrollBar::handle:vertical:pressed { background: #528bff; }
  QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; background: none; }
  ```
- **Document Default Typography & Tables:** Configured `lesson_content_->document()->setDefaultStyleSheet(...)` with compact typography, clean headers (`#4ec9b0` H3, `#e5c07b` H4), `border-collapse: collapse;` tables, and monospace syntax blocks.
- **Scroll Reset Policy:** In `WelcomeWidget::onLessonSelected(int row)`:
  ```cpp
  if (lesson_content_->verticalScrollBar()) {
      lesson_content_->verticalScrollBar()->setValue(0);
  }
  ```
- **Anchored Action Bar:** Wrapped `open_lesson_example_btn_` in `lesson_action_bar_` (`#lessonActionBar`) with a top separator border (`1px solid #282c34`). Hides atomically when a lesson has no associated example (e.g., Lesson 11).

---

## 3. Defect 2: "Restore Default Layout" Window Shrinking

### 3.1 Problem Analysis
When users selected **View → Restore Default Layout** (or pressed its shortcut), a maximized window or a custom-sized window (e.g., 1600×1000) unexpectedly collapsed and shrank to 1240×820.
- **Root Cause:** Line 1748 of `src/gui/main_window.cpp` contained an explicit, unconditional call: `resize(1240, 820);`.

### 3.2 Implemented Solution
- Removed unconditional `resize(1240, 820);`.
- Replaced with proportional dock sizing via `resizeDocks(...)`:
  ```cpp
  if (right_dock_ != nullptr) {
      int target_right_w = std::max(320, width() / 4);
      resizeDocks({right_dock_}, {target_right_w}, Qt::Horizontal);
  }
  if (bottom_dock_ != nullptr) {
      int target_bottom_h = std::max(220, height() / 3);
      resizeDocks({bottom_dock_}, {target_bottom_h}, Qt::Vertical);
  }
  ```
- Preserved current window geometry and maximized state:
  ```cpp
  if (!isMaximized() && (width() < 900 || height() < 600)) {
      resize(std::max(width(), 900), std::max(height(), 600));
  }
  ```
- Guaranteed dock unfloating, right and bottom area assignment, tabification with `watch_dock_`, and raising `bottom_dock_`.

---

## 4. Comprehensive Example Library Verification (51 Fixtures)

### 4.1 Disk Inventory vs. Canonical Catalog Parity
A strict set-equality check was performed between all `.asm` files on disk and the compile-time catalog in `include/bitforge/core/example_catalog.hpp`:
- **Physical `.asm` files on disk:** Exactly 51 files.
- **Entries in `kExampleCatalog`:** Exactly 51 entries.
- **Set difference:** $\emptyset$ (0 missing, 0 extra).

### 4.2 Fixture Breakdown & Execution Results
| Category | Directory | Count | Kind | Verification Status |
| :--- | :--- | :---: | :--- | :---: |
| **Quick Starts** | `examples/` | 2 | Runnable | 2 / 2 Halted cleanly |
| **Fundamentals** | `examples/fundamentals/` | 2 | Runnable | 2 / 2 Halted cleanly |
| **Arithmetic & Logic** | `examples/arithmetic/` | 6 | Runnable | 6 / 6 Halted cleanly |
| **Control Flow** | `examples/control_flow/` | 4 | Runnable | 4 / 4 Halted cleanly |
| **Memory & Pointers** | `examples/memory/` | 4 | Runnable | 4 / 4 Halted cleanly |
| **Stack & Subroutines** | `examples/stack_subroutines/` | 4 | Runnable | 4 / 4 Halted cleanly |
| **Virtual I/O Devices** | `examples/io_devices/` | 4 | Runnable | 4 / 4 Halted cleanly |
| **Algorithms** | `examples/algorithms/` | 5 | Runnable | 5 / 5 Halted cleanly |
| **Debugger Demos** | `examples/debugger/` | 6 | DebuggerDemo | 6 / 6 Verified (stepping/breakpoints/budget) |
| **Runtime Faults** | `examples/faults/` | 4 | RuntimeFault | 4 / 4 Triggered expected fault & atomicity |
| **Invalid Assembly** | `examples/invalid/` | 14 | InvalidAssembly | 14 / 14 Diagnostics correctly rejected |
| **Total** | | **51** | | **51 / 51 Passed (100%)** |

---

## 5. Test Suite Reconciliation & Multi-Preset Verification

### 5.1 Test Inventory
- **Core Unit Tests (`bitforge_tests.exe`):** 233 passed, 0 failed (100%).
- **GUI Tests (`bitforge_gui_tests.exe`):** 73 passed, 0 failed (100%).
- **Grand Total:** **306 tests across the entire BitForge suite**.

### 5.2 Multi-Preset Build and CTest Results
All four official CMake presets were built with Ninja on MSYS2 UCRT64 GCC 16.2.0 and passed all test suites:

| Preset | Target Binary | Build Status | CTest Result | Total Time |
| :--- | :--- | :---: | :---: | :---: |
| `debug` | `bitforge_tests.exe` | Clean | 1/1 Test Passed (233 unit tests) | 0.17 s |
| `ci` | `bitforge_tests.exe` | Clean | 1/1 Test Passed (233 unit tests) | 0.16 s |
| `gui` | `bitforge_gui.exe`, `bitforge_gui_tests.exe` | Clean | 2/2 Tests Passed (233 unit + 73 GUI) | 14.19 s |
| `gui-release` | `bitforge_gui.exe`, `bitforge_gui_tests.exe` | Clean | 2/2 Tests Passed (233 unit + 73 GUI) | 13.43 s |

### 5.3 Dedicated Phase 12.2 Test Cases
1. `testCurriculumLessonScrollingAndResetPolicy()`:
   - Verifies transparent styling and absence of nested borders on `lessonContent()`.
   - Validates custom 8px dark scrollbar stylesheet presence.
   - Verifies scrolling down within Lesson 0 is preserved during user reading.
   - Verifies switching to Lesson 1 immediately resets scroll position to 0.
   - Verifies Lesson 11 hides the action bar cleanly when no example file is attached.
   - Verifies switching back to Lesson 0 restores the action button and resets scroll position to 0.

2. `testRestoreDefaultLayoutPreservesWindowGeometryAndState()`:
   - Verifies maximized window remains maximized after restoring layout (no shrinking to 1240×820).
   - Verifies custom window geometry (e.g. 1500×960) is preserved exactly after restoring layout.
   - Verifies floating or hidden docks (`right_dock_`, `bottom_dock_`, `watch_dock_`) are re-docked, shown, unfloated, and tabified.
   - Verifies minimum size clamping (900×600) only activates when window dimensions are below threshold.

---

## 6. Visual Evidence Summary

The following rendered offscreen smoke test captures were generated and verified:
- `gui_smoke_welcome_curriculum.png`: Full window view of the Curriculum tab showing seamless integration of Lesson Details, clean 8px scrollbar, and anchored action bar.
- `gui_smoke_curriculum_lesson_details.png`: Isolated capture of the redesigned lesson details panel confirming typography, inline code badges, and absence of clipping.
- `gui_smoke_restored_layout_normal.png`: Workbench layout restored at custom geometry (1400×900) showing dock split proportions and status bar confirmation.
- `gui_smoke_restored_layout_maximized.png`: Workbench layout restored in maximized state confirming no window dimension loss.

---

## 7. Compliance Checklist
- [x] Frozen architecture specifications (`v1.0.md` through `v1.7.md`) preserved without modification.
- [x] Isolated QSettings test profile (`BitForgeTest`) strictly preserved.
- [x] Zero compiler warnings or errors on MSYS2 UCRT64 GCC 16.2.0.
- [x] 100% test pass rate across all 4 presets (306/306 tests).
- [x] Local repository commits only; no remote push.
