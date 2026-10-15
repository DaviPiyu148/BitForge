; ==============================================================================
; BitForge Example: Virtual LED Patterns (Port 0x10)
; Category: I/O Devices
; Description:
;   Demonstrates controlling the 8 virtual LEDs mapped to Port 0x10.
;   Sequentially outputs walking bit patterns (0x01, 0x02, 0x04, 0x08, 0x10,
;   0x20, 0x40, 0x80), alternating patterns (0x55, 0xAA), and all LEDs ON (0xFF).
;   Ideal for stepping with F10 in the GUI to see each LED toggle.
;
; Expected Results:
;   State:     Halted
;   Final LED: 0xFF (0b11111111 - all 8 LEDs illuminated)
;   R0:        0xFF
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    ; Walking single-bit patterns
    LDI R0, 0x01
    OUT 0x10, R0
    LDI R0, 0x02
    OUT 0x10, R0
    LDI R0, 0x04
    OUT 0x10, R0
    LDI R0, 0x08
    OUT 0x10, R0
    LDI R0, 0x10
    OUT 0x10, R0
    LDI R0, 0x20
    OUT 0x10, R0
    LDI R0, 0x40
    OUT 0x10, R0
    LDI R0, 0x80
    OUT 0x10, R0

    ; Alternating checkerboard patterns
    LDI R0, 0x55        ; 0b01010101
    OUT 0x10, R0
    LDI R0, 0xAA        ; 0b10101010
    OUT 0x10, R0

    ; All LEDs ON
    LDI R0, 0xFF        ; 0b11111111
    OUT 0x10, R0

    HALT
