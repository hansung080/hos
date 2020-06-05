[BITS 64]

SECTION .text

global k_inPortByte, k_outPortByte, k_inPortWord, k_outPortWord
global k_loadGdt, k_loadTss, k_loadIdt
global k_enableInterrupt, k_disableInterrupt, k_readRflags
global k_readTsc
global k_switchContext
global k_halt, k_pause
global k_testAndSet
global k_initFpu, k_saveFpuContext, k_loadFpuContext, k_setTs, k_clearTs
global k_enableGlobalLocalApic
global k_readMsr, k_writeMsr

; ====================================================================================================
; < Calling Convention - from C to assembly, IA-32e mode >
; -------------------------------------------
; Parameter  Integer    Float
; -------------------------------------------
;    1       RDI(1)     XMM0(11.1)
;    2       RSI(2)     XMM1(12.2)
;    3       RDX(3)     XMM2(13.3)
;    4       RCX(4)     XMM3(14.4)
;    5       R8(5)      XMM4(15.5)
;    6       R9(6)      XMM5(16.6)
;    7       stack(7)   XMM6(17.7)
;    8       stack(8)   XMM7(18.8)
;    9       stack(9)   stack(19.9)
;    10      stack(10)  stack(20.0)
;    ...     ...        ...
; -------------------------------------------
;
; -------------------------------------------
; Return     Integer    Float
; -------------------------------------------
;    -       RAX(1)     XMM0(1.1)
;    -       RDX        XMM1
; -------------------------------------------
; ====================================================================================================

; - param  : word port (RDI)
; - return : byte data (RAX)
k_inPortByte:
	push rdx
	
	mov rdx, rdi ; port
	mov rax, 0   ; data (initialization)
	; read data (1 byte) from a port number saved in DX, save it to AL (AL will be used as a return value.)
	in al, dx
	
	pop rdx
	ret

; - param  : word port (RDI), byte data (RSI)
; - return : void
k_outPortByte:
	push rdx
	push rax
	
	mov rdx, rdi ; port
	mov rax, rsi ; data
    ; write data (1 byte) saved in AL to a port number saved in DX.
	out dx, al
	
	pop rax
	pop rdx
	ret

; - param  : word port (RDI)
; - return : word data (RAX)
k_inPortWord:
	push rdx
	
	mov rdx, rdi ; port
	mov rax, 0   ; data (initialization)
	; read data (2 bytes) from a port number saved in DX, save it to AX (AX will be used as a return value.)
	in ax, dx
	
	pop rdx
	ret

; - param  : word port (RDI), word data (RSI)
; - return : void
k_outPortWord:
	push rdx
	push rax
	
	mov rdx, rdi ; port
	mov rax, rsi ; data
    ; write data (2 bytes) saved in AX to a port number saved in DX.
	out dx, ax
	
	pop rax
	pop rdx
	ret

; - param  : qword gdtrAddr (RDI)
; - return : void
k_loadGdt:
	; set the address of GDTR structure to GDTR register, and load GDT table on processor.
	lgdt [rdi]
	ret

; - param  : word tssOffset (DI)
; - return : void
k_loadTss:
	; set the offset of TSS segment descriptor to TR register, and load TSS segment on processor.
	ltr di
	ret

; - param  : qword idtrAddr (RDI)
; - return : void
k_loadIdt:
	; set the address of IDTR structure to IDTR register, and load IDT table on processor.
	lidt [rdi]
	ret

; - param  : void
; - return : void
k_enableInterrupt:
	sti ; start interrupt only in the current core.
	ret

; - param  : void
; - return : void
k_disableInterrupt:
	cli ; close interrupt only in the current core.
	ret

; - param  : void
; - return : qword data (RAX)
k_readRflags:
	pushfq  ; push RFLAGS register to stack.
	pop rax ; pop RFLAGS register from stack, save it to RAX (RAX will be used as a return value.)
	ret

; - param  : void
; - return : qword data (RAX)
k_readTsc:
	push rdx
	
	; read time stamp counter register (64 bits), save high 32 bits to RDX, low 32 bits to RAX.
	rdtsc
	
	; RAX = RAX | (RDX << 32) : RAX will be used as a return value.
	shl rdx, 32
	or rax, rdx
	
	pop rdx
	ret

; save context (15 general registers + 4 segment selectors = 19 registers)
%macro KSAVECONTEXT 0
	push rbp
	push rax
	push rbx
	push rcx
	push rdx
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	
	mov ax, ds ; use RAX to push DS, ES to stack, becase it's not allowed to push DS, ES to stack directly.
	push rax
	mov ax, es
	push rax
	push fs
	push gs
%endmacro

; restore context (15 general registers + 4 segment selectors = 19 registers)
%macro KLOADCONTEXT 0
	pop gs
	pop fs
	pop rax ; use RAX to pop DS, ES from stack, becase it's not allowed to pop DS, ES from stack directly.
	mov es, ax
	pop rax
	mov ds, ax
	
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	pop rbp
%endmacro

