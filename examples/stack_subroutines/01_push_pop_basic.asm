; ==============================================================================
; BitForge Example: Basic Stack Push and Pop (ADR-001)
; Category: Stack & Subroutines
; Description:
;   Demonstrates LIFO (Last-In, First-Out) stack behavior using PUSH and POP.
;   Per ADR-001:
;     - Empty stack sentinel: SP = 0xFFFF.
;     - Stack grows downward: PUSH decrements SP, then writes byte at new SP.
;     - POP reads byte at SP, then increments SP.
;   Pushes 0x11, 0x22, 0x33 and pops them back into different registers in reverse order.
;
; Expected Results:
;   State: Halted
;   SP:    0xFFFF (clean empty stack sentinel)
;   R1:    0x33 (first popped: top-of-stack)
;   R2:    0x22 (second popped)
;   R3:    0x11 (third popped: bottom-of-stack)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI  R0, 0x11
    PUSH R0             ; SP: 0xFFFF -> 0xFFFE, memory[0xFFFE] = 0x11

    LDI  R0, 0x22
    PUSH R0             ; SP: 0xFFFE -> 0xFFFD, memory[0xFFFD] = 0x22

    LDI  R0, 0x33
    PUSH R0             ; SP: 0xFFFD -> 0xFFFC, memory[0xFFFC] = 0x33

    POP  R1             ; R1 = 0x33, SP: 0xFFFC -> 0xFFFD
    POP  R2             ; R2 = 0x22, SP: 0xFFFD -> 0xFFFE
    POP  R3             ; R3 = 0x11, SP: 0xFFFE -> 0xFFFF

    HALT
