; Vector test: vadd, vdot (there is no vmul in the ISA), plus vsub and vcrs
; A vector operand is written as its first register: r20 = v1 (r20,r21,r22), r23 = v2 (r23,r24,r25), r26 = v3 (r26,r27,r28)
; Components are float32 bit patterns.  v1 = (1.0, 2.0, 3.0)   v2 = (4.0, 5.0, 6.0)
mov r20, 0x3F800000    ; 1.0
mov r21, 0x40000000    ; 2.0
mov r22, 0x40400000    ; 3.0
mov r23, 0x40800000    ; 4.0
mov r24, 0x40A00000    ; 5.0
mov r25, 0x40C00000    ; 6.0

vadd r26, r20, r23       ; v3 = v1 + v2 = (5,7,9)   -> r26=0x40A00000 r27=0x40E00000 r28=0x41100000
vdot r5, r20, r23       ; r5 = v1 . v2 = 32.0      -> r5=0x42000000
vsub r26, r23, r20       ; v3 = v2 - v1 = (3,3,3)   -> r26=r27=r28=0x40400000
vcrs r26, r20, r23       ; v3 = v1 x v2 = (-3,6,-3) -> r26=0xC0400000 r27=0x40C00000 r28=0xC0400000
vdot r6, r20, r20       ; r6 = v1 . v1 = 14.0      -> r6=0x41600000