; - param  : Context* currentContext (RDI), Context* nextContext (RSI)
; - return : void
k_switchContext:
	push rbp
	mov rbp, rsp
	
	; If currentContext == null, it doesn't have to save context.
	pushfq ; push RFLAGS to stack to keep RFLAGS not-changed after cmp command below.
	cmp rdi, 0
	je .loadContext
	popfq
	
	; ***** save context of a current task *****
	push rax ; back up RAX to use as register offset.
	
	; save 5 registers (SS, RSP, RFLAGS, CS, RIP) to Context structure (currentContext).
	mov ax, ss ; save SS
	mov qword [rdi + (23 * 8)], rax
	
	mov rax, rbp ; save RSP saved in RBP.
	add rax, 16  ; when saving RSP, be except RBP(push rbp) and return address.
	mov qword [rdi + (22 * 8)], rax
	
	pushfq ; push RFLAGS to stack
	pop rax
	mov qword [rdi + (21 * 8)], rax
	
	mov ax, cs ; save CS
	mov qword [rdi + (20 * 8)], rax
	
	mov rax, qword [rbp + 8] ; set RIP as return address in order to move the next line of k_switchContext function when restoring a next context.
	mov qword [rdi + (19 * 8)], rax
	
	pop rax
	pop rbp
	
	; move RSP to No.19 (RIP) offset in order to save left 19 registers to No.18 (RBP) ~ No.0 (GS) offset of Context structure.
	add rdi, (19 * 8)
	mov rsp, rdi
	sub rdi, (19 * 8)
	
	; save left 19 registers to Context structure (currentContext).
	KSAVECONTEXT

; ***** restore context of a next task *****
.loadContext:
	mov rsp, rsi
	
	; restore 19 registers from Context structure (nextContext).
	KLOADCONTEXT
	
	; restore left 5 registers from Context structure (nextContext), return to the address of RIP.
	iretq

; - param  : void
; - return : void
k_halt:
	; halt processor.
	hlt
	hlt
	ret

; - param  : void
; - return : void
k_pause:
	; pause processor.
	pause
	ret

; - param  : volatile byte* dest (RDI), byte cmp (RSI), byte src (RDX)
; - return : bool ret (RAX)
; - desc   : atomic operation for compare and set, same as <AX==cmp, A==*dest, B==src>
;            -> If cmp == *dest, set src to *dest, return true(1).
;            -> If cmp != *dest, return false(0)
k_testAndSet:
	; 1. lock
	;    -> This command is used as a preposition in assembly code,
	;       and lock system bus to prevent other processors or cores from accessing memory while executing of a following command.
	; 2. cmpxchg A, B
	;    -> If AX == A, mov RFLAGS.ZF, 1 and mov A, B
	;    -> If AX != A, mov RFLAGS.ZF, 0 and mov AX, A
	mov rax, rsi
	lock cmpxchg byte [rdi], dl
	je .SUCCESS ; If RFLAGS.ZF == 1, move to .SUCCESS
	
.NOTSAME:
	mov rax, 0x00 ; return false(0)
	ret
	
.SUCCESS:
	mov rax, 0x01 ; return true(1)
	ret

; - param  : void
; - return : void
k_initFpu:
	finit ; initialize FPU
	ret

; - param  : void* fpuContext (RDI)
; - return : void
k_saveFpuContext:
	fxsave [rdi] ; save FPU register (512 bytes) to fpuContext.
	ret

; - param  : void* fpuContext (RDI)
; - return : void
k_loadFpuContext:
	fxrstor [rdi] ; restore FPU register (512 bytes) from fpuContext.
	ret

; - param  : void
; - return : void
k_setTs:
	push rax
	
	; set CR0.TS (bit 3) to 1 in order to cause No.7 exception (#NM, Device Not Available) when task switching.
	mov rax, cr0
	or rax, 0x08
	mov cr0, rax
	
	pop rax
	ret

; - param  : void
; - return : void
k_clearTs:
	; set CR0.TS (bit 3) to 0
	clts
	ret

; - param  : void
; - return : void
k_enableGlobalLocalApic:
	push rcx
	push rdx
	push rax
	
	; IA32_APIC_BASE MSR (address 27, size 64 bits)
	; - local APIC global enable/disable field (bit 11) = [1: all local APICs enable]
	mov rcx, 27
	rdmsr
	or eax, 0x0800
	wrmsr

	pop rax
	pop rdx
	pop rcx
	ret

; ====================================================================================================
; < Parameters of MSR Commands (rdmsr, wrmsr) >
;   - ECX: MSR address
;   - EDX: high 32 bits of MSR (64 bits)
;   - EAX: low 32 bits of MSR (64 bits)
; ====================================================================================================

; - param  : qword addr (RDI), qword* high32bits (RSI), qword* low32bits (RDX)
; - return : void
k_readMsr:
	push rcx
	push rdx
	push rax
	push rbx

	mov rbx, rdx ; back up low32bits (RDX) to RBX.

	mov rcx, rdi
	rdmsr
	mov qword [rsi], rdx
	mov qword [rbx], rax

	pop rbx
	pop rax
	pop rdx
	pop rcx
	ret

; - param  : qword addr (RDI), qword high32bits (RSI), qword low32bits (RDX)
; - return : void
k_writeMsr:
	push rcx
	push rdx
	push rax
	
	mov rcx, rdi
	mov rax, rdx
	mov rdx, rsi
	wrmsr

	pop rax
	pop rdx
	pop rcx
	ret
