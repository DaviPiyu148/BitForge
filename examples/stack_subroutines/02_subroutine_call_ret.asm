; ==============================================================================
; BitForge Example: Subroutine Call and Return
; Category: Stack & Subroutines
; Description:
;   Demonstrates CALL and RET instructions.
;   CALL pushes the 16-bit return address (big-endian: high byte first, low byte second)
;   onto the hardware stack and jumps to the subroutine. RET pops the 16-bit address
;   and restores PC to resume sequential execution in main.
;
; Expected Results:
;   State: Halted
;   SP:    0xFFFF (stack fully unwound to empty)
;   R0:    14 (0x0E, 7 doubled by subroutine)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI  R0, 7          ; Initial value
    CALL double_val     ; Call subroutine
    HALT                ; Execution resumes here after RET

double_val:
    ADD  R0, R0         ; R0 = R0 + R0 = 14
    RET                 ; Return to caller
