; ==============================================================================
; BitForge Fault Demonstration: Invalid Port Fault
; Category: Runtime Faults
; Description:
;   Demonstrates the CPU architectural fault behavior when accessing an unmapped
;   or prohibited I/O port.
;   Attempts to output to Port 0x30, which is unmapped on the BitForge system bus.
;
; Expected Results:
;   State:       Faulted
;   Fault Type:  InvalidPort
;   Faulting PC: 0x0200
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 42
    OUT 0x30, R0        ; Port 0x30 is unmapped -> triggers InvalidPort fault
    HALT
