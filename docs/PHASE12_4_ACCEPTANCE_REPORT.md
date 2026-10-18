# BitForge Phase 12.4 — UI Defects and Qt Runtime Warning Acceptance Report

**Date:** October 19, 2026  
**Auditor & Implementation Agent:** Antigravity (Google DeepMind)  
**Baseline Git Commit:** `86c82f1` (Phase 12.3 Final Acceptance)  
**Target Git Branch:** `master`  
**Repository Working Directory:** `d:\BitForge`  

---

## 1. Executive Summary & Verification Matrix

BitForge Phase 12.4 successfully diagnoses, eliminates, and validates three critical usability and runtime defects identified after the Phase 12.3 release:

1. **Defect 6: Invisible Editor-Tab Close Button** — Resolved missing / faint tab close button graphic by generating high-contrast crisp vector close icons (`close-tab.png`, `close-tab-hover.png`, `close-tab-pressed.png`), adjusting `QTabBar::close-button` stylesheet rules to eliminate compression padding, setting explicit 16x16 dimensions, and verifying click-to-close vs. tab-switching behavior.
2. **Defect 7: Non-Clickable Max Steps Spin-Box Up Button** — Resolved unresponsive up button and missing arrow graphics by introducing dedicated `QSpinBox`, `QSpinBox::up-button`, `QSpinBox::down-button`, `QSpinBox::up-arrow`, and `QSpinBox::down-arrow` subcontrol rules with 22px right padding, generating crisp vector triangular arrows (`spin-up.png`, `spin-up-hover.png`, `spin-down.png`, `spin-down-hover.png`), and verifying mouse clicks, keyboard navigation, bounds, and budget synchronization.
3. **Defect 8: Negative-pointSize Font Runtime Warning** — Eliminated console warning `QFont::setPointSize: Point size <= 0 (-1), must be greater than 0` caused by an erroneous `QFont(family, -1, QFont::Bold)` constructor invocation in `DevicesPanel::showDiagnostics()`, and added defensive guards `if (pointSize > 0)` across theme font creation helpers.

### Deliverables Status Summary

| Item | Description | Baseline (`86c82f1`) | Phase 12.4 Status | Verification Evidence |
| :--- | :--- | :--- | :--- | :--- |
| **Defect 6** | Tab close button visibility & ergonomics | Invisible / faint blur | **Resolved** | 16x16 bright cross (`#e6edf3`, >12:1 contrast), automated click & contrast tests |
| **Defect 7** | Max Steps spin-box up button clickability | Blank boxes / up button unclickable | **Resolved** | Dedicated subcontrols, vector arrows, `QTest::mouseClick` step verification |
| **Defect 8** | Qt `QFont::setPointSize` runtime warning | Console warning on CPU fault & startup | **Resolved** | Replaced `-1` pointSize with `font.setBold(true)`, guarded theme font helpers |
| **GUI Tests**| Test suite expansion & regression coverage | 80 QtTest slots | **82 QtTest slots** | 82 / 82 slots PASS (100%) |
| **Core Tests**| Architecture & engine test coverage | 233 core checks | **233 core checks** | 233 / 233 tests PASS (100%) |
| **Total Tests**| Total test suite verification | 313 total checks | **315 total checks** | 315 / 315 tests PASS (100%) across all presets |
| **4 Presets** | `gui`, `debug`, `ci`, `gui-release` | Prior runs | **100% Pass** | All 4 presets pass cleanly |
| **Packaging** | Windows standalone portable package | Prior runs | **100% Pass** | `scripts/package_windows.ps1 -Validate` exit code 0 |

---

## 2. Root Cause Analysis & Technical Solutions

### 2.1 Defect 6: Invisible Editor-Tab Close Button

#### Observed Symptom
In editor tabs (e.g., `02_io_demo.asm`, `Untitled-1`), space was reserved on the right edge of each tab, but no clear close cross was visible. The close button rendered as an indistinct ~2px grey smudge that blended into the dark `#181a1f` background.

#### Root Causes
1. **Low Contrast & Line Thinness in Asset Generator:** In `tools/render_icons.cpp`, `drawCloseIcon` drew a 6x6 cross with a 1.6px stroke using low-contrast `#abb2bf`. When antialiased onto a transparent background and drawn over dark surfaces, alpha blending reduced perceived luminance to under `#40444b`, with a contrast ratio below 1.8:1.
2. **Compressive Padding in Stylesheet:** In `src/gui/theme.cpp`, `QTabBar::close-button` specified `padding: 2px;` alongside `margin-right: 5px; margin-left: 8px;`. In Qt's QStyleSheetStyle engine, inner padding on subcontrols reduces the paint rectangle inside the button boundary, shrinking the 16x16 icon to ~12x12 or less and obliterating the fine cross strokes.
3. **Missing Pressed State Asset:** The pressed state reused the hover asset without visual tactile feedback.

