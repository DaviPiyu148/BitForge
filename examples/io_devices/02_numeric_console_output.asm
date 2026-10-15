; ==============================================================================
; BitForge Example: Numeric Console Output (Port 0x02)
; Category: I/O Devices
; Description:
;   Demonstrates writing unsigned 8-bit integers to Virtual Output Port 0x02.
;   Each OUT instruction prints the register value as an unsigned decimal string,
;   separated by spaces. Outputs 0, 42, 100, and 255.
;
; Expected Results:
;   State:          Halted
;   Numeric Output: "0 42 100 255"
;   R0:             255 (0xFF)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 0
    OUT 0x02, R0

    LDI R0, 42
    OUT 0x02, R0

    LDI R0, 100
    OUT 0x02, R0

    LDI R0, 255
    OUT 0x02, R0

    HALT
