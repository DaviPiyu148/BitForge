; ==============================================================================
; BitForge Example: Branch on Zero (JZ and JNZ)
; Category: Control Flow
; Description:
;   Demonstrates conditional branching based on the Zero (Z) flag.
;   Tests both branch-taken and branch-not-taken conditions for JZ and JNZ.
;
; Expected Results:
;   State: Halted
;   R0:    0x00
;   R1:    0x01
;   R2:    0xAA (confirms both branch-taken paths executed)
;   Flags: [Z=0, C=0, N=0, V=0]
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    ; 1. Produce Zero: 5 - 5 = 0 (Z=1)
    LDI R0, 5
    LDI R1, 5
    SUB R0, R1          ; R0 = 0, Z=1
    JZ  zero_taken

    LDI R2, 0xFF        ; Fallthrough error indicator
    HALT

zero_taken:
    ; 2. Produce Non-Zero: DEC 2 -> 1 (Z=0)
    LDI R1, 2
    DEC R1              ; R1 = 1, Z=0
    JNZ not_zero_taken

    LDI R2, 0xEE        ; Fallthrough error indicator
    HALT

not_zero_taken:
    LDI R2, 0xAA        ; Success indicator
    HALT
