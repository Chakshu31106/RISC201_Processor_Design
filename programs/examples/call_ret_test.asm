; call / ret with a nested call. call pushes the caller's ra and its own return
; address on the stack, and ret pops them, so f does not have to save ra itself.
; Expected at the end: r7 = 3, r8 = 2, r9 = 1, sp = 65536 (empty stack)

call f           ; push old ra, push return address, jump
mov r9, 1        ; runs after f returns
b done

f:
call g           ; nested call
mov r8, 2
ret

g:
mov r7, 3
ret

done:
nop
