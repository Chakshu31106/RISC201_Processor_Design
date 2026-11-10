; Bit-manipulation instruction examples for the custom ISA
; Bit indices are assumed to be in the range 0..31 for 32-bit registers.

        mov   r1, #10           ; binary ...00001010
        pcnt  r2, r1            ; r2 = number of set bits in r1

        tog   r3, r1, #1        ; r3 = r1 with bit 1 toggled
        sbit  r4, r1, #0        ; r4 = r1 with bit 0 set
        cbit  r5, r1, #3        ; r5 = r1 with bit 3 cleared

        nop
