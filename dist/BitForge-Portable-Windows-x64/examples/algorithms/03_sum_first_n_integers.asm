; ==============================================================================
; BitForge Example: Sum of First N Positive Integers
; Category: Algorithms
; Description:
;   Computes the triangular number Sum(N) = 1 + 2 + 3 + ... + N for N = 10.
;   Formula: 10 * 11 / 2 = 55 (0x37).
;
; Expected Results:
;   State: Halted
;   R0:    55 (0x37, accumulated sum)
;   R1:    0  (loop counter exhausted)
;   Flags: [Z=1, C=0, N=0, V=0]
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 0           ; Sum accumulator = 0
    LDI R1, 10          ; N = 10

sum_loop:
    ADD R0, R1          ; Sum += N
    DEC R1              ; N--
    JNZ sum_loop        ; Repeat until N == 0

    HALT
