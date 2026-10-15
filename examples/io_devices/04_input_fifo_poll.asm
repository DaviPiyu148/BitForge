; ==============================================================================
; BitForge Example: Character Input FIFO (Port 0x00)
; Category: I/O Devices
; Description:
;   Demonstrates reading from Virtual Port 0x00 (Character Input FIFO) using IN.
;   Per the architecture specification:
;     - If the input FIFO contains characters, IN pops the front byte into Rd.
;     - If the input FIFO is empty, IN returns 0x00 and triggers an empty FIFO event.
;
; Expected Results:
;   State: Halted
;   R0:    0x00 (empty FIFO read returns zero)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    IN   R0, 0x00       ; Read input FIFO into R0 (returns 0x00 when empty)
    HALT
