; ==============================================================================
; BitForge Invalid Fixture: Malformed / Unterminated String
; Category: Invalid Assembly (Intentionally fails to assemble)
; Description:
;   Exercises the lexer's unterminated string literal diagnostic.
;   A string literal opened with a double quote without a closing quote
;   before newline produces a lexer error.
;
; Expected Diagnostic:
;   Severity: Error
;   Message:  Unterminated string literal (newline before closing '"')
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    .ASCII "This string has no closing quote
    HALT
