; ==============================================================================
; BitForge Example: Live Memory Viewer Inspection
; Category: Debugger
; Description:
;   Designed to observe real-time memory mutations in the GUI Memory Viewer.
;   Sequentially writes bytes 0xAA, 0xBB, 0xCC, 0xDD to addresses 0x8000..0x8003.
;
; How to Debug:
;   1. Assemble with F7.
;   2. Select the "Memory (64 KiB)" tab in the bottom dock.
;   3. Type "0x8000" in the address jump box or scroll to row 0x8000.
;   4. Press F10 to step instruction by instruction.
;   5. Watch each memory byte illuminate and update from 00 to AA, BB, CC, DD.
;
; Expected Results:
;   State:  Halted
;   Memory: 0x8000=0xAA, 0x8001=0xBB, 0x8002=0xCC, 0x8003=0xDD
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI R0, 0xAA
    ST  [0x8000], R0    ; Step 1: memory[0x8000] becomes 0xAA

    LDI R0, 0xBB
    ST  [0x8001], R0    ; Step 2: memory[0x8001] becomes 0xBB

    LDI R0, 0xCC
    ST  [0x8002], R0    ; Step 3: memory[0x8002] becomes 0xCC

    LDI R0, 0xDD
    ST  [0x8003], R0    ; Step 4: memory[0x8003] becomes 0xDD

    HALT
