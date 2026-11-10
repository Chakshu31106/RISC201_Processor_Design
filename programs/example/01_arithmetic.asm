; Arithmetic instruction examples for the custom ISA
; Registers are 32-bit. Immediate operands are written with #.
; Assumes the assembler accepts labels/comments and the ISA's listed syntax.

        mov   r1, 20
        mov   r2, 6
        mov   r9, -3

        add   r3, r1, r2       ; r3 = 20 + 6 = 26
        sub   r4, r1, r2       ; r4 = 20 - 6 = 14
        mul   r5, r1, r2       ; r5 = 20 * 6 = 120
        div   r6, r1, r2       ; r6 = 20 / 6
        mod   r7, r1, r2       ; r7 = 20 % 6
        pow   r8, r2, #3       ; r8 = 6^3

        cmp   r1, r2           ; updates comparison flags
        incr  r1               ; r1 = r1 + 1
        decr  r2               ; r2 = r2 - 1
        abs   r9               ; absolute value of r9 (input assumed preloaded)

        nop
