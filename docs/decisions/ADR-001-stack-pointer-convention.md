# ADR-001: Stack Pointer Convention and Subroutine Interface

## Status
Accepted

## Context
In `BitForge_Architecture_Specification_v1.7.md`, Section 5.2 contained an informal statement that `SP` "points to the next free stack location", whereas Sections 5.3, 5.5, 5.6, and 6.1 specified:
- `SP = 0xFFFF` as the empty-stack sentinel.
- Reserved stack storage confined to `0xF000` through `0xFFFE`.
- `PUSH Rs` decrements `SP` before storing the byte into `memory[SP]`.
- `POP Rd` reads the byte from `memory[SP]` before incrementing `SP`.
- `CALL addr16` decrements `SP` by 2, storing return address in big-endian order (`memory[SP - 2] = ret_hi`, `memory[SP - 1] = ret_lo`).
- `RET` loads `ret_hi = memory[SP]`, `ret_lo = memory[SP + 1]`, and increments `SP` by 2.

This ADR records the canonical convention implemented in BitForge Phase 2 and frozen for subsequent phases.

## Decision

The BitForge execution engine adopts the following strict architectural rules for the stack pointer:

1. **Empty Stack Sentinel:**
   - On CPU reset (`reset()`), `SP` is initialized to `0xFFFF`.
   - The address `0xFFFF` is an empty-stack sentinel and never stores stack data.

2. **Occupied Top-of-Stack Semantics:**
   - Whenever the stack is non-empty (`SP < 0xFFFF`), `SP` points directly to the currently occupied top-of-stack byte.

3. **Valid Stack Storage Address Range:**
   - Stack storage is strictly confined to addresses `0xF000` through `0xFFFE` inclusive (4,095 bytes maximum capacity).
   - Assembler directives reject program emissions at or above `0xF000` to prevent collision with the stack region.

4. **One-Byte PUSH (`PUSH Rs`, opcode `0x62`):**
   - **Pre-validation:** Requires `SP > 0xF000`. If `SP <= 0xF000`, the CPU aborts immediately with `FaultType::StackOverflow`.
   - **Commit:** Decrements `SP = (SP - 1) & 0xFFFF`, then stores register `Rs` at `memory[SP]`.
   - The first push from empty transitions `SP` from `0xFFFF` to `0xFFFE`.
   - The last valid push transitions `SP` from `0xF001` to `0xF000`.

5. **One-Byte POP (`POP Rd`, opcode `0x63`):**
   - **Pre-validation:** Requires `SP < 0xFFFF`. If `SP >= 0xFFFF`, the CPU aborts immediately with `FaultType::StackUnderflow`.
   - **Commit:** Reads byte from `memory[SP]`, increments `SP = (SP + 1) & 0xFFFF`, and writes value to `Rd`.
   - Popping the final byte at `0xFFFE` restores `SP` to the empty sentinel `0xFFFF`.

6. **Two-Byte Subroutine Call (`CALL addr16`, opcode `0x60`):**
   - **Pre-validation:** Requires `SP >= 0xF002`. If `SP < 0xF002`, the CPU aborts immediately with `FaultType::StackOverflow`.
   - **Return Address:** Computed as `ret_addr = (start_pc + 3) & 0xFFFF`.
   - **Big-Endian Stack Storage:**
     - `ret_hi = (ret_addr >> 8) & 0xFF`
     - `ret_lo = ret_addr & 0xFF`
     - `memory[SP - 2] = ret_hi`
     - `memory[SP - 1] = ret_lo`
     - `SP = SP - 2`
     - `PC = addr16`
   - At the new `SP`, `memory[SP]` holds `ret_hi` and `memory[SP + 1]` holds `ret_lo`, adhering to big-endian word layout.

7. **Two-Byte Subroutine Return (`RET`, opcode `0x61`):**
   - **Pre-validation:** Requires `SP <= 0xFFFD`. If `SP > 0xFFFD`, the CPU aborts immediately with `FaultType::StackUnderflow`.
   - **Commit:**
     - `ret_hi = memory[SP]`
     - `ret_lo = memory[SP + 1]`
     - `SP = SP + 2`
     - `PC = (ret_hi << 8) | ret_lo`

8. **Fault Atomicity and Universal PC Preservation:**
   - Any stack overflow or underflow violation triggers an immediate CPU fault and halts execution in `MachineState::Faulted`.
   - `registers_.pc` remains at the starting address of the faulting instruction.
   - All registers `R0`..`R3`, `IX`, `SP`, status flags `Z`, `C`, `N`, `V`, and memory remain unchanged.

## Consequences
- The stack convention is consistent across all CPU operations, unit tests, and assembler integration benchmarks.
- No changes to CPU runtime behavior or specifications v1.0 through v1.7 are required.
