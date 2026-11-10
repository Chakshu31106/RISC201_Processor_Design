; recursive factorial(5) with activation blocks, then iterative 5! pushed on stack
mov r0, 5
call .factorial
mov r6, r1       ; keep 5! in r6
mov r1, 1
mov r3, 5
loop:
mul r1, r1, r3
decr r3
cmp r3, 0
bgt loop
push r1
b .end
.factorial:
cmp r0, 1
beq .return
bgt .continue
b .return
.continue:
sub sp, sp, 8
st r0, 0[sp]
st ra, 4[sp]
sub r0, r0, 1
call .factorial
ld r0, 0[sp]
ld ra, 4[sp]
mul r1, r0, r1
add sp, sp, 8
ret
.return:
mov r1, 1
ret
.end:
nop
