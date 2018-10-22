[BITS 64]

SECTION .text

; export symbol
global executeSyscall

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