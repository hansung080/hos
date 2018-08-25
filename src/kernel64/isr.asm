[BITS 64]

SECTION .text

; handlers (7)
extern k_commonExceptionHandler, k_deviceNotAvailableHandler, k_commonInterruptHandler, k_timerHandler, k_keyboardHandler
extern k_mouseHandler, k_hddHandler

; Exception Handling ISR (21)
global k_isrDivideError, k_isrDebug, k_isrNmi, k_isrBreakPoint, k_isrOverflow
global k_isrBoundRangeExceeded, k_isrInvalidOpcode, k_isrDeviceNotAvailable, k_isrDoubleFault, k_isrCoprocessorSegmentOverrun
global k_isrInvalidTss, k_isrSegmentNotPresent, k_isrStackSegmentFault, k_isrGeneralProtection, k_isrPageFault
global k_isr15, k_isrFpuError, k_isrAlignmentCheck, k_isrMachineCheck, k_isrSimdError
global k_isrEtcException

; Interrupt Handling ISR (17)
global k_isrTimer, k_isrKeyboard, k_isrSlavePic, k_isrSerialPort2, k_isrSerialPort1
global k_isrParallelPort2, k_isrFloppyDisk, k_isrParallelPort1, k_isrRtc, k_isrReserved
global k_isrNotUsed1, k_isrNotUsed2, k_isrMouse, k_isrCoprocessor, k_isrHdd1
global k_isrHdd2, k_isrEtcInterrupt

; Order to Save/Restore Conxtet in HansOS (use IST stack)
; 1. saved/restored by processor (6): SS, RSP, RFLAGS, CS, RIP, error code (optional)
; 2. saved/restored by ISR (19): RBP, RAX, RBX, RCX, RDX, RDI, RSI, R8, R9, R10, R11, R12, R13, R14, R15, DS, ES, FS, GS

; save context and switch segment selectors
%macro KSAVECONTEXT 0
	; save context (15 general regiters + 4 segment selectors = 19 registers)
	push rbp
	mov rbp, rsp
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
	
	mov ax, ds ; push DS, ES to stack using RAX, because can't do it directly.
	push rax
	mov ax, es
	push rax
	push fs
	push gs
	
	; switch segment selectors: save kernel data segment descriptor to DS, ES, FS, GS.
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
%endmacro

; restore context
%macro KLOADCONTEXT 0
	; restore context (15 general registers + 4 segment selectors = 19 registers)
	pop gs
	pop fs
	pop rax ; pop DS, ES from stack using RAX, because can't do it directly.
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

;====================================================================================================
; Exception Handling ISR (21): #0 ~ #19, #20 ~ #31
;====================================================================================================
; #0 : Divide Error ISR
k_isrDivideError:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 0                    ; set vector number to first parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #1 : Debug Exception ISR
k_isrDebug:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 1                    ; set vector number to first parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #2 : NMI Interrupt ISR
k_isrNmi:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 2                    ; set vector number to first parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #3 : Break Point ISR
k_isrBreakPoint:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 3                    ; set vector number to first parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #4 : Overflow ISR
k_isrOverflow:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 4                    ; set vector number to first parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #5 : BOUND Range Exceeded ISR
k_isrBoundRangeExceeded:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 5                    ; set vector number to first parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #6 : Invalid Opcode (Undefined Opcode) ISR
k_isrInvalidOpcode:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 6                    ; set vector number to first parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #7 : Device Not Available (No Math Coprocessor) ISR
k_isrDeviceNotAvailable:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 7                    ; set vector number to first parameter.
	call k_deviceNotAvailableHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #8 : Double Fault ISR
k_isrDoubleFault:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 8                    ; set vector number to first parameter.
	mov rsi, qword [rbp + 8]      ; set error code to second parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	add rsp, 8                    ; remove error code from stack.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #9 : Coprocessor Segment Overrun (Reserved) ISR
k_isrCoprocessorSegmentOverrun:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 9                    ; set vector number to first parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #10 : Invalid TSS ISR
k_isrInvalidTss:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 10                   ; set vector number to first parameter.
	mov rsi, qword [rbp + 8]      ; set error code to second parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	add rsp, 8                    ; remove error code from stack.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #11 : Segment Not Present ISR
k_isrSegmentNotPresent:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 11                   ; set vector number to first parameter.
	mov rsi, qword [rbp + 8]      ; set error code to second parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	add rsp, 8                    ; remove error code from stack.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #12 : Stack-Segment Fault ISR
