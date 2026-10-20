; ==============================================================================
; BitForge Example: Nested Subroutine Calls
; Category: Stack & Subroutines
; Description:
;   Demonstrates nested subroutine execution (main -> func_a -> func_b).
;   Shows that the LIFO stack properly maintains multiple return address frames
;   without corruption, returning sequentially back through each caller to main.
;
; Expected Results:
;   State: Halted
;   SP:    0xFFFF (all return address frames unwound cleanly)
;   R0:    16 (0x10: initial 1 + func_b 10 + func_a 5)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI  R0, 1          ; Start with 1
    CALL func_a         ; Call first subroutine
    HALT                ; Expect R0 == 1 + 10 + 5 = 16

func_a:
    CALL func_b         ; Nested call to second subroutine
    LDI  R1, 5
    ADD  R0, R1         ; R0 += 5 (11 + 5 = 16)
    RET                 ; Return to main

func_b:
    LDI  R2, 10
    ADD  R0, R2         ; R0 += 10 (1 + 10 = 11)
    RET                 ; Return to func_a
