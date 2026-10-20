; ==============================================================================
; BitForge Example: Bitwise Logic (AND, OR, XOR, NOT)
; Category: Arithmetic
; Description:
;   Demonstrates the four bitwise instructions.
;   Performs bit masking (AND), bit setting (OR), bit flipping (XOR),
;   and bitwise inversion (NOT). Bitwise instructions always clear C and V.
;
; Expected Results:
;   State: Halted
;   R0:    0x5A (result of bitwise operations)
;   R1:    0x0F
;   R2:    0x50
;   R3:    0xFF
;   Flags: [Z=0, C=0, N=0, V=0]
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 0xAA        ; R0 = 0b10101010
    LDI R1, 0x0F        ; R1 = 0b00001111
    AND R0, R1          ; R0 = 0xAA & 0x0F = 0x0A (0b00001010)

    LDI R2, 0x50        ; R2 = 0b01010000
    OR  R0, R2          ; R0 = 0x0A | 0x50 = 0x5A (0b01011010)

    LDI R3, 0xFF        ; R3 = 0b11111111
    XOR R0, R3          ; R0 = 0x5A ^ 0xFF = 0xA5 (0b10100101)

    NOT R0              ; R0 = ~0xA5 = 0x5A (0b01011010)
    HALT
