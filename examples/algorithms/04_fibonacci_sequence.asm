; ==============================================================================
; BitForge Example: Fibonacci Sequence Generation
; Category: Algorithms
; Description:
;   Generates the first 9 Fibonacci numbers:
;     F(0)=0, F(1)=1, F(2)=1, F(3)=2, F(4)=3, F(5)=5, F(6)=8, F(7)=13, F(8)=21
;   Stores each number sequentially in memory starting at address 0x8000
;   and outputs the final value (21) to Decimal Console Port 0x02.
;
; Expected Results:
;   State:          Halted
;   R0:             21 (0x15, F(8))
;   Numeric Output: "21"
;   Memory at 0x8000..0x8008: [0, 1, 1, 2, 3, 5, 8, 13, 21]
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI16 IX, 0x8000    ; Base address for Fibonacci table in RAM

    ; F(0) = 0
    LDI   R0, 0
    STX   [IX], R0
    INX

    ; F(1) = 1
    LDI   R1, 1
    STX   [IX], R1
    INX

    ; Loop counter for remaining 7 terms (F(2) through F(8))
    LDI   R3, 7

fib_loop:
    LDI   R2, 0
    ADD   R2, R0        ; R2 = R0
    ADD   R2, R1        ; R2 = R0 + R1 (next Fibonacci term)
    STX   [IX], R2      ; Store term in memory table
    INX                 ; Advance memory pointer

    MOV   R0, R1        ; Shift window: R0 = previous term
    MOV   R1, R2        ; Shift window: R1 = current term

    DEC   R3
    JNZ   fib_loop

    ; Output final F(8) result to numeric console
    MOV   R0, R1        ; R0 = 21
    OUT   0x02, R0

    HALT
