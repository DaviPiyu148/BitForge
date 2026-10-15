; ==============================================================================
; BitForge Example: 8-Bit Multiplication via Repeated Addition
; Category: Algorithms
; Description:
;   Computes the product of two 8-bit unsigned integers (Multiplicand * Multiplier)
;   using repeated addition.
;   Computes 7 * 6 = 42 (0x2A).
;
; Expected Results:
;   State: Halted
;   R0:    42 (0x2A, product)
;   R1:    7  (multiplicand)
;   R2:    0  (multiplier decremented to zero)
;   Flags: [Z=1, C=0, N=0, V=0]
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 0           ; Product accumulator = 0
    LDI R1, 7           ; Multiplicand = 7
    LDI R2, 6           ; Multiplier = 6

multiply_loop:
    ADD R0, R1          ; R0 += 7
    DEC R2              ; Multiplier--
    JNZ multiply_loop   ; Repeat until multiplier is 0

    HALT
