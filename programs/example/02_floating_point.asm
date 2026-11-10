; Floating-point instruction examples for the custom ISA
; Floating-point values are 32-bit IEEE-754-like values held in registers.
; This ISA document does not specify a float-immediate/load syntax, so r1-r3
; are assumed to contain valid floating-point values before execution.
        mov   r1, #20.7
        mov   r2, #6.8
        fadd  r4, r1, r2       ; r4 = r1 + r2
        fsub  r5, r1, r2       ; r5 = r1 - r2
        fmul  r6, r1, r2       ; r6 = r1 * r2
        fdiv  r7, r1, r2       ; r7 = r1 / r2

        fcmp  r1, r2           ; updates floating-point comparison flags
        sqrt  r8, r2             ; square root of r8, per single-register form
        fabs  r9, r2          ; absolute value of r9
        sin   r10,r2          ; sine of r10 (angle units not specified)
        cos   r11,r2              ; cosine of r11 (angle units not specified)

        nop
