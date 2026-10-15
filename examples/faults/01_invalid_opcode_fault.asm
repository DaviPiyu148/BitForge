; ==============================================================================
; BitForge Fault Demonstration: Invalid Opcode Fault
; Category: Runtime Faults
; Description:
;   Demonstrates the CPU architectural fault behavior when encountering an
;   unrecognized opcode byte.
;   Emits raw byte 0x03 (unallocated opcode) into code memory using .BYTE.
;
; Expected Results:
;   State:       Faulted
;   Fault Type:  InvalidOpcode
;   Faulting PC: 0x0200
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    .BYTE 0x03          ; Unallocated opcode byte (triggers InvalidOpcode fault)
    HALT
