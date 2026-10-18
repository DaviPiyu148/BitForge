# BitForge Portable — Quick Start Guide

BitForge is a high-performance 8-bit digital computer emulator, two-pass assembler, source-level debugger, and Qt 6 educational workbench.

This package is completely standalone and requires no installation, administrator privileges, or external runtime installations.

---

## 1. Quick Launch

- **Double-click `launch_bitforge.bat`** (or `bitforge_gui.exe`) to start the GUI workbench.
- **Drag and drop** any `.asm` file onto `launch_bitforge.bat` or `bitforge_gui.exe` in Windows File Explorer to open it immediately.
- **Command Line**:
  ```cmd
  launch_bitforge.bat examples\01_arithmetic.asm
  ```

---

## 2. Package Contents

- `bitforge_gui.exe` — Desktop IDE and graphical debugger workbench.
- `bitforge_cli.exe` — Headless command-line assembler and execution runner.
- `launch_bitforge.bat` — Relative Windows batch launcher with path isolation.
- `examples/` — Complete collection of 51 assembly examples covering fundamentals, arithmetic, logic, branching, subroutines, I/O devices, algorithms, debugger demos, and edge cases.
- `platforms/qwindows.dll` — Qt 6 Windows platform integration plugin.
- `styles/`, `tls/`, `imageformats/` — Qt 6 support plugins.
- Runtime DLLs — `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`, `Qt6Network.dll`, `libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`.

---

## 3. Architecture Specification (v1.7 Frozen Contract)

- **Registers**: `R0`, `R1`, `R2`, `R3` (8-bit general purpose), `IX` (16-bit index register), `PC` (16-bit program counter), `SP` (16-bit stack pointer).
- **ALU Flags**: `Z` (Zero), `C` (Carry / Borrow), `N` (Negative / Sign), `V` (Two's Complement Signed Overflow).
- **Memory**: 64 KiB flat byte-addressable space (`0x0000`–`0xFFFF`).
  - Default program entry: `0x0200` (`.ORG 0x0200`, `.ENTRY start`).
  - Stack region: `0xF000`–`0xFFFE`, with empty sentinel `SP = 0xFFFF` per ADR-001.
- **Virtual I/O Ports**:
  - `0x00`: Input FIFO (interactive keyboard / byte stream input)
  - `0x01`: Text console output (ASCII stream)
  - `0x02`: Numeric display output (seven-segment vector display)
  - `0x10`: 8-bit Virtual LED register

---

## 4. Headless CLI Usage

To assemble and execute an assembly file without opening the graphical workbench:

```cmd
:: Run a program with an execution step limit
bitforge_cli.exe run examples\01_arithmetic.asm --max-steps 10000

:: Assemble a program to a binary image
bitforge_cli.exe assemble examples\01_arithmetic.asm -o program.bin

:: Inspect help and available options
bitforge_cli.exe --help
```
