; Load/store instruction examples for the custom ISA
; Syntax follows the ISA opcode table:
;   ld rd, [rs1 + #offset]
;   st rs1, [rd + #offset]
;   mov rd, #immediate
; Memory addresses and data layout are illustrative.

        mov   r1,#1024         ; base address of a memory region
        mov   r2,#1234

        st    r2, 0[r1]     ; memory[r1 + 0] = r2
        ld    r3, 0[r1]     ; r3 = memory[r1 + 0]

        mov   r4, #8
        st    r4, 4[r1]     ; memory[r1 + 4] = r4
        ld    r5, 4[r1]     ; r5 = memory[r1 + 4]

        nop
