# BitForge Example Library & Architecture Test Suite

Welcome to the **BitForge Assembly Example Library**. This directory contains a comprehensive, curated collection of verified BitForge assembly programs (`.asm`) designed for learning assembly programming, regression testing, and demonstrating the Qt 6 debugger and emulator workbench.

All programs conform to `BitForge_Architecture_Specification_v1.7.md` and ADR-001.

---

## Directory Organization

```text
examples/
  README.md                     # This catalog, instruction matrix, and guide
  01_arithmetic.asm             # Baseline arithmetic example
  02_io_demo.asm                # Baseline virtual I/O demonstration
  fundamentals/                 # NOP, HALT, register loading & transfer
  arithmetic/                   # ADD, SUB, INC, DEC, bitwise logic, flags
  control_flow/                 # JMP, JZ, JNZ, JC, JNC, countdown loops
  memory/                       # Direct load/store, IX indexing, array traversal
  stack_subroutines/            # PUSH, POP, CALL, RET, callee-save, nested calls
  io_devices/                   # Text console, decimal console, virtual LEDs, input FIFO
  algorithms/                   # Multiplication, division, sum of N, Fibonacci, GCD
  debugger/                     # Step tracing, breakpoints, self-loops, memory stepping
  faults/                       # Runtime CPU fault demonstrations
  invalid/                      # Invalid syntax fixtures (intentional assembly errors)
```

---

## 31-Instruction Coverage Matrix

The BitForge instruction set contains exactly **31 canonical instruction mnemonics** organized into four functional groups per Section 6.1 of `BitForge_Architecture_Specification_v1.7.md`. Opcodes are non-contiguous grouped byte values:
- **Control & Data Movement (11 mnemonics):** `0x00`, `0x01`, `0x10`..`0x18`
- **Arithmetic & Logic (9 mnemonics):** `0x20`..`0x23`, `0x30`..`0x33`, `0x40`
- **Branch, Stack & Call (9 mnemonics):** `0x50`..`0x54`, `0x60`..`0x63`
- **Input/Output (2 mnemonics):** `0x70`, `0x71`

Every single instruction in the BitForge ISA is executed by at least one runnable example:

| Opcode | Mnemonic | Format | Length | Primary Demonstrating Example |
| :---: | :---: | :--- | :---: | :--- |
| `0x00` | **NOP** | Implied | 1 | `fundamentals/01_nop_halt.asm` |
| `0x01` | **HALT** | Implied | 1 | All runnable examples |
| `0x10` | **LDI** | `LDI Rd, imm8` | 3 | `fundamentals/02_registers_transfer.asm` |
| `0x11` | **MOV** | `MOV Rd, Rs` | 3 | `fundamentals/02_registers_transfer.asm` |
| `0x12` | **LD** | `LD Rd, [addr16]` | 4 | `memory/01_direct_load_store.asm` |
| `0x13` | **ST** | `ST [addr16], Rs` | 4 | `memory/01_direct_load_store.asm` |
| `0x14` | **LDI16**| `LDI16 IX, addr16` | 3 | `memory/02_indexed_access_ix.asm` |
| `0x15` | **LDX** | `LDX Rd, [IX]` | 2 | `memory/02_indexed_access_ix.asm` |
| `0x16` | **STX** | `STX [IX], Rs` | 2 | `memory/02_indexed_access_ix.asm` |
| `0x17` | **INX** | Implied (`INX`) | 1 | `memory/02_indexed_access_ix.asm` |
| `0x18` | **DEX** | Implied (`DEX`) | 1 | `memory/02_indexed_access_ix.asm` |
| `0x20` | **ADD** | `ADD Rd, Rs` | 3 | `arithmetic/01_addition_subtraction.asm` |
| `0x21` | **SUB** | `SUB Rd, Rs` | 3 | `arithmetic/01_addition_subtraction.asm` |
| `0x22` | **INC** | `INC Rd` | 2 | `arithmetic/02_increment_decrement.asm` |
| `0x23` | **DEC** | `DEC Rd` | 2 | `arithmetic/02_increment_decrement.asm` |
| `0x30` | **AND** | `AND Rd, Rs` | 3 | `arithmetic/03_bitwise_logic.asm` |
| `0x31` | **OR** | `OR Rd, Rs` | 3 | `arithmetic/03_bitwise_logic.asm` |
| `0x32` | **XOR** | `XOR Rd, Rs` | 3 | `arithmetic/03_bitwise_logic.asm` |
| `0x33` | **NOT** | `NOT Rd` | 2 | `arithmetic/03_bitwise_logic.asm` |
| `0x40` | **CMP** | `CMP Ra, Rb` | 3 | `arithmetic/04_comparison_flags.asm` |
| `0x50` | **JMP** | `JMP addr16` | 3 | `control_flow/01_unconditional_jump.asm` |
| `0x51` | **JZ** | `JZ addr16` | 3 | `control_flow/02_branch_zero.asm` |
| `0x52` | **JNZ** | `JNZ addr16` | 3 | `control_flow/02_branch_zero.asm` |
| `0x53` | **JC** | `JC addr16` | 3 | `control_flow/03_branch_carry.asm` |
| `0x54` | **JNC** | `JNC addr16` | 3 | `control_flow/03_branch_carry.asm` |
| `0x60` | **CALL**| `CALL addr16` | 3 | `stack_subroutines/02_subroutine_call_ret.asm` |
| `0x61` | **RET** | Implied (`RET`) | 1 | `stack_subroutines/02_subroutine_call_ret.asm` |
| `0x62` | **PUSH**| `PUSH Rs` | 2 | `stack_subroutines/01_push_pop_basic.asm` |
| `0x63` | **POP** | `POP Rd` | 2 | `stack_subroutines/01_push_pop_basic.asm` |
| `0x70` | **IN** | `IN Rd, port8` | 3 | `io_devices/04_input_fifo_poll.asm` |
| `0x71` | **OUT** | `OUT port8, Rs` | 3 | `io_devices/01_text_console_output.asm` |

