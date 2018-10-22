[BITS 32]

global k_readCpuid, k_switchToKernel64

SECTION .text

; - param  : dword eax_, dword* eax, dword* ebx, dword* ecx, dword* edx
; - return : void
k_readCpuid:
	push ebp
	mov ebp, esp
	push eax
	push ebx
	push ecx
	push edx
	push esi
	
	; EAX=0x00000000: basic Cpuid info search (after cpuid command, save processor vendor name (12byte) to EBX-EDX-ECX.)
	; EAX=0x80000001: extended Cpuid info search (after cpuid command, save supportability of 64-bit mode to bit 29 of EDX.)
	mov eax, dword [ebp + 8] ; eax_
	cpuid
	
	mov esi, dword [ebp + 12] ; eax
	mov dword [esi], eax;
	
	mov esi, dword [ebp + 16] ; ebx
	mov dword [esi], ebx;
	
	mov esi, dword [ebp + 20] ; ecx
	mov dword [esi], ecx;
	
	mov esi, dword [ebp + 24] ; edx
	mov dword [esi], edx;
	
	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
	pop ebp
	ret

; - param  : void
; - return : void
k_switchToKernel64:
	
	; CR4 control register: OSXMMEXCPT(bit 10)=1, OSFXSR(bit 9)=1, PAE(bit 5)=1
	mov eax, cr4
	or eax, 0x620
	mov cr4, eax
	
	; set the base address of PML4 table (0x100000, 1MB) to CR3 control register.
	mov eax, 0x100000
	mov cr3, eax
	
	; ==================================================
	; < Parameters of MSR Commands (rdmsr, wrmsr) >
	;   - ECX: MSR address
	;   - EDX: high 32 bits of MSR (64 bits)
	;   - EAX: low 32 bits of MSR (64 bits)
	; ==================================================
	; IA32_EFER MSR (address 0xC0000080, size 64 bits): LME(bit 8)=1, SCE(bit 0)=1
	mov ecx, 0xC0000080
	rdmsr
	or eax, 0x0101
	wrmsr
	
	; CR0 control register: PG(bit 31)=1, CD(bit 30)=0, NW(bit 29)=0, TS(bit 3)=1, EM(bit 2)=0, MP(bit 1)=1
	; - enable paging, cache, FPU
	; - NW(bit 29)=0: [NOTE] NW must be set to 1 to use the write back policy.
	mov eax, cr0
	or eax, 0xE000000E
	xor eax, 0x60000004
	mov cr0, eax
	
	jmp 0x08:0x200000 ; set kernel64 code segment descriptor to CS segment selector, move to the address of kernel64 (0x200000, 2MB).
	
	jmp $ ; not executed.
