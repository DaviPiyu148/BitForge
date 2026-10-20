; BitForge Example 02: Virtual I/O Demonstration
; Demonstrates text output (Port 0x01), numeric output (Port 0x02), and LEDs (Port 0x10)

.ORG 0x0200
.ENTRY main

main:
    ; Text console: Output "HI!"
    LDI R0, 'H'
    OUT 0x01, R0
    LDI R0, 'I'
    OUT 0x01, R0
    LDI R0, '!'
    OUT 0x01, R0

    ; Numeric console: Output 42
    LDI R0, 42
    OUT 0x02, R0

    ; Virtual LEDs: Set pattern 0x55 (0b01010101)
    LDI R0, 0x55
    OUT 0x10, R0

    HALT
