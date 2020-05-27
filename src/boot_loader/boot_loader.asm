[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x07C0:START ; set <0x07C0> to CS segment register, and move to the address <0x7C00 + START>.

TOTAL_SECTOR_COUNT: dw 0x02    ; physical address 0x7C05: the total sector count of hOS image except boot-loader (max 1152 sectors, max 0x90000 bytes)
KERNEL32_SECTOR_COUNT: dw 0x02 ; physical address 0x7C07: the sector count of kernel32
BSP_FLAG: db 0x01              ; physical address 0x7C09: BSP flag (0: AP, 1: BSP)
GRAPHIC_MODE_FLAG: db 0x01     ; physical address 0x7C0A: graphic mode flag (0: text mode, 1: graphic mode)

START:
	mov ax, 0x07C0 ; boot-loader memory address (0x7C00)
	mov ds, ax
	mov ax, 0xB800 ; video memory address (0xB8000)
	mov es, ax
	
	mov ax, 0x0000
	mov ss, ax
	mov sp, 0xFFFE
	mov bp, 0xFFFE
	
	mov byte [BOOT_DRIVE], dl
	
	mov si, 0

.SCREEN_CLEAR_LOOP:
	mov byte [es:si], 0
	mov byte [es:si+1], 0x0A
	add si, 2
	cmp si, 80*25*2
	jl .SCREEN_CLEAR_LOOP
	
	push MESSAGE1
	push 0
	push 0
	call PRINT_MESSAGE
	add sp, 6
	
	push IMAGE_LOADING_MESSAGE
	push 1
	push 0
	call PRINT_MESSAGE
	add sp, 6

RESET_DISK:
	mov ax, 0x00              ; function number (0x00: reset)
	mov dl, byte [BOOT_DRIVE] ; drive number
	int 0x13                  ; cause software interrupt (0x13: BIOS Service -> Disk I/O Service)
	jc HANDLE_DISK_ERROR      ; exception handling
	
	mov ah, 0x08               ; function number (0x08: read disk parameters)
	mov dl, byte [BOOT_DRIVE]  ; drive number
	int 0x13                   ; cause software interrupt (0x13: BIOS Service -> Disk I/O Service)
	jc HANDLE_DISK_ERROR       ; exception handling
	mov byte [LAST_HEAD], dh   ; last head number (DH 8 bits)
	mov al, cl                 ; -
	and al, 0x3f               ; -
	mov byte [LAST_SECTOR], al ; last sector number (CL low 6 bits)
	mov byte [LAST_TRACK], ch  ; last track number (CH 8 bits + CL high 2 bits)
	
	mov si, 0x1000             ; memory address for the read sectors (ES:BX, 0x10000)
	mov es, si
	mov bx, 0x0000
	mov di, word [TOTAL_SECTOR_COUNT]

READ_DATA:
	cmp di, 0
	je READ_END
	sub di, 1
	
	mov ah, 0x02                 ; function number (0x02: read sectors)
	mov al, 0x01                 ; read sector count
	mov ch, byte [TRACK_NUMBER]  ; read track number
	mov cl, byte [SECTOR_NUMBER] ; read sector number
	mov dh, byte [HEAD_NUMBER]   ; read head number
	mov dl, byte [BOOT_DRIVE]    ; drive number
	int 0x13                     ; cause software interrupt (0x13: BIOS Service -> Disk I/O Service)
	jc HANDLE_DISK_ERROR         ; exception handling
	
	add si, 0x0020
	mov es, si
	
	mov al, byte [SECTOR_NUMBER]
	add al, 1
	mov byte [SECTOR_NUMBER], al
	cmp al, byte [LAST_SECTOR]
	jbe READ_DATA
	
	add byte [HEAD_NUMBER], 1
	mov byte [SECTOR_NUMBER], 0x01
	mov al, byte [LAST_HEAD]
	cmp byte [HEAD_NUMBER], al
	ja .ADD_TRACK
	jmp READ_DATA

.ADD_TRACK:
	mov byte [HEAD_NUMBER], 0x00
	add byte [TRACK_NUMBER], 1
	jmp READ_DATA

READ_END:
	push LOADING_COMPLETE_MESSAGE
	push 1
	push 20
	call PRINT_MESSAGE
	add sp, 6
	
	; ====================================================================================================
	; < VBE Functions (VBE Version 2.0 or Higher) >
	; 1. function 0x4F01: get VBE mode info block.
	;    => input registers
	;       - AX: function number (0x4F01)
	;       - CX: mode number
	;       - ES: VBE mode info block segment address
	;       - DI: VBE mode info block offset
	;    => output register and address
	;       - AX: result
	;         - AL: 0x4F: function support, !0x4F: function not support
	;         - AH: 0x00: done success, 0x01 ~ 0x03: done failure or function not support
	;       - [ES:DI] address: VBE mode info block
	; 
	; 2. function 0x4F02: set VBE mode.
	;    => input registers
	;       - AX: function number (0x4F02)
	;       - BX: mode number and flags
	;         - bit 0~8: mode number
	;         - bit 9~10: reserved (0)
	;         - bit 11: 0: use default screen update count, 1: use screen update count from CRTC (It requires to set CRTC info block address to ES and DI.)
	;         - bit 12~13: reserved (0)
	;         - bit 14: 0: use window frame buffer mode, 1: use linear frame buffer mode
	;         - bit 15: 0: clear screen right after mode switching, 1: do not clear screen
	;       - ES: CRTC info block segment address
	;       - DI: CRTC info block offset
	;    => output register
	;       - AX: result
	;         - AL: 0x4F: function support, !0x4F: function not support
	;         - AH: 0x00: done success, 0x01 ~ 0x03: done failure or function not support
	; 
	; < VBE Modes >
	; ---------------------------------------------------------------
	; mode number     resolution     color count
	; ---------------------------------------------------------------
	; ...
	; 0x117           1024 * 768     16 bits (64K) color, R:G:B=5:6:5
	; ...
	; ---------------------------------------------------------------
	; 
	; ====================================================================================================
	
	; function 0x4F01: get VBE mode info block.
	mov ax, 0x4F01 ; function number
	mov cx, 0x117  ; mode number
	mov bx, 0x07E0 ; VBE mode info block segment address: VBE mode info block physical address -> [ES:DI] -> [0x07E0:0x00] -> 0x7E00
	mov es, bx
	mov di, 0x00   ; VBE mode info block offset
	int 0x10       ; cause software interrupt (0x10: BIOS Service -> Video Control Service)
	cmp ax, 0x004F ; check result if success
	jne VBE_ERROR
	
	; If graphic mode flag == [0: text mode], jump to protected mode without swithing graphic mode.
	cmp byte [GRAPHIC_MODE_FLAG], 0x00
	je JUMP_TO_PROTECTED_MODE
	
	; switch graphic mode
	; function 0x4F02: set VBE mode.
	mov ax, 0x4F02 ; function number
	mov bx, 0x4117 ; mode number 0x117, use default screen update count, use linear frame buffer mode, clear screen right after mode switching
	int 0x10       ; cause software interrupt (0x10: BIOS Service -> Video Control Service)
	cmp ax, 0x004F ; check result if success
	jne VBE_ERROR
	jmp JUMP_TO_PROTECTED_MODE
	
VBE_ERROR:
	push CHANGE_GRAPHIC_MODE_FAIL_MESSAGE
	push 2
	push 0
	call PRINT_MESSAGE
	add sp, 6
	jmp $
	
JUMP_TO_PROTECTED_MODE:
	jmp 0x1000:0x0000 ; set <0x1000> to CS segment register, and move to the address <0x10000>.

HANDLE_DISK_ERROR:
	push DISK_ERROR_MESSAGE
	push 1
	push 20
	call PRINT_MESSAGE
	add sp, 6
	jmp $

PRINT_MESSAGE:
	push bp
	mov bp, sp
	push es
	push si
	push di
	push ax
	push cx
	push dx
	
	mov ax, 0xB800
	mov es, ax
	
	mov ax, word [bp+6]
	mov si, 160
	mul si
	mov di, ax
	
	mov ax, word [bp+4]
	mov si, 2
	mul si
	add di, ax
	
	mov si, word [bp+8]

.MESSAGE_LOOP:
	mov cl, byte [si]
	cmp cl, 0
	je .MESSAGE_END
	mov byte [es:di], cl
	add si, 1
	add di, 2
	jmp .MESSAGE_LOOP

.MESSAGE_END:
	pop dx
	pop cx
	pop ax
	pop di
	pop si
	pop es
	pop bp
	ret

MESSAGE1:                         db 0 ; start boot-loader
DISK_ERROR_MESSAGE:               db 'disk error', 0
IMAGE_LOADING_MESSAGE:            db 0 ; load hOS image...
LOADING_COMPLETE_MESSAGE:         db 0 ; pass
CHANGE_GRAPHIC_MODE_FAIL_MESSAGE: db 'graphic mode switching failure', 0

SECTOR_NUMBER: db 0x02
HEAD_NUMBER:   db 0x00
TRACK_NUMBER:  db 0x00

BOOT_DRIVE:  db 0x00
LAST_SECTOR: db 0x00
LAST_HEAD:   db 0x00
LAST_TRACK:  db 0x00

times 510 - ($ - $$) db 0x00
db 0x55
db 0xAA
