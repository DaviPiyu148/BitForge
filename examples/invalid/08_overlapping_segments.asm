; ==============================================================================
; BitForge Invalid Fixture: Overlapping Memory Segments
; Category: Invalid Assembly (Intentionally fails to assemble)
; Description:
;   Exercises the assembler's memory overlap detector.
;   Emits code at address 0x0200, and then sets .ORG back to 0x0200 to emit
;   conflicting bytes to the same memory addresses.
;
; Expected Diagnostic:
;   Severity: Error
;   Message:  Overlapping segment emission at address 0x0200
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 1

; Second segment overlaps initial segment
.ORG 0x0200
    LDI R1, 2
    HALT
