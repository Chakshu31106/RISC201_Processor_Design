; ld / st demo. Data memory is 64 KB, big-endian, word accesses must be
; 4-byte aligned. The stack lives at the top, so 0x2000 is ordinary data space.
; Expected at the end: r3 = 123, r5 = 456
mov r2, 0x2000
mov r1, 123
st r1, 8[r2]     ; mem[0x2008] = 123
st r1, -4[r2]    ; mem[0x1FFC] = 123
ld r3, 8[r2]     ; r3 = 123
mov r4, 0x2010
mov r1, 456
st r1, 0[r4]
ld r5, 0[r4]     ; r5 = 456
