; ==============================================================================
; BitForge Invalid Fixture: Immediate Numeric Literal Overflow
; Category: Invalid Assembly (Intentionally fails to assemble)
; Description:
;   Exercises the assembler's 8-bit immediate range validation.
;   LDI expects an immediate in [-128, 255]. Specifying 999 produces an error.
;
; Expected Diagnostic:
;   Severity: Error
;   Message:  Immediate value out of range [-128, 255] for LDI: 999
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 999
    HALT
