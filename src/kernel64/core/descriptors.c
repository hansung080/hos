#include "descriptors.h"
#include "../utils/util.h"
#include "isr.h"

void k_initGdtAndTss(void) {
	Gdtr* gdtr;
	GdtEntry8* entry;
	Tss* tss;
	int i;
	
	// create GDTR.
	gdtr = (Gdtr*)GDTR_STARTADDRESS;
	entry = (GdtEntry8*)(GDTR_STARTADDRESS + sizeof(Gdtr));
	gdtr->limit = GDT_TABLESIZE - 1;
	gdtr->baseAddr = (qword)entry;
	gdtr->padding1 = 0;
	gdtr->padding2 = 0;
	
	// set TSS address.
	tss = (Tss*)((qword)entry + GDT_TABLESIZE);
	
	// create GDT (null, kernel code, kernel data, user data, user code, and TSS segment descriptors).
	k_setGdtEntry8(&(entry[0]), 0, 0, 0, 0, 0);
	k_setGdtEntry8(&(entry[1]), 0x00000000, 0xFFFFF, GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_KERNELCODE, GDT_TYPE_CODE);
	k_setGdtEntry8(&(entry[2]), 0x00000000, 0xFFFFF, GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_KERNELDATA, GDT_TYPE_DATA);
	k_setGdtEntry8(&(entry[3]), 0x00000000, 0xFFFFF, GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_USERDATA, GDT_TYPE_DATA);
	k_setGdtEntry8(&(entry[4]), 0x00000000, 0xFFFFF, GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_USERCODE, GDT_TYPE_CODE);
	for (i = 0; i < MAXPROCESSORCOUNT; i++) { 
		k_setGdtEntry16((GdtEntry16*)&(entry[GDT_MAXENTRY8COUNT + (i * 2)]), (qword)tss + (i * sizeof(Tss)), sizeof(Tss) - 1, GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS, GDT_TYPE_TSS);
	}
	
	// create TSS.
	k_initTss(tss);
}

void k_setGdtEntry8(GdtEntry8* entry, dword baseAddr, dword limit, byte upperFlags, byte lowerFlags, byte type) {
	entry->lowerLimit = limit & 0xFFFF;
	entry->lowerBaseAddr = baseAddr & 0xFFFF;
	entry->upperBaseAddr1 = (baseAddr >> 16) & 0xFF;
	entry->typeAndLowerFlags = lowerFlags | type;
	entry->upperLimitAndUpperFlags = upperFlags | ((limit >> 16) & 0xF);
	entry->upperBaseAddr2 = (baseAddr >> 24) & 0xFF;
}

void k_setGdtEntry16(GdtEntry16* entry, qword baseAddr, dword limit, byte upperFlags, byte lowerFlags, byte type) {
	entry->lowerLimit = limit & 0xFFFF;
	entry->lowerBaseAddr = baseAddr & 0xFFFF;
	entry->middleBaseAddr1 = (baseAddr >> 16) & 0xFF;
	entry->typeAndLowerFlags = lowerFlags | type;
	entry->upperLimitAndUpperFlags = upperFlags | ((limit >> 16) & 0xF);
	entry->middleBaseAddr2 = (baseAddr >> 24) & 0xFF;
	entry->upperBaseAddr = (baseAddr >> 32) & 0xFFFFFFFF;
	entry->reserved = 0;
}

/**
  < IST Address of Each Core >
    - Total IST area is 1 MB size (7MB ~ 8MB).
    - A core has 64 KB-sized IST which is 1 MB divided by 16 cores.
    - Cores get assigned IST from the end of area, because data is added to stack from the end.
    
    ------------------- 8 MB - (64 KB * 0)
    | core 0 (64 KB)  |
    ------------------- 8 MB - (64 KB * 1)
    | core 1 (64 KB)  |
    ------------------- 8 MB - (64 KB * 2)
    | core 2 (64 KB)  |
    ------------------- 8 MB - (64 KB * 3)
    |      ...        |
    ------------------- 8 MB - (64 KB * 15)
    | core 15 (64 KB) |
    ------------------- 7 MB
*/

