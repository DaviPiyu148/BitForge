; ==============================================================================
; BitForge Example: Breakpoint Testing Target
; Category: Debugger
; Description:
;   Specifically structured for setting source breakpoints in the GUI gutter.
;   Includes three designated checkpoint instructions separated by work blocks.
;
; Recommended Breakpoint Lines:
;   - Line 34 (checkpoint_1: LDI R1, 0x11)
;   - Line 40 (checkpoint_2: LDI R2, 0x22)
;   - Line 46 (checkpoint_3: LDI R3, 0x33)
;
; How to Debug:
;   1. Assemble with F7.
;   2. Click line numbers 34 and 40 in the gutter (red dot appears).
;   3. Click Run (F5). Execution halts at line 34 BEFORE executing LDI R1, 0x11.
;   4. Click Continue (F8). Execution resumes and stops at line 40.
;
; Expected Results on Full Execution:
;   State: Halted
;   R0:    0x01
;   R1:    0x11
;   R2:    0x22
;   R3:    0x33
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 0x01

checkpoint_1:
    LDI R1, 0x11        ; Line 34: Recommended Breakpoint 1

    NOP
    NOP

checkpoint_2:
    LDI R2, 0x22        ; Line 40: Recommended Breakpoint 2

    NOP
    NOP

checkpoint_3:
    LDI R3, 0x33        ; Line 46: Recommended Breakpoint 3

    HALT
