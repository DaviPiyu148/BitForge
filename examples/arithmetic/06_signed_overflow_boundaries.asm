; ==============================================================================
; BitForge Example: Signed Overflow Boundaries
; Category: Arithmetic
; Description:
;   Demonstrates two's-complement signed overflow flag (V=1) behavior.
;   Case A: Positive overflow: 0x7F (+127) + 1 = 0x80 (-128) -> V=1, N=1
;   Case B: Negative underflow: 0x80 (-128) - 1 = 0x7F (+127) -> V=1, N=0
;
; Expected Results:
;   State: Halted
;   R0:    0x80 (positive overflow result)
;   R1:    0x01
;   R2:    0x7F (negative underflow result)
;   R3:    0x01
;   Flags: [Z=0, C=1, N=0, V=1] (V=1 signed overflow from final subtraction)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    ; Part A: +127 + 1 = -128 (Positive overflow: V=1, N=1, C=0)
    LDI R0, 0x7F
    LDI R1, 0x01
    ADD R0, R1

    ; Part B: -128 - 1 = +127 (Negative underflow: V=1, N=0, C=1)
    LDI R2, 0x80
    LDI R3, 0x01
    SUB R2, R3

    HALT
