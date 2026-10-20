; ==============================================================================
; BitForge Invalid Fixture: Emission in Reserved Stack Region
; Category: Invalid Assembly (Intentionally fails to assemble)
; Description:
;   Exercises the assembler's reserved stack boundary enforcement.
;   Per ADR-001 and the architecture specification, memory 0xF000..0xFFFF is
;   strictly reserved for the hardware stack and cannot be targeted by .ORG or code.
;
; Expected Diagnostic:
;   Severity: Error
;   Message:  Cannot set .ORG into reserved stack region (address 0xF000 >= 0xF000)
; ==============================================================================

.ORG 0xF000             ; Prohibited: inside reserved stack range 0xF000..0xFFFF
.ENTRY main

main:
    NOP
    HALT
