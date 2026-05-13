.arch armv8-a
.text
.align 2
.global abs_sum
.type abs_sum, %function

// Compute the sum of the absolute value of values in the array.
// x0: address of long array[32]
// x1: size of array (32)
abs_sum:
    // Modify below here
    movz    x2, #0              // Running sum.

.loop:
    ldur x3, [x0, #0]           // Load element 1 from memory.
    ldur x4, [x0, #8]           // Load element 2 from memory.
    ldur x5, [x0, #16]          // Load element 3 from memory.
    ldur x6, [x0, #24]          // Load element 4 from memory.
    ldur x11, [x0, #32]          // Load element 5 from memory.

    add x0, x0, #40             // Increment pointer.
    sub x1, x1, #5              // Decrement remaining size.
    
    asr x7, x3, #63             // extract sign so now it's 0 or -1
    asr x8, x4, #63             // repeat for elements 2-5
    asr x9, x5, #63
    asr x10, x6, #63
    asr x12, x11, #63

    eor x3, x3, x7              // if negative, take inverse, else nothing
    eor x4, x4, x8              // repeat for elements 2-5
    eor x5, x5, x9
    eor x6, x6, x10
    eor x11, x11, x12

    subs x3, x3, x7             // if negative, add 1, else nothing
    subs x4, x4, x8             // repeat for elements 2-5
    subs x5, x5, x9 
    subs x6, x6, x10
    subs x11, x11, x12 

    adds x2, x2, x3             // add all to sum
    adds x2, x2, x4
    adds x2, x2, x5
    adds x2, x2, x6
    adds x2, x2, x11

    movz x13, #4
    cmp x1, xzr                 // Set flags for loop branch.
    b.eq .done                  // Continue loop.
    cmp x1, x13
    b.gt .loop                  

.loop2:                          // in case array size isn't divisible by 4
    ldur x3, [x0, #0]           // Load element 1 from memory
    add x0, x0, #8              // Increment pointer.
    sub x1, x1, #1              // Decrement remaining size.
    asr x4, x3, #63
    eor x3, x3, x4              // if negative, take inverse, else nothing
    subs x3, x3, x4             // if negative, add 1, else nothing
    adds x2, x2, x3
    cmp x1, xzr
    b.ne .loop2
.done:
    adds    x0, xzr, x2         // Move sum into x0.
    ret
.size   abs_sum, .-abs_sum
