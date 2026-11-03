; Stack demo: the stack starts at the top of data memory (sp = 65536) and grows down, 4 bytes per push.
; Expected at the end: r6 = 30, r7 = 20, sp = 65536
mov r5, 10
push r5          ; stack: 10
mov r5, 20
push r5          ; stack: 10 20
mov r5, 30
push r5          ; stack: 10 20 30
pop r6           ; r6 = 30
peek r7          ; r7 = 20 (stays on the stack)
mpop #2          ; drop two words -> empty
push r5
clr              ; sp = 0
