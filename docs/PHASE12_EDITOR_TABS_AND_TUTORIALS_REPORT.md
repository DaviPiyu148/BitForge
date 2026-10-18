# BitForge Phase 12 — Multi-Document Editor and Interactive Onboarding System Report

**Date:** October 19, 2026  
**Project:** `BitForge` (Educational 8-Bit Computer Workbench)  
**Base Commit:** `609b3d3` (Phase 11.1)  
**Target:** Qt 6.11.2 / C++17 Desktop Workbench  
**Status:** **Complete & Verified**

---

## 1. Executive Summary

BitForge Phase 12 implements two major developer-experience and pedagogical capabilities in the BitForge Qt 6 Widgets application:

1. **Production-Grade Multi-Document Tabbed Assembly Editor**:
   - Central document tabs widget (`QTabWidget* document_tabs_`) replacing the single-document stack.
   - Multi-document model (`EditorDocumentInfo`) managing untitled documents (`Untitled-1`, `Untitled-2`, etc.), named files, canonical paths, and individual editor instances.
   - Dirty-state tracking with asterisk (`*`) indicators on tab labels and main window titles, connected to both `contentsChanged` and `modificationChanged(bool)`.
   - Comprehensive tab lifecycle operations: New Tab (`Ctrl+N`), Open File (`Ctrl+O`), Close Tab (`Ctrl+W`), Close Active Tab, Close Others, Close All, Next Tab (`Ctrl+Tab`), Previous Tab (`Ctrl+Shift+Tab`).
   - Unsaved-changes protection: Interactive modal prompts (Save, Discard, Cancel) preventing accidental data loss when closing individual tabs, batch-closing tabs, or quitting the application.
   - Non-destructive atomic file operations: Unwritable paths preserve editor contents and dirty state without corrupting the document path.
   - Path canonicalization on Windows: Case-insensitive, relative-path, and separator-normalized matching prevents opening duplicate tabs for the same physical file.
   - Tab context menu: Right-click context menu on tab headers providing Save, Save As, Close Tab, Close Others, Close All Tabs, Copy Full Path, and Reveal in Explorer.
   - Execution decoupling: Switching editor tabs never reloads, resets, or corrupts the active CPU runtime state (registers, flags, memory, stack, devices). `Run from Entry (F5)` executes the active document, while `Continue (F8)`, `Step (F10)`, `Pause`, and `Reset (F6)` operate seamlessly on the active execution session.

2. **Integrated Welcome & Tutorial Center**:
   - Re-architected `WelcomeWidget` into a 3-tab onboarding dashboard (`welcome_tabs_`) embedded directly into document tabs as the initial tab (`⚡ Welcome`), coexisting with editor tabs.
   - **Tab 1: 🚀 Quick Start**: High-contrast action cards (New File, Open Source, Browse Examples, Guided Tour), Recent Files list with clear action, and Keyboard Shortcut quick reference grid.
   - **Tab 2: 📚 Curriculum (11 Lessons)**: A structured 11-lesson computer architecture course covering registers, execution lifecycle, two-pass assembly, memory, stack frames, breakpoints, tracing, watches, and virtual I/O devices, complete with one-click example loading.
   - **Tab 3: 🎓 First Program Tutorial**: Step-by-step beginner tutorial that creates `tutorial_first_program.asm` non-destructively, walking beginners through assembly, stepping, register inspection, and virtual LED hardware output.
   - **Startup Preference**: "Show Welcome on Startup" `QCheckBox` persisted via `QSettings`.

---

## 2. Verification & Test Metrics

All tests across all four CMake presets build with zero warnings and pass with a 100% success rate:

| CMake Preset | Configuration | Core Tests (GoogleTest) | GUI Tests (QtTest) | Pass Rate | Time |
| :--- | :--- | :---: | :---: | :---: | :---: |
| `debug` | Headless Debug | 233 / 233 | N/A | **100%** | 0.16s |
| `ci` | Strict CI (Warnings as Errors) | 233 / 233 | N/A | **100%** | 0.14s |
| `gui` | Debug GUI & Workbench | 233 / 233 | 67 / 67 | **100%** | 13.47s |
| `gui-release` | Optimized Release GUI | 233 / 233 | 67 / 67 | **100%** | 12.26s |

