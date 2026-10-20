; ==============================================================================
; BitForge Example: Increment and Decrement
; Category: Arithmetic
; Description:
;   Demonstrates INC and DEC instructions.
;   Verifies that INC and DEC update Zero (Z) and Negative (N) flags while
;   strictly preserving the Carry (C) flag per the v1.7 specification.
;
; Expected Results:
;   State: Halted
;   R0:    0x00 (wrapped past 0xFF with Z=1, but C preserved)
;   R1:    0xFF (decremented past 0x00 with N=1)
;   Flags: [Z=0, C=0, N=1, V=0]
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 0xFE        ; R0 = 254
    INC R0              ; R0 = 255 (0xFF, N=1, Z=0)
    INC R0              ; R0 = 0   (0x00, Z=1, N=0, C is preserved as 0)

    LDI R1, 0x01        ; R1 = 1
    DEC R1              ; R1 = 0   (0x00, Z=1)
    DEC R1              ; R1 = 255 (0xFF, N=1, Z=0)
    HALT
