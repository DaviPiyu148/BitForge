; ==============================================================================
; BitForge Example: Comparison and Flags
; Category: Arithmetic
; Description:
;   Demonstrates the CMP instruction.
;   Per BitForge v1.7, CMP performs internal subtraction (R1 - R2) and updates
;   all flags (Z, C, N, V) without modifying either register:
;     - Z=1 if operands are equal.
;     - C=1 (no-borrow) if left >= right; C=0 (borrow) if left < right.
;     - N=1 if bit 7 of the difference is 1.
;
; Expected Results:
;   State: Halted
;   R0:    10 (unchanged)
;   R1:    20 (unchanged)
;   Flags: [Z=0, C=0, N=1, V=0] (10 < 20 produces borrow C=0, and negative N=1)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 10          ; First operand
    LDI R1, 20          ; Second operand
    CMP R0, R1          ; 10 - 20 = -10 (0xF6). Flags: C=0 (borrow), N=1, Z=0
    HALT
