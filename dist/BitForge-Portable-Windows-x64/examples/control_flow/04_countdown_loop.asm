; ==============================================================================
; BitForge Example: Countdown Loop
; Category: Control Flow
; Description:
;   Demonstrates a counted loop using DEC and JNZ.
;   Initializes counter R0 to 5 and accumulates the sum (5 + 4 + 3 + 2 + 1)
;   into R1. When the counter reaches zero, JNZ falls through to HALT.
;
; Expected Results:
;   State: Halted
;   R0:    0x00 (counter exhausted)
;   R1:    0x0F (15 decimal, accumulated sum)
;   Flags: [Z=1, C=0, N=0, V=0]
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 5           ; Loop counter: 5 iterations
    LDI R1, 0           ; Sum accumulator

loop:
    ADD R1, R0          ; R1 += R0
    DEC R0              ; Decrement counter (sets Z flag when 0)
    JNZ loop            ; Repeat while R0 != 0

    HALT
