; ==============================================================================
; BitForge Example: Direct Load and Store
; Category: Memory
; Description:
;   Demonstrates direct memory access using ST [addr16], Rs and LD Rd, [addr16].
;   Writes an 8-bit value to Extended Data RAM at address 0x8050, clears the
;   source register, and reads the byte back into a different register.
;
; Expected Results:
;   State:  Halted
;   R0:     0x00 (cleared)
;   R1:     0x7E (loaded from memory)
;   Memory: byte at 0x8050 == 0x7E
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 0x7E
    ST  [0x8050], R0    ; memory[0x8050] = 0x7E

    LDI R0, 0x00        ; Clear R0

    LD  R1, [0x8050]    ; R1 = memory[0x8050] = 0x7E
    HALT
