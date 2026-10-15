; ==============================================================================
; BitForge Example: Indexed Memory Access via IX
; Category: Memory
; Description:
;   Demonstrates the 16-bit Index Register (IX) and indexed memory instructions:
;   LDI16, STX, LDX, INX, and DEX.
;   Writes two bytes consecutively, steps IX backward and forward, and reads
;   them back.
;
; Expected Results:
;   State:  Halted
;   IX:     0x8001
;   R1:     0x11 (read from 0x8000)
;   R2:     0x22 (read from 0x8001)
;   Memory: 0x8000 == 0x11, 0x8001 == 0x22
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI16 IX, 0x8000    ; Initialize index pointer to 0x8000
    LDI   R0, 0x11
    STX   [IX], R0      ; memory[0x8000] = 0x11

    INX                 ; IX = 0x8001
    LDI   R0, 0x22
    STX   [IX], R0      ; memory[0x8001] = 0x22

    DEX                 ; IX = 0x8000
    LDX   R1, [IX]      ; R1 = memory[0x8000] = 0x11

    INX                 ; IX = 0x8001
    LDX   R2, [IX]      ; R2 = memory[0x8001] = 0x22

    HALT