k_isrStackSegmentFault:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 12                   ; set vector number to first parameter.
	mov rsi, qword [rbp + 8]      ; set error code to second parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	add rsp, 8                    ; remove error code from stack.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #13 : General Protection ISR
k_isrGeneralProtection:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 13                   ; set vector number to first parameter.
	mov rsi, qword [rbp + 8]      ; set error code to second parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	add rsp, 8                    ; remove error code from stack.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #14 : Page Fault ISR
k_isrPageFault:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 14                   ; set vector number to first parameter.
	mov rsi, qword [rbp + 8]      ; set error code to second parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	add rsp, 8                    ; remove error code from stack.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #15 : Intel Reserved ISR
k_isr15:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 15                   ; set vector number to first parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #16 : x87 FPU Floating-Point Error (Math Fault) ISR
k_isrFpuError:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 16                   ; set vector number to first parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #17 : Alignment Check ISR
k_isrAlignmentCheck:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 17                   ; set vector number to first parameter.
	mov rsi, qword [rbp + 8]      ; set error code to second parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	add rsp, 8                    ; remove error code from stack.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #18 : Machine Check ISR
k_isrMachineCheck:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 18                   ; set vector number to first parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #19 : SIMD Floating-Point Exception ISR
k_isrSimdError:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 19                   ; set vector number to first parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #20~#31 : Intel Reserved ISR
k_isrEtcException:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 20                   ; set vector number to first parameter.
	call k_commonExceptionHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

;====================================================================================================
; Interrupt Handling ISR (17): #32 ~ #47, #48 ~ #99
;====================================================================================================
; #32 : Timer ISR
k_isrTimer:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 32                   ; set vector number to first parameter.
	call k_timerHandler           ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #33 : PS/2 Keyboard ISR
k_isrKeyboard:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 33                   ; set vector number to first parameter.
	call k_keyboardHandler        ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #34 : Slave PIC Controller ISR
k_isrSlavePic:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 34                   ; set vector number to first parameter.
	call k_commonInterruptHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #35 : Serial Port 2 (COM Port 2) ISR
k_isrSerialPort2:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 35                   ; set vector number to first parameter.
	call k_commonInterruptHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #36 : Serial Port 1 (COM Port 1) ISR
k_isrSerialPort1:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 36                   ; set vector number to first parameter.
	call k_commonInterruptHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #37 : Parallel Port 2 (Print Port 2) ISR
k_isrParallelPort2:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 37                   ; set vector number to first parameter.
	call k_commonInterruptHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #38 : Floppy Disk Controller ISR
k_isrFloppyDisk:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 38                   ; set vector number to first parameter.
	call k_commonInterruptHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #39 : Parallel Port 1 (Print Port 1) ISR
k_isrParallelPort1:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 39                   ; set vector number to first parameter.
	call k_commonInterruptHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #40 : RTC ISR
k_isrRtc:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 40                   ; set vector number to first parameter.
	call k_commonInterruptHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #41 : Reserved ISR
k_isrReserved:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 41                   ; set vector number to first parameter.
	call k_commonInterruptHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #42 : Not Used 1 ISR
k_isrNotUsed1:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 42                   ; set vector number to first parameter.
	call k_commonInterruptHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #43 : Not Used 2 ISR
k_isrNotUsed2:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 43                   ; set vector number to first parameter.
	call k_commonInterruptHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #44 : PS/2 Mouse ISR
k_isrMouse:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 44                   ; set vector number to first parameter.
	call k_mouseHandler           ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #45 : Coprocessor ISR
k_isrCoprocessor:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 45                   ; set vector number to first parameter.
	call k_commonInterruptHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #46 : Hard Disk 1 (HDD1) ISR
k_isrHdd1:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 46                   ; set vector number to first parameter.
	call k_hddHandler             ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #47 : Hard Disk 2 (HDD2) ISR
k_isrHdd2:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 47                   ; set vector number to first parameter.
	call k_hddHandler             ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.

; #48~#99 : ETC Interrupt ISR
k_isrEtcInterrupt:
	KSAVECONTEXT                  ; save context and switch segment selectors.
	
	mov rdi, 48                   ; set vector number to first parameter.
	call k_commonInterruptHandler ; call C handler function.
	
	KLOADCONTEXT                  ; restore context.
	iretq                         ; restore context saved by processor, and return to the code where had be running.
