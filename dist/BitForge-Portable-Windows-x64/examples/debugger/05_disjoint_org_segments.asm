; ==============================================================================
; BitForge Example: Disjoint Memory Segments (.ORG)
; Category: Debugger
; Description:
;   Demonstrates multiple non-contiguous memory segments declared using .ORG.
;   - Segment 1 starts at 0x0200 (main code).
;   - Segment 2 starts at 0x0500 (remote subroutine helper).
;   Verifies multi-segment program image loading, source-level line mapping
;   across disparate address ranges, and jumping/calling across memory gaps.
;
; Expected Results:
;   State: Halted
;   R0:    10 (initial value)
;   R1:    25 (computed by remote subroutine in 0x0500 segment)
;   SP:    0xFFFF (return address unwound cleanly)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI  R0, 10
    CALL remote_helper   ; Call into disjoint segment at 0x0500
    HALT

; --- Disjoint Code Segment ---
.ORG 0x0500

remote_helper:
    LDI  R1, 15
    ADD  R1, R0          ; R1 = 15 + 10 = 25
    RET
