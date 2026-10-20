; ==============================================================================
; BitForge Invalid Fixture: Invalid Register Identifier
; Category: Invalid Assembly (Intentionally fails to assemble)
; Description:
;   Exercises the assembler's register validation diagnostic.
;   BitForge supports registers R0, R1, R2, R3 (and pointer registers IX, SP, PC).
;   Register R9 is non-existent.
;
; Expected Diagnostic:
;   Severity: Error
;   Message:  Invalid destination register 'R9' for LDI
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R9, 10
    HALT