#### Technical Solution
1. **Asset Generation (`tools/render_icons.cpp`):**
   - Updated `drawCloseIcon` to draw a bold, crisp 9x9 vector cross centered in 16x16 with 2.0px stroke width.
   - Normal state: Bright silver-white (`#e6edf3`), yielding a contrast ratio of > 12:1 against `#181a1f` (far exceeding WCAG AAA requirements).
   - Hover state (`close-tab-hover.png`): Rounded dark-slate badge (`#3e4451`) with pure white (`#ffffff`) cross.
   - Pressed state (`close-tab-pressed.png`): Rounded crimson accent (`#e06c75`) with pure white (`#ffffff`) cross.
2. **Stylesheet Optimization (`src/gui/theme.cpp`):**
   ```css
   QTabBar::close-button {
       image: url(:/icons/close-tab.png);
       subcontrol-position: right;
       subcontrol-origin: padding;
       width: 16px;
       height: 16px;
       margin-top: 2px;
       margin-bottom: 2px;
       margin-right: 4px;
       margin-left: 6px;
       border-radius: 3px;
   }
   QTabBar::close-button:hover {
       image: url(:/icons/close-tab-hover.png);
       background-color: #3e4451;
   }
   QTabBar::close-button:pressed {
       image: url(:/icons/close-tab-pressed.png);
       background-color: #e06c75;
   }
   ```
   Removing `padding: 2px` permits 1:1 pixel rendering of the 16x16 icon asset without subcontrol clipping.

---

### 2.2 Defect 7: Non-Clickable Max Steps Spin-Box Up Button

#### Observed Symptom
In the execution toolbar, the `Max Steps` spin box accepted direct keyboard input, and the lower half of the button column responded to clicks, but clicking the top half (the up button) was unresponsive or did not increment the budget. Furthermore, the button column appeared as two dark blank squares without visible arrow graphics.

#### Root Causes
1. **Missing Subcontrol Definitions:** In `src/gui/theme.cpp`, `QSpinBox` was grouped under the generic input selector `QLineEdit, QSpinBox, QComboBox` with `padding: 4px 8px;`. No rules existed for `QSpinBox::up-button`, `QSpinBox::down-button`, `QSpinBox::up-arrow`, or `QSpinBox::down-arrow`.
2. **Child Widget Hit Interception:** Because `padding-right` was only 8px, Qt's internal `QLineEdit` child widget expanded over the right edge of the spinbox, directly intercepting mouse click events intended for the `SC_SpinBoxUp` subcontrol.
3. **Missing Arrow Icons:** Without explicit `up-arrow` and `down-arrow` image specifications, Qt's fallback style painted solid dark background rects with unrendered arrows.

#### Technical Solution
1. **Asset Generation (`tools/render_icons.cpp`):**
   - Created vector arrow icons: `spin-up.png`, `spin-up-hover.png`, `spin-down.png`, `spin-down-hover.png`.
   - Rendered anti-aliased centered triangular polygons in neutral slate `#abb2bf` and hover teal `#4ec9b0`.
   - Registered all four assets in `resources/bitforge.qrc`.
2. **Dedicated Stylesheet Rules (`src/gui/theme.cpp`):**
   - Separated `QSpinBox` from `QLineEdit` and applied `padding-right: 22px;` so the text edit region never overlaps the button subcontrols.
   - Defined `QSpinBox::up-button` with `subcontrol-origin: border; subcontrol-position: top right; width: 18px; border-left: 1px solid #3b4048; border-bottom: 1px solid #3b4048; border-top-right-radius: 3px; background-color: #282c34;`.
   - Defined `QSpinBox::down-button` with `subcontrol-origin: border; subcontrol-position: bottom right; width: 18px; border-left: 1px solid #3b4048; border-bottom-right-radius: 3px; background-color: #282c34;`.
   - Mapped `up-arrow` and `down-arrow` to the new resource images with hover and disabled/off handling.
3. **Ergonomic Enhancements (`src/gui/main_window.cpp`):**
   - Set `budget_label->setBuddy(budget_spinbox_);` for accessibility and keyboard focus routing.
   - Set `budget_spinbox_->setMinimumWidth(110);` ensuring 9 digits (`100,000,000`) and subcontrols fit comfortably without truncation.
   - Added tooltip: `"Maximum instruction execution step budget before pausing"`.

