; ==============================================================================
; BitForge Example: NOP and HALT Fundamentals
; Category: Fundamentals
; Description:
;   Demonstrates the simplest possible sequential execution.
;   Three NOP (No Operation) instructions advance the Program Counter (PC)
;   without modifying any registers or CPU status flags. Execution terminates
;   cleanly on HALT.
;
; Expected Results:
;   State: Halted
;   PC:    0x0204
;   R0-R3: 0x00
;   Flags: [Z=0, C=0, N=0, V=0]
;   Steps: 4
; ==============================================================================

.ORG 0x0200
.ENTRY start

start:
    NOP                 ; Step 1: PC 0x0200 -> 0x0201, no state change
    NOP                 ; Step 2: PC 0x0201 -> 0x0202, no state change
    NOP                 ; Step 3: PC 0x0202 -> 0x0203, no state change
    HALT                ; Step 4: PC 0x0203 -> 0x0204, CPU enters Halted state
