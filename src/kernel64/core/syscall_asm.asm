[BITS 64]

SECTION .text

; import symbol
extern k_processSyscall

; export symbols
global k_syscallEntryPoint, k_syscallTestTask

; - param  : qword syscallNumber (RDI), const ParamTable* paramTable (RSI)
; - return : void
k_syscallEntryPoint:
	push rcx ; back up RIP (user) saved in RCX, related to IA32_LSTAR MSR.
	push r11 ; back up RFLAGS (user) saved in R11, related to IA32_FMASK MSR.

	; back up segment selectors except CS, SS.
	; can not push DS, ES directly. Thus, push them indirectly using CX.
	mov cx, ds
	push cx
	mov cx, es
	push cx
	push fs
	push gs

	; set GDT_OFFSET_KERNELDATASEGMENT (0x10) to segment selectors except CS, SS.
	; ([REF] CS, SS is set from IA32_STAR MSR.)
	mov cx, 0x10
	mov ds, cx
	mov es, cx
	mov fs, cx
	mov gs, cx

	call k_processSyscall

	; restore backup registers.
	pop gs
	pop fs
	pop cx
	mov es, cx
	pop cx
	mov ds, cx
	pop r11
	pop rcx

	; use 'o64' keyword in NASM complier in order to set W (bit 3) of REX prefix to 1 (IA-32e mode).
	o64 sysret

; - param  : void
; - return : void
k_syscallTestTask:
	mov rdi, 0xFFFFFFFFFFFFFFFF ; syscall number  : SYSCALL_TEST (0xFFFFFFFFFFFFFFFF)
	mov rsi, 0x00               ; parameter table : null (0x00)
	syscall

	mov rdi, 0xFFFFFFFFFFFFFFFF ; syscall number  : SYSCALL_TEST (0xFFFFFFFFFFFFFFFF)
	mov rsi, 0x00               ; parameter table : null (0x00)
	syscall

	mov rdi, 0xFFFFFFFFFFFFFFFF ; syscall number  : SYSCALL_TEST (0xFFFFFFFFFFFFFFFF)
	mov rsi, 0x00               ; parameter table : null (0x00)
	syscall
	
	mov rdi, 308  ; syscall number  : SYSCALL_EXIT (308)
	mov rsi, 0x00 ; parameter table : null (0x00)
	syscall
	jmp $
