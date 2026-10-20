; BitForge Example 01: Arithmetic Baseline
; Computes 10 + 20 and stores the result (0x1E / 30) at address 0x8000

.ORG 0x0200
.ENTRY start

start:
    LDI R0, 10         ; R0 = 10 (0x0A)
    LDI R1, 20         ; R1 = 20 (0x14)
    ADD R0, R1         ; R0 = R0 + R1 = 30 (0x1E)
    ST [0x8000], R0    ; memory[0x8000] = 30
    HALT               ; halt execution
