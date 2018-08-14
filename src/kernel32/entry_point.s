[ORG 0x00]
[BITS 16]

SECTION .text

START:
	mov ax, 0x1000 ; entry point memory address of kernel32 (0x10000)
	mov ds, ax
	mov es, ax
	
	; compare BSP flag
	mov ax, 0x0000
	mov es, ax
	cmp byte [es:0x7C09], 0x00 ; If BSP flag == [0:AP], move to AP start point.
	je .AP_STARTPOINT
	
	; enable A20 gate
	mov ax, 0x2401        ; function number (0x2401: enable A20 gate)
	int 0x15              ; cause software interrupt (0x15: BIOS Service -> System Service)
	jc .A20_GATE_ERROR    ; error handling
	jmp .A20_GATE_SUCCESS ; success handling

.A20_GATE_ERROR:
	in al, 0x92  ; read 1 byte from system control port (0x92) and save it to AL.
	or al, 0x02  ; set A20 gate bit (bit 1) to 1.
	and al, 0xFE ; set system reset bit (bit 0) to 0.
	out 0x92, al ; save it to system control port (0x92).

.A20_GATE_SUCCESS:
.AP_STARTPOINT:
	; switch to kernel32
	cli                 ; close interrupt.
	lgdt [GDTR]         ; load GDT.
	mov eax, 0x4000003B ; set it to CR0 (PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1).
	mov cr0, eax        ; save it to CR0.
	jmp dword 0x18:(PROTECTED_MODE - $$ + 0x10000) ; set the kernel32 code segment descriptor to CS segment selector, move the address <PROTECTED_MODE>.

[BITS 32]
PROTECTED_MODE:
	mov ax, 0x20
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov esp, 0xFFFE
	mov ebp, 0xFFFE
	
	; compare BSP flag
	cmp byte [es:0x7C09], 0x00 ; If BSP flag == [0:AP], move to AP start point.
	je .AP_STARTPOINT
	
	push (SWITCH_SUCCESS_MESSAGE - $$ + 0x10000)
	push 4
	push 0
	call PRINT_MESSAGE
	add esp, 12

.AP_STARTPOINT:
	jmp dword 0x18:0x10200 ; set the kernel32 code segment descriptor to CS segment selector, move to the address of C entry point function (0x10200).

PRINT_MESSAGE:
	push ebp
	mov ebp, esp
	push esi
	push edi
	push eax
	push ecx
	push edx
	
	mov eax, dword [ebp + 12]
	mov esi, 160
	mul esi
	mov edi, eax
	
	mov eax, dword [ebp + 8]
	mov esi, 2
	mul esi
	add edi, eax
	
	mov esi, dword [ebp + 16]

.MESSAGE_LOOP:
	mov cl, byte [esi]
	cmp cl, 0
	je .MESSAGE_END
	mov byte [edi + 0xB8000], cl
	add esi, 1
	add edi, 2
	jmp .MESSAGE_LOOP

.MESSAGE_END:
	pop edx
	pop ecx
	pop eax
	pop edi
	pop esi
	pop ebp
	ret

align 8, db 0 ; align data below with 8 bytes.

dw 0x0000 ; needed to align GDTR with 8 bytes.

GDTR:
	dw GDT_END - GDT - 1    ; GDT Size (2 bytes)
	dd (GDT - $$ + 0x10000) ; GDT BaseAddress (4 bytes)

GDT:
	NULL_DESCRIPTOR: ; null segment descriptor (8 bytes)
		dw 0x0000
		dw 0x0000
		db 0x00
		db 0x00
		db 0x00
		db 0x00
	
	IA32E_CODE_DESCRIPTOR: ; kernel64 code segment descriptor (8 bytes)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x9A      ; P=1, DPL=00, S=1, Type=0xA:CodeSegment (Execute/Read)
		db 0xAF      ; G=1, D/B=0, L=1, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00
	
	IA32E_DATA_DESCRIPTOR: ; kernel64 data segment descriptor (8 bytes)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x92      ; P=1, DPL=00, S=1, Type=0x2:DataSegment (Read/Write)
		db 0xAF      ; G=1, D/B=0, L=1, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00
	
	PROTECT_CODE_DESCRIPTOR: ; kernel32 code segment descriptor (8 bytes)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x9A      ; P=1, DPL=00, S=1, Type=0xA:CodeSegment (Execute/Read)
		db 0xCF      ; G=1, D/B=1, L=0, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00
	
	PROTECT_DATA_DESCRIPTOR: ; kernel32 data segment descriptor (8 bytes)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x92      ; P=1, DPL=00, S=1, Type=0x2:DataSegment (Read/Write)
		db 0xCF      ; G=1, D/B=1, L=0, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00
	
GDT_END:

SWITCH_SUCCESS_MESSAGE: db '- switch to protected mode...................pass', 0

times 512 - ($ - $$) db 0x00
