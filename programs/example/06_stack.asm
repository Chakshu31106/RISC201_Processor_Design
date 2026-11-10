; Stack instruction examples for the custom ISA
; r30 is the stack pointer and r31 is the return-address register.
; Exact stack growth direction and mpop/peek/clr side effects are not
; specified in the ISA document; this example follows the mnemonic intent.

        mov   r1, #11
        mov   r2, #22
        mov   r3, #33

        push  r1               ; push r1
        push  r2               ; push r2
        push  r3               ; push r3

        peek  r4               ; copy/read top stack item into r4
        pop   r5               ; pop top item into r5
        mpop  #2               ; discard/pop two stack entries

        push  r1
        push  r2
                             ; clear stack (as defined by implementation)

        nop
