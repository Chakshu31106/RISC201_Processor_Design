; Vector test: vadd, vdot (there is no vmul in the ISA), plus vsub and vcrs
; A vector operand is written as its first register: r20 = v1 (r20,r21,r22), r23 = v2 (r23,r24,r25), r26 = v3 (r26,r27,r28)
; Components are float32 bit patterns.  v1 = (1.0, 2.0, 3.0)   v2 = (4.0, 5.0, 6.0)
mov r20, #1.0
mov r21, #2.0
mov r22, #3.0
mov r23, #4.0
mov r24, #5.0
mov r25, #6.0


vcrs r26, r20, r23       ; v3 = v1 x v2 = (-3,6,-3) -> r26=0xC0400000 r27=0x40C00000 r28=0xC0400000
vang r1, r20, r23 
vdot r3, r20, r23
vmag r4, r20
vmag r5, r23
