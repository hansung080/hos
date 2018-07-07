#ifndef __DESCRIPTORS_H__
#define __DESCRIPTORS_H__

#include "types.h"
#include "multiprocessor.h"

/* Macros Related with GDT/TSS */

// null/code/data/TSS segment descriptor field
#define GDT_TYPE_CODE        0x0A // Code Segment (Execute/Read)
#define GDT_TYPE_DATA        0x02 // Data Segment (Read/Write)
#define GDT_TYPE_TSS         0x09 // TSS Segment (Not Busy)
#define GDT_FLAGS_LOWER_S    0x10 // Descriptor Type (1:segment descriptor, 0:system descriptor)
#define GDT_FLAGS_LOWER_DPL0 0x00 // Descriptor Privilege Level 0 (Highest, Kernel)
#define GDT_FLAGS_LOWER_DPL1 0x20 // Descriptor Privilege Level 1
#define GDT_FLAGS_LOWER_DPL2 0x40 // Descriptor Privilege Level 2
#define GDT_FLAGS_LOWER_DPL3 0x60 // Descriptor Privilege Level 3 (Lowest, User)
#define GDT_FLAGS_LOWER_P    0x80 // Present (1:valid current descriptor, 0:invalid current descriptor)
#define GDT_FLAGS_UPPER_L    0x20 // field for IA-32e mode (1: code segment for 64-bit mode of IA-32e mode, 0: code segment for 32-bit mode of IA-32e mode)
#define GDT_FLAGS_UPPER_DB   0x40 // Default Operation Size (1: 32-bit segment, 0: 16-bit segment).
#define GDT_FLAGS_UPPER_G    0x80 // Granularity (1: set segment size up to 1MB(Limit 20bits)*4KB(weight)=4GB, 0: set segment size up to 1MB(Limit 20bits))

// useful macros
#define GDT_FLAGS_LOWER_KERNELCODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_KERNELDATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_TSS        (GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERCODE   (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERDATA   (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_UPPER_CODE       (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA       (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_TSS        (GDT_FLAGS_UPPER_G)

// segment descriptor offset from GDT base address
#define GDT_KERNELCODESEGMENT 0x08
#define GDT_KERNELDATASEGMENT 0x10
#define GDT_TSSSEGMENT        0x18

// etc macros
#define GDTR_STARTADDRESS   0x142000
#define GDT_MAXENTRY8COUNT  3
#define GDT_MAXENTRY16COUNT (MAXPROCESSORCOUNT)
#define GDT_TABLESIZE       ((sizeof(GdtEntry8) * GDT_MAXENTRY8COUNT) + (sizeof(GdtEntry16) * GDT_MAXENTRY16COUNT))
#define TSS_SEGMENTSIZE     (sizeof(Tss) * MAXPROCESSORCOUNT)

/* Macros Related with IDT */

// IDT gate descriptor field
#define IDT_TYPE_INTERRUPT 0x0E // Interrupt Gate
#define IDT_TYPE_TRAP      0x0F // Trap Date
#define IDT_FLAGS_DPL0     0x00 // Descriptor Privilege Level 0 (Highest, Kernel)
#define IDT_FLAGS_DPL1     0x20 // Descriptor Privilege Level 1
#define IDT_FLAGS_DPL2     0x40 // Descriptor Privilege Level 2
#define IDT_FLAGS_DPL3     0x60 // Descriptor Privilege Level 3 (Lowest, User)
#define IDT_FLAGS_P        0x80 // Present (1:valid current descriptor, 0: invalid current descriptor)
#define IDT_FLAGS_IST0     0    // stack switching in a traditional way: switching stack only when the privilege changes.
#define IDT_FLAGS_IST1     1    // stack switching in a IST way: switching stack always, HansOS only use IST 1 out of IST 1~7).

// useful macros
#define IDT_FLAGS_KERNEL (IDT_FLAGS_DPL0 | IDT_FLAGS_P)
#define IDT_FLAGS_USER   (IDT_FLAGS_DPL3 | IDT_FLAGS_P)

