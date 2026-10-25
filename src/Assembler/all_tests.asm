; RISC201 combined test file
; Part 1: programs that must assemble. Part 2 (bottom): disabled cases that must be rejected.

; ---- test ----
mov r1, 5
mov r2, 10
add r3, r1, r2
nop

; ---- arithmetic_test ----
add  r3, r1, r2
add r3 , r2, #-4
sub  r4, r1, #10
mul  r5, r3, r4
div  r6, r5, r2
mod  r7, r5, #3
cmp  r7, #0
incr r1
decr r2
pow  r8, r2, #3
abs  r9

; ---- logical_test ----
and  r3, r1, r2
or   r4, r1, r2
xor  r5, r3, r4
not  r6,
nand r7, r1, r2
nor  r8, r1, r2

; ---- not_operand_test ----
not r1,

; ---- shift_test ----
lsl  r3, r1, r2
lsr  r4, r1, #2
asr  r5, r3, #1
rotl r6, r4, r2
rotr r7, r5, #3

; ---- bit_test ----
pcnt r3, r1
tog  r4, r1, #5
sbit r5, r2, #7
cbit r6, r3, #2

; ---- branch_test ----
b #16
beq #-8
bgt #24
bst #-16
call #32
ret

; ---- branch_label_test ----
b forward_target
nop
forward_target:
nop

; ---- backward_branch_test ----
nop
nop
backward_target:
nop
b backward_target

; ---- memory_test ----
ld r1, 4[r2]
st r3, 8[r4]
ld r5, -4[r6]
st r7, -8[r8]
mov r5, r6
mov r7, #123

; ---- offset_test ----
ld r1, 2199023255551[r2]
ld r1, -2199023255552[r2]

; ---- stack_test ----
push r5
pop r6
peek r7
mpop #3
clr

; ---- float_test ----
fadd r1, r2, r3
fsub r4, r5, r6
fmul r7, r8, r9
fdiv r10, r11, r12
fcmp r13, r14
sqrt r15, r16
fabs r17, r18
sin r19, r20
cos r21, r22

; ---- vector_test ----
vadd r20, r23, r26
vsub r20, r23, r26
vdot r1, r20, r23
vcrs r20, r23, r26
vmag r1, r20
vang r1, r20, r23
vld r20, r2, 4
vld r23, r5, -4
vld r26, r0, 100
vst r20, r4, 8
vst r23, r6, -8
vst r26, r5, 100

; ---- vector_memory_test ----
vld r20, [r2 + #8]
vst r23, [r4 + #16]
vld r27, 12[r5]
vld r26, r7

; ---- factorial ----
mov r1, 1
mov r2, 5
loop:
mul r1, r1, r2
decr r2
cmp r2, 0
bgt loop
push r1
nop

; ---- nop_test ----
nop

; ======== EXPECTED FAILURES (disabled) ========
; The assembler writes no output if any line has an error, so these stay commented out.
; To check one: remove the leading ';' from its instruction line(s) and assemble; expect the error shown.

; ---- bit_negative_test ---- expect: tog: bit index must be between 0 and 31
; tog r1, r2, #-1

; ---- bit_range_test ---- expect: tog: bit index must be between 0 and 31
; tog r1, r2, #32

; ---- fcmp_operand_test ---- expect: fcmp: expected rs1, rs2
; fcmp r1

; ---- invalid_test ---- expect: Unknown mnemonic: addd
; addd r1, r2, r3

; ---- nop_operand_test ---- expect: nop: expected no operands  (and: clr: expected no operands)
; nop r1
; clr r1

; ---- operand_count_test ---- expect: pcnt: expected rd, rs1
; pcnt r1
