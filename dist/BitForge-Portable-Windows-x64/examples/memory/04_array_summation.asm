; ==============================================================================
; BitForge Example: Array Summation
; Category: Memory
; Description:
;   Traverses an array of five positive bytes, sums their values into R0,
;   and writes the accumulated sum to memory at address 0x8100.
;   Elements: 5, 10, 15, 20, 25.
;   Sum = 5 + 10 + 15 + 20 + 25 = 75 (0x4B).
;
; Expected Results:
;   State:  Halted
;   R0:     75 (0x4B, total accumulated sum)
;   R1:     0 (loop counter exhausted)
;   Memory: byte at 0x8100 == 0x4B
; ==============================================================================

.ORG 0x0200
.ENTRY main

main:
    LDI16 IX, numbers   ; Pointer to numbers array
    LDI   R1, 5         ; Number of elements
    LDI   R0, 0         ; Sum accumulator = 0

loop:
    LDX   R2, [IX]      ; Load current array element into R2
    ADD   R0, R2        ; Accumulate sum: R0 += R2
    INX                 ; Move to next element
    DEC   R1            ; Decrement counter
    JNZ   loop          ; Repeat until count is 0

    ST    [0x8100], R0  ; Store total sum to memory[0x8100]
    HALT

.ORG 0x0300
numbers:
    .BYTE 5, 10, 15, 20, 25
