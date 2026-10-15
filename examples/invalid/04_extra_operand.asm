; ==============================================================================
; BitForge Invalid Fixture: Extra Unexpected Operand
; Category: Invalid Assembly (Intentionally fails to assemble)
; Description:
;   Exercises the assembler's end-of-statement syntax validation.
;   NOP takes no operands; passing an operand produces a syntax error.
;
; Expected Diagnostic:
;   Severity: Error
;   Message:  Unexpected token in statement: 'R0'
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    NOP R0
    HALT
