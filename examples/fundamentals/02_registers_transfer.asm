; ==============================================================================
; BitForge Example: Register Loading and Transfer
; Category: Fundamentals
; Description:
;   Demonstrates loading 8-bit immediate values into registers (LDI) and
;   transferring data between general-purpose registers (MOV).
;   Exercises R0, R1, R2, and R3.
;
; Expected Results:
;   State: Halted
;   PC:    0x0210
;   R0:    0x99 (transferred from R3)
;   R1:    0x42 (transferred from R0)
;   R2:    0x99 (loaded immediate)
;   R3:    0x99 (transferred from R2)
;   Flags: [Z=0, C=0, N=0, V=0] (LDI and MOV do not alter flags)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 0x42        ; R0 = 0x42
    MOV R1, R0          ; R1 = R0 = 0x42
    LDI R2, 0x99        ; R2 = 0x99
    MOV R3, R2          ; R3 = R2 = 0x99
    MOV R0, R3          ; R0 = R3 = 0x99
    HALT
