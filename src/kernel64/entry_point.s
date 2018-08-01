[BITS 64]

SECTION .text

extern k_main
extern g_apicIdAddr, g_awakeApCount

START:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	
	; If it's BSP, use whole 6MB ~ 7MB area as kernel64 stack.
	mov rsp, 0x6FFFF8
	mov rbp, 0x6FFFF8
	
	; compare BSP flag
	cmp byte [0x7C09], 0x01 ; IF BSP flag == [1:BSP] , move to BSP start point.
	je .BSP_STARTPOINT
	
	; IF it's AP, use 6MB ~ 7MB area divided by max processor count (16) as kernel64 stack. (use the area from the end.)
	mov rax, 0
	mov rbx, qword [g_apicIdAddr]
	mov eax, dword [rbx]
	shr rax, 24      ; save Local APIC ID Field (bit 24~31) of Local APIC ID Register to RAX.
	
	mov rbx, 0x10000 ; 0x10000(64KB, stack size) = 0x100000(1MB, total stack size) / 0x10(16, max processor count)
	mul rbx          ; RAX(stack offset) = RAX(APIC ID) * RBX(stack size)
	
	sub rsp, rax
	sub rbp, rax
	
	; increase awoken AP count (use lock to synchronize memory from processors.)
	lock inc dword [g_awakeApCount]

.BSP_STARTPOINT:
	call k_main
	
	jmp $
