; ==============================================================================
; BitForge Fault Demonstration: Stack Overflow Fault
; Category: Runtime Faults
; Description:
;   Demonstrates the CPU architectural fault behavior when pushing data beyond
;   the hardware stack boundary (valid stack: 0xF000..0xFFFE).
;   Uses an infinite recursive CALL. Each CALL pushes a 2-byte return address.
;   After ~2,048 recursive calls, SP reaches 0xF000, and the subsequent push
;   triggers StackOverflow.
;
; Expected Results:
;   State:       Faulted
;   Fault Type:  StackOverflow
;   Faulting PC: 0x0200
;   Steps:       ~2048
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    CALL main           ; Recursive call until stack boundary (0xF000) is exceeded
    HALT
