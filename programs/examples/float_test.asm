; Floating point: registers hold raw IEEE-754 single-precision bit patterns.
; Expected: r3 = 7.0, r4 = 2.0, r5 = 11.25, ST flag set (2.5 < 4.5), r8 = 3.0, r10 = 2.5
mov r1, 0x40200000   ; 2.5
mov r2, 0x40900000   ; 4.5
fadd r3, r1, r2
fsub r4, r2, r1
fmul r5, r1, r2
fdiv r6, r2, r1
fcmp r1, r2
mov r7, 0x41100000   ; 9.0
sqrt r8, r7
mov r9, 0xC0200000   ; -2.5
fabs r10, r9