---

## Quick-Start: Running Examples

### Option 1: Desktop GUI Workbench
1. Double-click `launch_bitforge.bat` in the repository root.
2. In BitForge, open any `.asm` file via **File → Open File...** (`Ctrl+O`) or drag and drop a file onto `launch_bitforge.bat`.
3. Press <kbd>F7</kbd> to **Assemble**.
4. Press <kbd>F10</kbd> to **Single-Step** or <kbd>F5</kbd> to **Run**.
5. Click in the left editor gutter to set breakpoints (marked by red dots `●`).

### Option 2: Command-Line Runner (CLI)
From PowerShell (with MSYS2 UCRT64 in PATH):
```powershell
# Assemble and run with register dump
.\build\gui\bitforge.exe run examples/arithmetic/01_addition_subtraction.asm --dump-regs

# Run an algorithm example
.\build\gui\bitforge.exe run examples/algorithms/01_multiplication_repeated_addition.asm --dump-regs
```

---

## Catalog of Verified Examples

### 1. Fundamentals (`examples/fundamentals/`)

| File | Description | Termination | Expected Registers | Expected Flags |
| :--- | :--- | :---: | :--- | :---: |
| `01_nop_halt.asm` | Three sequential NOPs followed by HALT. | Halted | `R0-R3 = 0` | `Z=0 C=0 N=0 V=0` |
| `02_registers_transfer.asm` | LDI and MOV data transfer across R0..R3. | Halted | `R0=0x99, R1=0x42, R2=0x99, R3=0x99` | `Z=0 C=0 N=0 V=0` |

### 2. Arithmetic & Logic (`examples/arithmetic/`)

| File | Description | Termination | Expected Registers | Expected Flags |
| :--- | :--- | :---: | :--- | :---: |
| `01_addition_subtraction.asm` | (50 + 25) - 15 = 60. | Halted | `R0=0x3C (60), R1=0x19, R2=0x0F` | `Z=0 C=0 N=0 V=0` |
| `02_increment_decrement.asm` | INC past 0xFF (wraps to 0), DEC past 0x00. Preserves C flag. | Halted | `R0=0x00, R1=0xFF` | `Z=0 C=0 N=1 V=0` |
| `03_bitwise_logic.asm` | Bitwise AND, OR, XOR, NOT operations. | Halted | `R0=0x5A, R1=0x0F, R2=0x50, R3=0xFF` | `Z=0 C=0 N=0 V=0` |
| `04_comparison_flags.asm` | CMP 10, 20 updates flags without altering registers. | Halted | `R0=10, R1=20` | `Z=0 C=1 N=1 V=0` |
| `05_carry_borrow_boundaries.asm` | Unsigned carry generation (0xFF + 1) and borrow (4 - 6). | Halted | `R0=0x00, R1=0x01, R2=0xFE, R3=0x06` | `Z=0 C=1 N=1 V=0` |
| `06_signed_overflow_boundaries.asm` | Signed overflow (+127 + 1 = -128) and underflow (-128 - 1 = +127). | Halted | `R0=0x80, R1=0x01, R2=0x7F, R3=0x01` | `Z=0 C=0 N=0 V=1` |

