; ==============================================================================
; BitForge Invalid Fixture: Unresolved Label
; Category: Invalid Assembly (Intentionally fails to assemble)
; Description:
;   Exercises the assembler's Pass 2 unresolved symbol diagnostic.
;   Branching to a symbol that is never declared produces an error.
;
; Expected Diagnostic:
;   Severity: Error
;   Message:  Unresolved label: 'nonexistent_target'
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    JMP nonexistent_target
    HALT