---

### 2.3 Defect 8: Negative-pointSize Font Runtime Warning

#### Observed Symptom
Whenever a CPU fault occurred or the diagnostics table was populated, the Qt runtime recorded:
`QFont::setPointSize: Point size <= 0 (-1), must be greater than 0`

#### Root Cause
In `src/gui/devices_panel.cpp` line 375:
```cpp
item_sev->setFont(QFont(item_sev->font().family(), -1, QFont::Bold));
```
In Qt 6, the `QFont` constructor signature is:
```cpp
QFont(const QString &family, int pointSize = -1, int weight = -1, bool italic = false);
```
Passing `-1` as the second argument passed `-1` as `pointSize`! When Qt initializes the font object with `pointSize <= 0`, it emits a `qWarning`.

#### Technical Solution
1. **Fix in `src/gui/devices_panel.cpp`:**
   ```cpp
   auto* item_sev = new QTableWidgetItem("CPU Fault");
   item_sev->setForeground(QColor(220, 53, 69));
   QFont fault_font = item_sev->font();
   fault_font.setBold(true);
   item_sev->setFont(fault_font);
   diag_table_->setItem(row, 0, item_sev);
   ```
   This preserves the item's existing valid positive point size and sets bold styling directly.
2. **Defensive Guards in `src/gui/theme.cpp`:**
   Updated `theme::monoFont` and `theme::uiFont` with `if (pointSize > 0)` safety guards before calling `font.setPointSize(pointSize)`.

---

## 3. Automated Test Suite Expansion

Two new dedicated regression tests and comprehensive updates to existing tests were implemented in `tests/gui/test_gui.cpp`:

### 3.1 `testEditorTabCloseButtonMetricsAndStyle`
- **Resource Integrity:** Asserts `:/icons/close-tab.png`, `close-tab-hover.png`, and `close-tab-pressed.png` exist in the Qt resource bundle.
- **Stylesheet Rules:** Validates presence of `QTabBar::close-button`, `width: 16px`, `height: 16px`, and hover/pressed states.
- **Subcontrol Metrics:** Verifies the close button exists on editor tab 1, is visible, and meets dimensions (`>= 16x16`).
- **Contrast Audit:** Grabs the rendered close button pixmap, iterates over its pixels, and asserts `>= 10` pixels exhibit high-luminance silver/white (`R >= 180, G >= 180, B >= 180`), confirming the bold cross is actually rendered.
- **Click Discrimination:** Clicks the left/center label area of the tab and asserts tab switches without closing. Clicks the close button directly and asserts the tab count decrements.

### 3.2 `testMaxStepsSpinBoxSubcontrolsAndMouseClicks`
- **Resource Integrity:** Asserts all four spin arrow PNGs exist.
- **Geometry Non-Overlap:** Uses `QStyleOptionSpinBox` and `style()->subControlRect()` to verify `up_rect` and `down_rect` are valid, within the widget boundary, and do NOT intersect `edit_rect`.
- **Mouse Click Interaction:** Simulates `QTest::mouseClick` at `up_rect.center()` (increments value by 10,000 from 1,000,000 to 1,010,000) and `down_rect.center()` (decrements value back to 1,000,000).
- **Keyboard Interaction:** Simulates `Qt::Key_Up` and `Qt::Key_Down` key clicks.
- **Boundary Clamping:** Verifies upper clamp at `100,000,000` and lower clamp at `1`.
- **Budget Synchronization:** Sets budget to 500,000, executes a program via `window.onRun()`, and verifies the controller receives and honors the budget.

### 3.3 `testNoNegativePointSizeWarningOnFaultOrStartup`
- **Helper Robustness:** Asserts `theme::monoFont(-1)`, `monoFont(0)`, `uiFont(-1)`, and `uiFont(0)` return valid fonts with positive point sizes without warnings.
- **Fault Diagnostics Integration:** Installs a custom Qt message handler, runs `01_invalid_opcode.asm` to trigger a CPU fault, calls `devices->showDiagnostics()`, validates the "CPU Fault" table item is bold with positive point size, and asserts zero `setPointSize` warnings were emitted.

---

## 4. Test Execution & Presets Verification

### Test Count Reconciliation
- Core Unit & Architecture Tests: **233 / 233 passed** (ALU, Bus, CPU, Memory, Types, CLI, Debugger, Examples).
- GUI Acceptance Test Suite: **82 / 82 slots passed** (increased from 80 with the 2 new Phase 12.4 test slots).
- Total Project Checks: **315 / 315 passed (100%)**.