### 3. Control Flow (`examples/control_flow/`)

| File | Description | Termination | Expected Registers | Expected Flags |
| :--- | :--- | :---: | :--- | :---: |
| `01_unconditional_jump.asm` | JMP over a trap instruction to target. | Halted | `R0=1, R1=2, R2=0` | `Z=0 C=0 N=0 V=0` |
| `02_branch_zero.asm` | Tests JZ taken on zero, JNZ taken on non-zero. | Halted | `R0=0x00, R1=0x01, R2=0xAA` | `Z=0 C=0 N=0 V=0` |
| `03_branch_carry.asm` | Tests JC taken on carry set, JNC taken on carry clear. | Halted | `R0=0x20, R1=0xBB` | `Z=0 C=0 N=0 V=0` |
| `04_countdown_loop.asm` | Counted loop adding 5 + 4 + 3 + 2 + 1 = 15. | Halted | `R0=0, R1=15 (0x0F)` | `Z=1 C=0 N=0 V=0` |

### 4. Memory & Pointers (`examples/memory/`)

| File | Description | Termination | Expected Registers | Expected Memory / State |
| :--- | :--- | :---: | :--- | :--- |
| `01_direct_load_store.asm` | ST to 0x8050, clear R0, LD back into R1. | Halted | `R0=0x00, R1=0x7E` | `[0x8050] == 0x7E` |
| `02_indexed_access_ix.asm` | LDI16 IX, STX, INX, DEX, LDX sequence. | Halted | `R1=0x11, R2=0x22, IX=0x8001` | `[0x8000]=0x11, [0x8001]=0x22` |
| `03_array_traversal.asm` | Traversing `.BYTE 10, 20, 30, 40, 50` array via IX. | Halted | `R0=50, R1=0, IX=0x0305` | Read all 5 elements |
| `04_array_summation.asm` | Sums elements 5, 10, 15, 20, 25 to memory 0x8100. | Halted | `R0=75 (0x4B), R1=0` | `[0x8100] == 0x4B` |

### 5. Stack & Subroutines (`examples/stack_subroutines/`)

| File | Description | Termination | Expected Registers | Stack State (ADR-001) |
| :--- | :--- | :---: | :--- | :--- |
| `01_push_pop_basic.asm` | PUSH 0x11, 0x22, 0x33 and POP in reverse order. | Halted | `R1=0x33, R2=0x22, R3=0x11` | `SP = 0xFFFF` (Empty) |
| `02_subroutine_call_ret.asm` | Simple CALL and RET doubling R0 from 7 to 14. | Halted | `R0=14 (0x0E)` | `SP = 0xFFFF` (Unwound) |
| `03_register_preservation.asm` | Callee-save convention: subroutine preserves R1 using PUSH/POP. | Halted | `R0=30, R1=0xAA` | `SP = 0xFFFF` (Preserved) |
| `04_nested_calls.asm` | Nested execution: `main -> func_a -> func_b`. | Halted | `R0=16 (1 + 10 + 5)` | `SP = 0xFFFF` (Unwound) |

### 6. I/O Devices & Consoles (`examples/io_devices/`)

| File | Description | Termination | Visible Device Output |
| :--- | :--- | :---: | :--- |
| `01_text_console_output.asm` | Outputs `"BitForge!"` to Port 0x01. | Halted | Text Console: `"BitForge!"` |
| `02_numeric_console_output.asm` | Outputs 0, 42, 100, 255 to Port 0x02. | Halted | Numeric Console: `"0 42 100 255"` |
| `03_virtual_led_patterns.asm` | Walking bit patterns, alternating, and 0xFF to Port 0x10. | Halted | LEDs: `0xFF` (all 8 illuminated) |
| `04_input_fifo_poll.asm` | Reading Port 0x00 when FIFO is empty. | Halted | `R0 = 0x00` (non-fatal empty poll) |

### 7. Algorithms (`examples/algorithms/`)

