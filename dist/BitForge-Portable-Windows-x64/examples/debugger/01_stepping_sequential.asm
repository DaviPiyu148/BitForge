; ==============================================================================
; BitForge Example: Sequential Single-Stepping
; Category: Debugger
; Description:
;   Designed specifically for practicing single-step execution with F10.
;   Each sequential instruction modifies a different register or flag, allowing
;   clear observation of register-change highlighting in the GUI.
;
; How to Debug:
;   1. Assemble with F7.
;   2. Press F10 repeatedly to step through one instruction at a time.
;   3. Observe the yellow execution arrow (▶) advancing down each line.
;   4. Watch modified register values highlight in amber/yellow.
;
; Expected Results:
;   State: Halted
;   R0:    3 (1 + 2)
;   R1:    2
;   R2:    7 (3 + 4)
;   R3:    4
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 1           ; Step 1: R0 becomes 1
    LDI R1, 2           ; Step 2: R1 becomes 2
    LDI R2, 3           ; Step 3: R2 becomes 3
    LDI R3, 4           ; Step 4: R3 becomes 4
    ADD R0, R1          ; Step 5: R0 becomes 3
    ADD R2, R3          ; Step 6: R2 becomes 7
    HALT                ; Step 7: Execution halts
