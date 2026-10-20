; ==============================================================================
; BitForge Example: Register Preservation Across Subroutines
; Category: Stack & Subroutines
; Description:
;   Demonstrates the callee-saved register convention.
;   The caller maintains crucial data in R1. The subroutine needs R1 as a
;   scratch register for an internal calculation. It pushes R1 on entry and
;   pops R1 before returning, ensuring the caller's state is completely preserved.
;
; Expected Results:
;   State: Halted
;   SP:    0xFFFF
;   R0:    30 (result calculated by subroutine: 10 + 20)
;   R1:    0xAA (caller's original value preserved intact)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI  R0, 10
    LDI  R1, 0xAA       ; Important caller data in R1
    CALL add_twenty
    HALT                ; Verify R0 == 30 and R1 == 0xAA

add_twenty:
    PUSH R1             ; Callee-save: preserve caller's R1 on stack
    LDI  R1, 20         ; Overwrite R1 with scratch value
    ADD  R0, R1         ; R0 += 20 (10 + 20 = 30)
    POP  R1             ; Restore caller's original R1
    RET