| File | Algorithm | Termination | Expected Registers | Notes |
| :--- | :--- | :---: | :--- | :--- |
| `01_multiplication_repeated_addition.asm` | 8-bit unsigned multiplication: 7 * 6 = 42. | Halted | `R0=42 (0x2A), R1=7, R2=0` | Deterministic loop |
| `02_division_repeated_subtraction.asm` | 8-bit unsigned division: 45 / 7. | Halted | `R0=6 (Quotient), R1=3 (Rem)` | 6 * 7 + 3 = 45 |
| `03_sum_first_n_integers.asm` | Triangular number: Sum(1..10) = 55. | Halted | `R0=55 (0x37), R1=0` | 10 * 11 / 2 = 55 |
| `04_fibonacci_sequence.asm` | Generates F(0)..F(8), writes to 0x8000, prints F(8). | Halted | `R0=21 (0x15)` | Num: `"21"`, Mem: `[0,1,1,2,3,5,8,13,21]` |
| `05_greatest_common_divisor.asm` | Euclidean GCD via repeated subtraction: GCD(48, 18). | Halted | `R0=6, R1=6` | GCD(48, 18) = 6 |

### 8. Debugger Demonstrations (`examples/debugger/`)

| File | Purpose / Scenario | Recommended Debug Action | Expected Observation |
| :--- | :--- | :--- | :--- |
| `01_stepping_sequential.asm` | Step-by-step instruction execution | Press <kbd>F10</kbd> repeatedly | Yellow arrow advances; registers highlight amber |
| `02_branch_taken_not_taken.asm` | Visualizing branch jump vs. fallthrough | Step past JZ instructions | Arrow jumps forward on taken, steps to next line on not taken |
| `03_breakpoint_target.asm` | Source line breakpoints | Click gutter on lines 34, 40, 46; press <kbd>F5</kbd> | Execution pauses cleanly BEFORE target line |
| `04_self_loop_budget.asm` | Budget exhaustion & pause responsiveness | Set Max Steps=50000; press <kbd>F5</kbd> | Stops with `BudgetExhausted`; GUI remains responsive |
| `05_disjoint_org_segments.asm` | Multi-segment memory image | Step through `CALL 0x0500` | PC jumps across segment boundary, source mapping tracks |
| `06_memory_inspection_step.asm` | Live memory viewer mutation | Switch to Memory tab at 0x8000; press <kbd>F10</kbd> | Bytes update sequentially: AA, BB, CC, DD |

---

## Intentional Fault Demonstrations (`examples/faults/`)

These programs are designed to verify CPU architectural fault detection and atomicity:

| File | Intended Architectural Fault | Faulting Address | Expected CPU State |
| :--- | :--- | :---: | :--- |
| `01_invalid_opcode_fault.asm` | `FaultType::InvalidOpcode` (emits raw byte 0x03) | `0x0200` | MachineState::Faulted |
| `02_stack_underflow_fault.asm` | `FaultType::StackUnderflow` (POP from SP=0xFFFF) | `0x0200` | MachineState::Faulted |
| `03_stack_overflow_fault.asm` | `FaultType::StackOverflow` (recursive CALL past 0xF000) | `0x0200` | MachineState::Faulted |
| `04_invalid_port_fault.asm` | `FaultType::InvalidPort` (OUT to unmapped port 0x30) | `0x0203` | MachineState::Faulted |

---

## Invalid Assembly Fixtures (`examples/invalid/`)

These fixtures test the two-pass assembler's diagnostic engine and are expected to **fail assembly**:

| File | Invalid Construct Tested | Expected Diagnostic Class |
| :--- | :--- | :--- |
| `01_unknown_mnemonic.asm` | Unrecognized instruction `FOOBAR R0` | `Unknown instruction mnemonic 'FOOBAR'` |
| `02_invalid_register.asm` | Non-existent register `R9` in `LDI R9, 10` | `Invalid destination register 'R9'` |
| `03_missing_operand.asm` | Missing comma / second operand `ADD R0` | `Expected ',' between operands in ADD` |
| `04_extra_operand.asm` | Extraneous token on line `NOP R0` | `Unexpected trailing token: 'R0'` |
| `05_duplicate_label.asm` | Defining symbol `loop_point:` twice | `Duplicate label definition: 'loop_point'` |
| `06_unresolved_label.asm` | Branching to nonexistent symbol | `Unresolved label: 'nonexistent_target'` |
| `07_literal_overflow.asm` | Immediate value 999 exceeding 8 bits | `Immediate value out of range [-128, 255]` |
| `08_overlapping_segments.asm` | Conflicting `.ORG 0x0200` blocks | `Overlapping segment emission at address 0x0200` |
| `09_reserved_stack_emission.asm` | Setting `.ORG 0xF000` into hardware stack | `Cannot set .ORG into reserved stack region` |
| `10_malformed_string.asm` | Unterminated string literal `.ASCII "unclosed` | `Unterminated string literal` |
