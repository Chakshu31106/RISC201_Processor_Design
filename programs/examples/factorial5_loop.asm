; RISC201 program: compute factorial of 5 (5! = 120), real loop this time
; Branch offset is relative to the branch instruction's own address (confirmed).

mov r1, 1        ; result = 1
mov r2, 5        ; counter = 5

loop:
mul r1, r1, r2   ; result *= counter
decr r2          ; counter--
cmp r2, 0        ; compare counter to 0
bgt loop         ; if counter > 0, jump back to "loop"

push r1          ; push final result (120) onto the stack
nop