### Phase 12 Dedicated GUI Regression Test Suite (10 New Tests)
1. `testMultiDocumentTabsOpeningAndSwitching()`: Validates multi-tab creation, switching active tabs, verifying active editor decoupling, and tab bar indexing.
2. `testTabPathCanonicalizationAndNoDuplicates()`: Validates canonical duplicate tab prevention across relative paths, absolute paths, and Windows backslashes.
3. `testNewUntitledDocumentsAndDirtyIndicators()`: Validates sequential untitled numbering (`Untitled-1`, `Untitled-2`), dynamic asterisk (`*`) tab and window title dirty indicators, and clearing on save.
4. `testTabCloseAndUnsavedChangesProtection()`: Validates clean tab closing, dirty tab Discard branch, and dirty tab Cancel branch without data loss.
5. `testCloseAllAndCloseOtherTabs()`: Validates batch closing ("Close Others", "Close All Tabs") and automatic restoration of the Welcome tab when all documents are closed.
6. `testExecutionIndependenceAcrossTabSwitches()`: Validates that assembling Document 1 and switching active view to Document 2 does not alter or reassemble Document 1's executing session during stepping and running.
7. `testWelcomeAndTutorialCurriculumLessons()`: Validates 3-tab Welcome widget, 11 curriculum lessons in the lesson list, dynamic HTML rendering, and example loading buttons.
8. `testFirstProgramTutorialNonDestructive()`: Validates non-destructive launch of `tutorial_first_program.asm` with full instructions and duplicate activation.
9. `testWelcomeTabOpenCloseAndSettingsPersistence()`: Validates `showWelcomeOnStartup` preference toggling, QSettings persistence, closing the Welcome tab, and reopening via `showWelcomeScreen()`.
10. `testTabKeyboardShortcutsNavigation()`: Validates Next Tab and Previous Tab cycling across open documents.

---

## 3. Architecture & Implementation Details

### 3.1 Tabbed Multi-Document Editor Model
- **Central Widget Architecture**: Replaced the rigid `central_stack_` (`QStackedWidget`) with `central_container_` containing the `FindReplaceBar` on top and `document_tabs_` (`QTabWidget`) below.
- **Data Model**:
  ```cpp
  struct EditorDocumentInfo {
      CodeEditor* editor{nullptr};
      QString file_path;
      QString title;
      bool is_untitled{true};
  };
  ```
- **Tab Context Menu**: Right-clicking any tab opens a context menu with options to Save, Save As, Close Tab, Close Others, Close All Tabs, Copy Full Path, and Reveal in Explorer.
- **Execution Decoupling**: The runtime tracks `executing_editor_` and `executing_document_path_` independently from `current_editor_`. If the user is viewing Tab B while Tab A is executing, the status bar displays `READY (Exec: program_a.asm)` to make the decoupled execution context transparent.

### 3.2 Welcome & Tutorial Center
- **Tabbed Hierarchy**:
  - `welcome_tabs_` (`QTabWidget`):
    - Page 0: Quick Start
    - Page 1: Curriculum (11 Lessons)
    - Page 2: First Program Tutorial
- **Responsive Layout**: Pages are wrapped in frameless `QScrollArea` widgets to guarantee usability on compact screens (e.g. 1024x768) without inflating minimum window dimensions.
- **Lesson Curriculum Catalog**:
  1. Getting Started with BitForge (Architecture & CPU Lifecycle)
  2. The Assembly Editor (Directives, Syntax & Tabs)
  3. The Two-Pass Assembler & Diagnostics
  4. Execution Controls (Run, Step, Continue, Breakpoints & Speed)
  5. CPU Registers & Status Flags
  6. Memory Architecture & Hardware Call Stack
  7. Breakpoints, Execution Tracing & Instruction Inspector
  8. Watch Expressions & Variable Inspection
  9. Virtual Devices & I/O Ports
  10. Example Library & Further Practice
  11. Keyboard Shortcuts & Workspace Customization

---

## 4. Visual Verification

The following offscreen screenshots were captured during automated validation:
1. `gui_smoke_tabs_multidocument.png`: Visualizes multiple tabs (`⚡ Welcome`, `01_arithmetic.asm`, `02_io_demo.asm *`, `tutorial_first_program.asm`) with active teal indicator and dirty marker.
2. `gui_smoke_welcome_curriculum.png`: Visualizes the 11-lesson curriculum list, formatted lesson details, and one-click example button.
3. `gui_smoke_first_program_tutorial.png`: Visualizes `tutorial_first_program.asm` loaded in the editor with beginner instructions, register views, and watch expressions dock.

---

## 5. Portable Packaging & Smoke Test

The portable Windows packaging script was executed and verified:
- **Packaging Script**: `scripts/package_windows.ps1`
- **Output Directory**: `dist/BitForge-Portable-Windows-x64/`
- **Archive**: `dist/BitForge-Portable-Windows-x64.zip`
- **Offscreen Smoke Test**:
  ```cmd
  set PATH=C:\Windows\System32;C:\Windows
  dist\BitForge-Portable-Windows-x64\bitforge_gui.exe -platform offscreen --smoke-test
  ```
  **Result**: `[BitForge Smoke Test] SUCCESS: MainWindow and all panels initialized cleanly.` (Exit code 0).
