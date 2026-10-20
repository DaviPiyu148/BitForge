; ==============================================================================
; BitForge Example: Branch Taken vs. Not Taken Tracing
; Category: Debugger
; Description:
;   Demonstrates observing control flow decisions during single-stepping.
;   - First branch (JZ branch_1): Condition is TRUE (Zero flag set).
;     The yellow PC arrow jumps forward, skipping the dead line.
;   - Second branch (JZ branch_2): Condition is FALSE (Zero flag clear).
;     The yellow PC arrow falls through sequentially to the next line.
;
; How to Debug:
;   1. Assemble with F7.
;   2. Step with F10.
;   3. At JZ branch_1, observe the execution arrow jump directly to line 29.
;   4. At JZ branch_2, observe the execution arrow advance to line 35.
;
; Expected Results:
;   State: Halted
;   R0:    0x00
;   R1:    0x01
;   R2:    0x22 (indicates not-taken path executed)
;   R3:    0xAA (indicates taken path executed)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    ; --- Part 1: Branch TAKEN ---
    LDI R0, 5
    LDI R1, 5
    SUB R0, R1          ; R0 = 0, Z=1
    JZ  branch_1        ; Condition TRUE: jumps to branch_1

    LDI R2, 0xFF        ; Skipped (never executed)

branch_1:
    LDI R3, 0xAA        ; Jump target reached

    ; --- Part 2: Branch NOT TAKEN ---
    LDI R1, 1           ; R1 = 1 (Z=0)
    DEC R1              ; R1 = 0 (Z=1)
    INC R1              ; R1 = 1 (Z=0, non-zero)
    JZ  branch_2        ; Condition FALSE: falls through to next line

    LDI R2, 0x22        ; Executed on fallthrough
    JMP finish

branch_2:
    LDI R2, 0xEE        ; Skipped

finish:
    HALT