### Presets Matrix Results

```
======================================================================
 BitForge Multi-Preset Build and Test Verification
======================================================================
 [1] gui (Debug with GUI Tests):
     BitForgeTests:     233 / 233 Passed (0.14s)
     BitForgeGuiTests:   82 /  82 Passed (12.87s)
     Status: 100% PASSED

 [2] debug (Core Debug):
     BitForgeTests:     233 / 233 Passed (0.13s)
     Status: 100% PASSED

 [3] ci (Core CI / Sanitizers):
     BitForgeTests:     233 / 233 Passed (0.13s)
     Status: 100% PASSED

 [4] gui-release (Release with GUI Tests & Packaging):
     BitForgeTests:     233 / 233 Passed (0.06s)
     BitForgeGuiTests:   82 /  82 Passed (12.71s)
     Status: 100% PASSED
======================================================================
```

---

## 5. Windows Portable Packaging Validation

The automated Windows packaging script was executed with full verification:
```powershell
powershell -ExecutionPolicy Bypass -File scripts/package_windows.ps1 -Validate
```

**Results:**
- Binary packaging: `dist/BitForge-Portable-Windows-x64/` staged with `bitforge_gui.exe`, `bitforge_cli.exe`, Qt 6 DLLs, and 51 assembly examples.
- ZIP distribution: `dist/BitForge-Portable-Windows-x64.zip` created.
- Sanitized PATH validation (`C:\Windows\System32;C:\Windows`):
  * CLI `--version` passed (`BitForge 0.1.0 (Architecture Specification v1.7)`).
  * CLI `--help` passed.
  * CLI register dump / program execution passed cleanly.
  * GUI smoke test (`-platform offscreen --smoke-test`) passed cleanly with zero font warnings:
- Script exit code: **0**.

> [!NOTE]
> **Packaging Scope & Clean-Machine Validation Boundary:**
> While `package_windows.ps1 -Validate` confirms that the packaged portable binary bundle operates with complete independence from the development compiler toolchain by running under a sanitized runtime environment (`PATH=C:\Windows\System32;C:\Windows`), this validation is executed on the development host. It validates self-contained DLL staging, runtime dependency resolution, asset binding, and CLI/GUI invocation. However, it does not substitute for testing on an independent clean Windows installation (or air-gapped clean VM) where system-wide runtimes, registry keys, and graphics driver stacks have not been previously installed. A clean-machine matrix in an isolated sandbox or CI runner is recommended prior to broad end-user distribution.

---

## 6. Commit Manifest

The changes staged for Phase 12.4 are contained entirely within non-frozen source, tool, and resource files:

| File | Change Description |
| :--- | :--- |
| `resources/icons/close-tab.png` | 16x16 bold high-contrast silver-white cross (`#e6edf3`, stroke 2.0) |
| `resources/icons/close-tab-hover.png` | 16x16 hover badge (`#3e4451`) with pure white cross |
| `resources/icons/close-tab-pressed.png` | 16x16 pressed badge (`#e06c75`) with pure white cross |
| `resources/icons/spin-up.png` | Vector triangular up arrow in slate `#abb2bf` |
| `resources/icons/spin-up-hover.png` | Vector triangular up arrow in teal `#4ec9b0` |
| `resources/icons/spin-down.png` | Vector triangular down arrow in slate `#abb2bf` |
| `resources/icons/spin-down-hover.png` | Vector triangular down arrow in teal `#4ec9b0` |
| `resources/bitforge.qrc` | Registered new close pressed and spin arrow icon aliases |
| `tools/render_icons.cpp` | Added vector rendering generators for close icons and spin arrows |
| `src/gui/theme.cpp` | Updated `QTabBar::close-button`, added `QSpinBox` subcontrols, guarded font helpers |
| `src/gui/devices_panel.cpp` | Fixed `QFont` invocation to set bold without negative point size |
| `src/gui/main_window.cpp` | Set buddy relationship, min width 110px, and tooltip on `budget_spinbox_` |
| `tests/gui/test_gui.cpp` | Updated close button test and added spinbox & font warning regression tests |
| `docs/PHASE12_4_ACCEPTANCE_REPORT.md` | Formal engineering and visual acceptance report |

No frozen CPU architecture specifications (`v1.0` through `v1.7`) were modified.
All modifications remain strictly local on the `master` branch (no remote push).
