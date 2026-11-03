; =====================================
; RISC201 VECTOR INSTRUCTION TESTS (syntax examples from the team's all_tests.asm)
; A vector operand is r20 (v1), r23 (v2) or r26 (v3). To run vectors, see vector_ops_test.asm
; =====================================
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
vld r20, [r2 + #8]
vst r23, [r4 + #16]
