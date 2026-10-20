; ==============================================================================
; BitForge Example: Array Traversal
; Category: Memory
; Description:
;   Demonstrates defining a contiguous byte array using the .BYTE directive
;   and traversing it sequentially using the IX register and INX.
;   Reads elements 10, 20, 30, 40, 50.
;
; Expected Results:
;   State:  Halted
;   R0:     50 (last element read from array)
;   R1:     0 (loop counter decremented to zero)
;   IX:     0x0305 (points past the last element of the array)
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI16 IX, data_array ; IX points to start of array (0x0300)
    LDI   R1, 5          ; Array length = 5

loop:
    LDX   R0, [IX]       ; Read current array element into R0
    INX                  ; Advance IX to next array element
    DEC   R1             ; Decrement counter
    JNZ   loop           ; Repeat until all 5 elements are processed

    HALT

.ORG 0x0300
data_array:
    .BYTE 10, 20, 30, 40, 50
