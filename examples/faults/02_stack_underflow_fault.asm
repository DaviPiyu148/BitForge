; ==============================================================================
; BitForge Fault Demonstration: Stack Underflow Fault
; Category: Runtime Faults
; Description:
;   Demonstrates the CPU architectural fault behavior when attempting to POP
;   from an empty stack.
;   Per ADR-001, the stack starts at SP = 0xFFFF (empty sentinel).
;   Attempting POP from 0xFFFF immediately triggers StackUnderflow.
;
; Expected Results:
;   State:       Faulted
;   Fault Type:  StackUnderflow
;   Faulting PC: 0x0200
;   SP:          0xFFFF (preserved atomically per architectural fault semantics)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    POP R0              ; Attempt POP from empty stack (SP == 0xFFFF)
    HALT
