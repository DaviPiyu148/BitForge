; ==============================================================================
; BitForge Invalid Fixture: Duplicate Symbol Definition
; Category: Invalid Assembly (Intentionally fails to assemble)
; Description:
;   Exercises the assembler's symbol table duplicate check.
;   Defining the same label twice in the same file produces an error.
;
; Expected Diagnostic:
;   Severity: Error
;   Message:  Duplicate symbol definition: 'loop_point'
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
loop_point:
    NOP

loop_point:
    HALT
