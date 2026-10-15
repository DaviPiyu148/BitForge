; ==============================================================================
; BitForge Example: Branch on Carry (JC and JNC)
; Category: Control Flow
; Description:
;   Demonstrates conditional branching based on the Carry (C) flag.
;   Tests both JC (jump if carry set) and JNC (jump if carry clear).
;
; Expected Results:
;   State: Halted
;   R0:    0x20
;   R1:    0xBB (confirms both branch paths executed)
;   Flags: [Z=0, C=0, N=0, V=0]
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    ; 1. Generate Carry: 0xFF + 1 = 0x00 with C=1
    LDI R0, 0xFF
    LDI R1, 0x01
    ADD R0, R1          ; C=1
    JC  carry_taken

    LDI R1, 0xFF        ; Fallthrough error indicator
    HALT

carry_taken:
    ; 2. Clear Carry: 0x10 + 0x10 = 0x20 with C=0
    LDI R0, 0x10
    LDI R1, 0x10
    ADD R0, R1          ; C=0
    JNC no_carry_taken

    LDI R1, 0xEE        ; Fallthrough error indicator
    HALT

no_carry_taken:
    LDI R1, 0xBB        ; Success indicator
    HALT
