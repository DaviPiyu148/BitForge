; ==============================================================================
; BitForge Invalid Fixture: Missing Operand
; Category: Invalid Assembly (Intentionally fails to assemble)
; Description:
;   Exercises the assembler's operand count validation.
;   ADD requires two operands (ADD Rd, Rs), but only one is supplied.
;
; Expected Diagnostic:
;   Severity: Error
;   Message:  Expected ',' between operands in ADD
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    ADD R0
    HALT
