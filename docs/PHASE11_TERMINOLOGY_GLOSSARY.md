# BitForge — Phase 11 Standardized Terminology & UI/UX Glossary

**Status:** Official Standard  
**Document Version:** 1.0  
**Phase:** 11 (UI/UX, Typography, Terminology, and Usability Audit)  
**Governing Architecture:** BitForge Architecture Specification v1.7  
**Date:** 2026-10-18  

---

## 1. Executive Summary & Purpose

This document defines the authoritative, standardized vocabulary, typography, notation, and interaction concepts for the **BitForge** ecosystem. It governs all user-facing desktop UI surfaces (menus, toolbars, panel headers, tab labels, status bars, dialogs, tooltips, empty-state indicators), assembly examples, technical documentation, and regression tests.

The goal is to maintain absolute conceptual precision and technical rigor while ensuring the application remains clear, accessible, and welcoming to students and engineers learning computer architecture.

---

## 2. Typographic & Notation Standards

### 2.1 Hexadecimal Values
- **Format:** Always lowercase prefix `0x` followed by uppercase hexadecimal digits.
- **Byte Values (8-bit):** Exactly 2 hexadecimal digits (e.g., `0x00`, `0x2A`, `0xFF`).
- **Word Values / Addresses (16-bit):** Exactly 4 hexadecimal digits (e.g., `0x0200`, `0x8000`, `0xFFFF`).
- **Prohibited:** Never format with capital prefix `0X` (e.g., avoid `0X0200` or `0X2A`).

### 2.2 Binary Values
- **Format:** Always lowercase prefix `0b` followed by binary bits `0` and `1`.
- **Byte Values:** Exactly 8 bits grouped or continuous (e.g., `0b01010101`).
- **Prohibited:** Never format with capital prefix `0B` (e.g., avoid `0B01010101`).

### 2.3 Decimal & Secondary Representations
- **Format:** When displayed alongside hexadecimal values, decimal numbers must be enclosed in parentheses with natural spacing (e.g., `0x0A (10)` or `0x00 (0)`).
- **Prohibited:** Avoid fixed-width space padding inside short labels (e.g., avoid `0x0A ( 10)` or `0x00 (  0)`).

### 2.4 Keyboard Shortcuts
- Express keyboard shortcuts using standard platform accelerators:
  - Function keys: `F5`, `F6`, `F7`, `F8`, `F9`, `F10`.
  - Combinations: `Ctrl+N`, `Ctrl+O`, `Ctrl+S`, `Ctrl+F`, `Shift+F3`.
  - Keys: `Enter`, `Escape` (or `Esc`), `Backspace`.

---

## 3. CPU Architecture & Register Nomenclature

| Canonical Term | Abbreviation | Bit Width | Description & UI Standard |
| :--- | :--- | :--- | :--- |
| **Program Counter** | `PC` | 16 bits | Pointer to the next instruction byte to be fetched and executed. Standard display: `0x0200`. |
| **Stack Pointer** | `SP` | 16 bits | Points to the current top of the downward-growing hardware stack. Empty sentinel value: `0xFFFF`. |
| **Index Register** | `IX` | 16 bits | Base register used for indexed memory addressing (`[IX+disp]`). |
| **General Register 0** | `R0` | 8 bits | Primary general-purpose register and arithmetic accumulator. |
| **General Register 1** | `R1` | 8 bits | General-purpose data register. |
| **General Register 2** | `R2` | 8 bits | General-purpose data register. |
| **General Register 3** | `R3` | 8 bits | General-purpose data register. |

