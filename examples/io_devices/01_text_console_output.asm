; ==============================================================================
; BitForge Example: Text Console Output (Port 0x01)
; Category: I/O Devices
; Description:
;   Demonstrates writing ASCII characters to Virtual Output Port 0x01.
;   Outputs the string "BitForge!" character by character.
;
; Expected Results:
;   State:       Halted
;   Text Output: "BitForge!"
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 'B'
    OUT 0x01, R0

    LDI R0, 'i'
    OUT 0x01, R0

    LDI R0, 't'
    OUT 0x01, R0

    LDI R0, 'F'
    OUT 0x01, R0

    LDI R0, 'o'
    OUT 0x01, R0

    LDI R0, 'r'
    OUT 0x01, R0

    LDI R0, 'g'
    OUT 0x01, R0

    LDI R0, 'e'
    OUT 0x01, R0

    LDI R0, '!'
    OUT 0x01, R0

    HALT
