; ==============================================================================
; BitForge Example: Self-Loop and Budget Testing
; Category: Debugger
; Description:
;   An intentional infinite self-loop (JMP spin).
;   Used to verify:
;     1. Instruction budget exhaustion (stopping safely when max steps limit is reached).
;     2. GUI Pause functionality (clicking Pause halts execution cleanly without GUI freeze).
;     3. Responsive offscreen execution across 10,000-step scheduling slices.
;
; How to Debug:
;   1. In the GUI toolbar, set Max Steps to a small value (e.g. 50000).
;   2. Click Run (F5).
;   3. Observe the state badge update to "PAUSED" with status "Execution budget exhausted."
;   4. Or with a large budget, click Run and immediately click Pause to interrupt it.
;
; Expected Results:
;   Stop Reason: BudgetExhausted (or Paused if user clicked Pause)
;   PC:          0x0203 (spin label)
;   R0:          42
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 42          ; Marker value

spin:
    JMP spin            ; Infinite self-loop (address 0x0203)
