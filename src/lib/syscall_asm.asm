[BITS 64]

SECTION .text

; import symbols
extern main, exit

; export symbols
global _start, executeSyscall

; - desc : entry point of applications
_start:
	call main

	mov rdi, rax ; set RAX (return value of main) to RDI (first parameter of exit).
	call exit

	; This infinite loop below will be not executed, because task exits above.
	jmp $

	ret

; - param  : qword syscallNumber (RDI), const ParamTable* paramTable (RSI)
; - return : qword result (RAX)
executeSyscall:
	push rcx ; back up RCX (user) to save RIP (user), related to IA32_LSTAR MSR.
	push r11 ; back up R11 (user) to save RFLAGS (user), related to IA32_FMASK MSR.

	syscall

	; restore backup registers.
	pop r11
	pop rcx
	ret