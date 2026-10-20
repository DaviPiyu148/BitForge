; ==============================================================================
; BitForge Example: 8-Bit Division via Repeated Subtraction
; Category: Algorithms
; Description:
;   Computes integer quotient and remainder for Dividend / Divisor using
;   repeated subtraction.
;   Per v1.7, CMP sets C=1 when left >= right (no borrow), and C=0 when left < right (borrow).
;   Loop terminates with JNC when remainder < divisor.
;   Computes 45 / 7:
;     45 = (6 * 7) + 3
;     Quotient  = 6
;     Remainder = 3
;
; Expected Results:
;   State: Halted
;   R0:    6 (Quotient)
;   R1:    3 (Remainder)
;   R2:    7 (Divisor)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 0           ; Quotient = 0
    LDI R1, 45          ; Dividend (will become remainder)
    LDI R2, 7           ; Divisor

div_loop:
    CMP R1, R2          ; Compare remainder with divisor (R1 - R2)
    JNC div_done        ; If R1 < R2 (borrow required, C=0): finished

    SUB R1, R2          ; R1 >= R2: R1 -= R2 (subtract divisor)
    INC R0              ; Quotient++
    JMP div_loop

div_done:
    HALT
