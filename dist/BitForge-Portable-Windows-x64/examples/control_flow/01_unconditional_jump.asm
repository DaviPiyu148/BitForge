; ==============================================================================
; BitForge Example: Unconditional Jump
; Category: Control Flow
; Description:
;   Demonstrates the JMP instruction.
;   Jumps unconditionally over an unwanted instruction to reach the target label.
;
; Expected Results:
;   State: Halted
;   R0:    1 (set before jump)
;   R1:    2 (set at jump target)
;   R2:    0 (trap instruction loading 99 was skipped)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 1
    JMP target

trap:
    LDI R2, 99          ; Skipped by JMP

target:
    LDI R1, 2
    HALT
