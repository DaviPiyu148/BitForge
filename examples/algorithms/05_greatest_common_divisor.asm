; ==============================================================================
; BitForge Example: Greatest Common Divisor (Euclidean Algorithm)
; Category: Algorithms
; Description:
;   Computes the Greatest Common Divisor (GCD) of two positive integers
;   using the Euclidean algorithm via repeated subtraction.
;   Per v1.7, CMP sets C=0 when left < right (borrow required).
;   Computes GCD(48, 18) = 6.
;
; Expected Results:
;   State: Halted
;   R0:    6 (Greatest Common Divisor)
;   R1:    6
;   Flags: [Z=1, C=1, N=0, V=0] (C=1 because 6 >= 6 on final comparison)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 48          ; First integer (a = 48)
    LDI R1, 18          ; Second integer (b = 18)

gcd_loop:
    CMP R0, R1          ; Compare a with b (R0 - R1)
    JZ  gcd_done        ; If a == b, GCD is found
    JNC b_greater       ; If a < b (C=0 borrow required), branch to subtract a from b

    SUB R0, R1          ; a > b (C=1): a = a - b
    JMP gcd_loop

b_greater:
    SUB R1, R0          ; b > a: b = b - a
    JMP gcd_loop

gcd_done:
    HALT                ; R0 holds the GCD (6)
