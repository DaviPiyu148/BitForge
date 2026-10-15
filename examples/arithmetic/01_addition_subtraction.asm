; ==============================================================================
; BitForge Example: Addition and Subtraction
; Category: Arithmetic
; Description:
;   Demonstrates basic integer arithmetic using ADD and SUB.
;   Computes (50 + 25) - 15 = 60.
;
; Expected Results:
;   State: Halted
;   R0:    0x3C (60 decimal)
;   R1:    0x19 (25 decimal)
;   R2:    0x0F (15 decimal)
;   Flags: [Z=0, C=1, N=0, V=0]
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 50          ; R0 = 50 (0x32)
    LDI R1, 25          ; R1 = 25 (0x19)
    ADD R0, R1          ; R0 = 50 + 25 = 75 (0x4B)
    LDI R2, 15          ; R2 = 15 (0x0F)
    SUB R0, R2          ; R0 = 75 - 15 = 60 (0x3C)
    HALT
