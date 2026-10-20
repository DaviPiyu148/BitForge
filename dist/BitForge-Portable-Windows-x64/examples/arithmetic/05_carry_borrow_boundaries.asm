; ==============================================================================
; BitForge Example: Carry and Borrow Flag Boundaries
; Category: Arithmetic
; Description:
;   Demonstrates unsigned overflow and underflow carry flag behavior per v1.7:
;     - ADD: C=1 when operation produces an unsigned carry-out (> 0xFF).
;     - SUB: C=1 when no borrow is required (left >= right);
;            C=0 when an unsigned borrow occurs (left < right).
;
; Expected Results:
;   State: Halted
;   R0:    0x00 (from 0xFF + 0x01 carry wrap)
;   R1:    0x01
;   R2:    0xFE (from 0x04 - 0x06 borrow wrap)
;   R3:    0x06
;   Flags: [Z=0, C=0, N=1, V=0] (C=0 indicates borrow from final subtraction)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    ; 1. Addition generating unsigned Carry (0xFF + 1 = 0x100 -> 0x00, C=1, Z=1)
    LDI R0, 0xFF
    LDI R1, 0x01
    ADD R0, R1

    ; 2. Subtraction generating unsigned Borrow (4 - 6 = -2 -> 0xFE, C=0 borrow, N=1, Z=0)
    LDI R2, 0x04
    LDI R3, 0x06
    SUB R2, R3

    HALT