// etc macros
#define IDT_MAXENTRYCOUNT 100
#define IDTR_STARTADDRESS (GDTR_STARTADDRESS + sizeof(Gdtr) + GDT_TABLESIZE + TSS_SEGMENTSIZE)
#define IDT_STARTADDRESS  (IDTR_STARTADDRESS + sizeof(Idtr))
#define IDT_TABLESIZE     (IDT_MAXENTRYCOUNT * sizeof(IdtEntry))

// macros related with IST
#define IST_STARTADDRESS 0x700000 // 7M
#define IST_SIZE         0x100000 // 1M

#pragma pack(push, 1)

// GDTR/IDTR structure (16 bytes: add padding bytes to align with 16 bytes which is the multiple of 8 bytes.)
typedef struct k_Gdtr {
	word limit;     // GDT/IDT Size
	qword baseAddr; // GDT/IDT BaseAddress
	word padding1;  // Padding Byte
	dword padding2; // Padding Byte
} Gdtr, Idtr;

// null/code/data segment descriptor (8 bytes)
typedef struct k_GdtEntry8 {      // < Assembly Code >
	word lowerLimit;              // dw 0xFFFF     ; Limit=0xFFFF
	word lowerBaseAddr;           // dw 0x0000     ; BaseAddress=0x0000
	byte upperBaseAddr1;          // db 0x00       ; BaseAddress=0x00
	byte typeAndLowerFlags;       // db 0x9A|0x92  ; P=1, DPL=00, S=1, Type=0xA:CodeSegment(Execute/Read)|0x2:DataSegment(Read/Write)
	byte upperLimitAndUpperFlags; // db 0xAF       ; G=1, D/B=0, L=1, AVL=0, Limit=0xF
	byte upperBaseAddr2;          // db 0x00       ; BaseAddress=0x00
} GdtEntry8;

// TSS segment descriptor (16 bytes)
typedef struct k_GdtEntry16 {     // < Assembly Code >
	word lowerLimit;              // dw 0xFFFF     ; Limit=0xFFFF
	word lowerBaseAddr;           // dw 0x0000     ; BaseAddress=0x0000
	byte middleBaseAddr1;         // db 0x00       ; BaseAddress=0x00
	byte typeAndLowerFlags;       // db 0x99       ; P=1, DPL=00, S=1, Type=0x9:TSSSegment(NotBusy)
	byte upperLimitAndUpperFlags; // db 0xAF       ; G=1, D/B=0, L=0, AVL=0, Limit=0xF
	byte middleBaseAddr2;         // db 0x00       ; BaseAddress=0x00
	dword upperBaseAddr;          // dd 0x00000000 ; BaseAddress=0x00000000
	dword reserved;               // dd 0x00000000 ; Reserved=0x00000000
} GdtEntry16;

// TSS segment (104 bytes)
typedef struct k_Tss {
	dword reserved1;
	qword rsp[3];
	qword reserved2;
	qword ist[7];
	qword reserved3;
	word reserved4;
	word ioMapBaseAddr;
} Tss;

// IDT gate descriptor (16byte)
typedef struct k_IdtEntry { // < Assembly Code >
	word lowerBaseAddr;     // dw 0x????     ; HandlerOffset=0x????
	word segmentSelector;   // dw 0x0008     ; KernelCodeSegmentDescriptor=0x0008
	byte ist;               // db 0x01       ; Padding=00000, IST=001
	byte typeAndFlags;      // db 0x8E       ; P=1, DPL=00, Padding=0, Type=0xE:InterruptGate|0xF:TrapGate
	word middleBaseAddr;    // dw 0x????     ; HandlerOffset=0x????
	dword upperBaseAddr;    // dd 0x???????? ; HandlerOffset=0x????????
	dword reserved;         // dd 0x00000000 ; Reserved=0x00000000
} IdtEntry;

#pragma pack(pop)

// Functions Related with GDT/TSS
void k_initGdtAndTss(void);
void k_setGdtEntry8(GdtEntry8* entry, dword baseAddr, dword limit, byte upperFlags, byte lowerFlags, byte type);
void k_setGdtEntry16(GdtEntry16* entry, qword baseAddr, dword limit, byte upperFlags, byte lowerFlags, byte type);
void k_initTss(Tss* tss);

// Functions Related with IDT
void k_initIdt(void);
void k_setIdtEntry(IdtEntry* entry, void* handler, word selector, byte ist, byte flags, byte type);

#endif // __DESCRIPTORS_H__