### 3.1 ALU Status Flags
ALU flags represent the mathematical condition of the most recent ALU instruction.
- **Zero Flag (`Z`):** Set (`1`) if the operation result equals 0; cleared (`0`) otherwise.
- **Carry / No-Borrow Flag (`C`):** Set (`1`) on unsigned carry-out from addition, or when minuend ≥ subtrahend during subtraction/comparison (indicating no borrow occurred); cleared (`0`) on borrow.
- **Negative Flag (`N`):** Set (`1`) if bit 7 (MSB) of the result is 1 (negative in two's complement); cleared (`0`) otherwise.
- **Signed Overflow Flag (`V`):** Set (`1`) if signed two's-complement overflow occurs outside $[-128, +127]$; cleared (`0`) otherwise.

---

## 4. Execution Lifecycle & Controls

| Command / Action | Primary Shortcut | Semantic Definition & Contract |
| :--- | :--- | :--- |
| **Assemble** | `F7` | Two-pass assembly of the active editor buffer into binary machine code. Validates syntax, generates source-line mappings, initializes memory image, and transitions engine state to `Ready`. |
| **Run from Entry** | `F5` | Initiates a fresh program run. Resets CPU registers (`R0–R3=0`, `IX=0`, `PC=entry`), resets `SP` to empty sentinel `0xFFFF`, clears ALU flags, resets writable memory variables back to their initial static-data image, clears I/O devices, and runs execution from the program entry point. |
| **Continue** | `F8` | Resumes continuous execution from the *current* `PC` without resetting registers, memory, stack, or peripheral state. Used to proceed past breakpoints or paused steps. |
| **Step** | `F10` | Single-step execution. Executes exactly one instruction from current `PC`, updates all UI views, journals the execution delta, and halts in `Paused` state. |
| **Pause** | `F8` (Toggle) / UI | Suspends active continuous execution slices immediately at the next instruction boundary, keeping all machine state intact. |
| **Reset** | `F6` | Clears CPU state, resets `SP=0xFFFF`, flushes virtual I/O buffers, and returns machine state to `Ready` at entry. |

### 4.1 Machine States
- **`READY` (Green):** Program assembled and loaded into memory; engine primed at entry address.
- **`RUNNING` (Blue):** Asynchronous execution loop actively stepping instructions.
- **`PAUSED` (Amber):** Execution suspended at a breakpoint, following a single step, or by user pause.
- **`HALTED` (Grey / Slate):** Program completed normally via execution of the `HALT` opcode (`0x01`).
- **`FAULTED` (Crimson / Red):** Execution trapped on an unrecoverable architectural CPU fault.
- **`STALE` (Amber / Warning):** Source buffer was edited after the last assembly; requires reassembly (`F7`) before running.

---

## 5. Memory, Addressing & Stack Taxonomy

### 5.1 Memory Space
- **Total Address Space:** 64 KiB ($65,536$ bytes), spanning `0x0000` through `0xFFFF`.
- **Byte (`byte_t`):** Fundamental 8-bit storage unit.
- **Word (`word_t`):** 16-bit value stored in **little-endian** order (least significant byte at address $A$, most significant byte at address $A+1$).
- **Code Segment / Entry:** Standard program code typically originates at `.ORG 0x0200` with explicit `.ENTRY`.
- **Static Data Region:** Memory defined using assembler directives `.BYTE`, `.WORD`, or `.STRING`.

### 5.2 Hardware Stack
- **Stack Region:** Addresses `0xF000` through `0xFFFE`.
- **Growth Direction:** Downward towards lower memory addresses.
  - `PUSH` decrements `SP` by 1 (or 2 for words) and writes data.
  - `POP` reads data and increments `SP` by 1 (or 2 for words).
- **Empty Sentinel:** `SP = 0xFFFF`. When no items are pushed on the stack, the stack pointer resides at `0xFFFF`.
- **UI Labeling Rule:** The workbench tab must be labeled **`Hardware Stack`** (or **`Stack`**). Internal architectural document names (such as `ADR-001`) must remain in technical specifications and code docstrings, not in end-user workbench UI tabs or status banners.
- **Empty Stack Display:** Standard status text: `"Empty stack (SP = 0xFFFF; no items pushed)"`.

---

## 6. Virtual Devices & Port Map

| Port Address | Device Name | Direction | Behavioral Contract |
| :--- | :--- | :--- | :--- |
| **`Port 0x00`** | **Input FIFO** | Input (`IN`) | Byte FIFO queue for keyboard and stream input. `IN R0, 0x00` pops the next byte. If empty, returns `0x00` and increments underflow counter without faulting. |
| **`Port 0x01`** | **Text Console** | Output (`OUT`) | ASCII character console. `OUT 0x01, R0` appends the character to the terminal view. |
| **`Port 0x02`** | **Numeric Console & 7-Segment Display** | Output (`OUT`) | Numeric decimal output. `OUT 0x02, R0` writes an unsigned decimal byte ($0$–$255$) to the numeric log and updates the 3-digit vector 7-segment display. |
| **`Port 0x10`** | **Virtual LED Array** | Output (`OUT`) | 8-bit physical indicator row. Bits 7..0 map to individual green/dark LEDs. Formatted as `0x.. (0b........)`. |
| **`Port 0x03`–`0x0F`, `0x11`–`0xFF`** | **Unmapped Ports** | In/Out | Non-strict mode: writes are ignored; reads return `0x00`. Strict mode: triggers `IoPortUnmapped` CPU fault. |

---

## 7. Diagnostics, Faults & Errors

### 7.1 Assembler Diagnostics (Compile Time)
- **Error:** Fatal syntax or semantic violation preventing binary emission (e.g., undefined label, operand range overflow, reserved address conflict).
- **Warning:** Non-fatal advisory notice that does not stop compilation.
- **UI Surface:** Diagnostics panel in bottom dock displaying severity, line number, column, and message.

### 7.2 CPU Faults (Runtime)
- **Architectural Traps:** Runtime exceptions triggering atomic CPU fault state:
  - `DivisionByZero`: Attempt to execute `DIV` with divisor register equal to zero.
  - `InvalidOpcode`: Undefined byte encountered during opcode fetch.
  - `StackOverflow`: `PUSH` attempted when `SP` has reached the stack floor (`0xF000`).
  - `StackUnderflow`: `POP` attempted when `SP` is at the empty sentinel (`0xFFFF`).
  - `OutOfBoundsMemory`: Access outside valid memory bounds.
  - `IoPortUnmapped`: Access to unmapped port under strict I/O policy.
- **Behavior:** CPU execution halts immediately, state changes to `FAULTED`, and registers remain at the pre-fault state for post-mortem debugging.

---

## 8. UI Terminology Standards & Prohibitions

| Approved Term | Prohibited / Deprecated Term | Rationale |
| :--- | :--- | :--- |
| **Hardware Stack** | Stack (ADR-001) | Avoid exposing internal software engineering ADR references to students. |
| **Empty stack (SP = 0xFFFF; no items pushed)** | Empty stack (SP = 0xFFFF sentinel per ADR-001; no items pushed) | Clean, educational phrasing without internal design record numbers. |
| **Values** (Tab in Registers Panel) | Hex && Dec | Prevents ugly tab text truncation and ampersand rendering artifacts. |
| **Bits** (Tab in Registers Panel) | Bit Visualization | Fits comfortably in narrow dock panels alongside "Values". |
| **Run from Entry (F5)** | Run | Distinguishes fresh-run semantics (resetting memory/stack) from resuming execution. |
| **Continue (F8)** | Resume | Standard debugger convention matching GDB, LLDB, and Visual Studio. |
| **Step (F10)** | Step Into / Step Over | BitForge operates at single-instruction granularity; "Step" is concise and exact. |
| **0x0200** | 0X0200 | Standard lowercase hexadecimal prefix. |
| **0b01010101** | 0B01010101 | Standard lowercase binary prefix. |
| **0x0A (10)** | 0x0A ( 10) | Natural spacing without arbitrary whitespace padding in decimal parentheses. |
| **Numeric Display** / **Decimal Readout** | Numeric Display — Port 0x02 (inside bezel) | Removes repetitive header duplication inside the display bezel. |

---

## 9. Conclusion

Adherence to this terminology glossary is mandatory across all BitForge user interfaces, documentation files, tutorial guides, and test assertions. Any deviation is considered a documentation or UX defect.
