; ==============================================================================
; BitForge Invalid Fixture: Unknown Mnemonic
; Category: Invalid Assembly (Intentionally fails to assemble)
; Description:
;   Exercises the assembler's unknown instruction diagnostic.
;
; Expected Diagnostic:
;   Severity: Error
;   Message:  Unknown instruction mnemonic 'FOOBAR'
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    FOOBAR R0
    HALT