void k_initTss(Tss* tss) {
	int i;
	
	// create TSS segments as many as max processor count.
	for (i = 0; i < MAXPROCESSORCOUNT; i++) {
		k_memset(tss, 0, sizeof(Tss));
		
		// allocate from the end of IST area. (IST must be aligned with 16 bytes.)
		tss->ist[0] = IST_STARTADDRESS + IST_SIZE - (IST_SIZE / MAXPROCESSORCOUNT * i);
		
		// set IO map base address more than Limit field of TSS segment descriptor, in order not to use IO map.
		tss->ioMapBaseAddr = 0xFFFF;
		
		tss++;
	}
}

void k_initIdt(void) {
	Idtr* idtr;
	IdtEntry* entry;
	int i;
	
	// create IDTR.
	idtr = (Idtr*)IDTR_STARTADDRESS;
	entry = (IdtEntry*)(IDTR_STARTADDRESS + sizeof(Idtr));
	idtr->limit = IDT_TABLESIZE - 1;
	idtr->baseAddr = (qword)entry;
	
	/**
	  < IDT >
	  - vector 0 ~ 31    : exception handlers
	  - vector 32 ~ 47   : interrupt handlers (interrupt from ISA bus)
	  - vector 48 ~ 99   : interrupt hanlders (etc interrupt)
	  - vector 100 ~ 255 : hOS do not use
	*/
	
	// create IDT (100 IDT gate descriptors): put ISR to 0 ~ 99 vectors of IDT.
	// Exception Handling ISR (21): #0 ~ #19, #20 ~ #31
	k_setIdtEntry(&(entry[0]),  k_isrDivideError,               GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[1]),  k_isrDebug,                     GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[2]),  k_isrNmi,                       GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[3]),  k_isrBreakPoint,                GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[4]),  k_isrOverflow,                  GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[5]),  k_isrBoundRangeExceeded,        GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[6]),  k_isrInvalidOpcode,             GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[7]),  k_isrDeviceNotAvailable,        GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[8]),  k_isrDoubleFault,               GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[9]),  k_isrCoprocessorSegmentOverrun, GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[10]), k_isrInvalidTss,                GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[11]), k_isrSegmentNotPresent,         GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[12]), k_isrStackSegmentFault,         GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[13]), k_isrGeneralProtection,         GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[14]), k_isrPageFault,                 GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[15]), k_isr15,                        GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[16]), k_isrFpuError,                  GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[17]), k_isrAlignmentCheck,            GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[18]), k_isrMachineCheck,              GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[19]), k_isrSimdError,                 GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	for (i = 20; i < 32; i++) {
		k_setIdtEntry(&(entry[i]), k_isrEtcException,           GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	}
	
	// Interrupt Handling ISR (17): #32 ~ #47, #48 ~ #99
	k_setIdtEntry(&(entry[32]), k_isrTimer,                     GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[33]), k_isrKeyboard,                  GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[34]), k_isrSlavePic,                  GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[35]), k_isrSerialPort2,               GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[36]), k_isrSerialPort1,               GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[37]), k_isrParallelPort2,             GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[38]), k_isrFloppyDisk,                GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[39]), k_isrParallelPort1,             GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[40]), k_isrRtc,                       GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[41]), k_isrReserved,                  GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[42]), k_isrNotUsed1,                  GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[43]), k_isrNotUsed2,                  GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[44]), k_isrMouse,                     GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[45]), k_isrCoprocessor,               GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[46]), k_isrHdd1,                      GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	k_setIdtEntry(&(entry[47]), k_isrHdd2,                      GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	for (i = 48; i < IDT_MAXENTRYCOUNT; i++) {
		k_setIdtEntry(&(entry[i]), k_isrEtcInterrupt,           GDT_OFFSET_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	}
}

void k_setIdtEntry(IdtEntry* entry, void* handler, word selector, byte ist, byte flags, byte type) {
	entry->lowerBaseAddr = (qword)handler & 0xFFFF;
	entry->segmentSelector = selector;
	entry->ist = ist & 0x7;
	entry->typeAndFlags = flags | type;
	entry->middleBaseAddr = ((qword)handler >> 16) & 0xFFFF;
	entry->upperBaseAddr = ((qword)handler >> 32) & 0xFFFFFFFF;
	entry->reserved = 0;
}
