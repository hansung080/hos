#include "types.h"
#include "page.h"
#include "mode_switch.h"

// BSP flag
#define BSPFLAG     *(byte*)0x7C09 // BSP flag (1:BSP, 0:AP)
#define BSPFLAG_BSP 0x01           // BSP
#define BSPFLAG_AP  0x00           // AP

void k_printStr(int x, int y, const char* str);
bool k_isMemEnough(void);
bool k_initKernel64Area(void);
void k_copyKernel64To2MB(void);

void k_main(void) {
	int y = 1; // y of cursor, blank line 0.
	dword eax, ebx, ecx, edx;
	char vendor[13] = {0, };
	
	// compare BSP flag
	if (BSPFLAG == BSPFLAG_AP) {
		// switch to kernel64
		k_switchToKernel64();
		
		while (true);
	}
	
	// print the first message of kernel32 at line 1.
	k_printStr(0, y++, "*** HansOS Initialization ***");
	
	// [Note]
	// print boot-loader messages here,
	// and remove messages from boot-loader, because of the risk of duplication between messages and USB partition info in boot-loader.
	k_printStr(0, y++, "- start boot-loader..........................pass");
	k_printStr(0, y++, "- load HansOS image..........................pass");
	
	// already printed 'switch to protected mode' message at line 4 in entry_point.s.
	y++;
	
	// print the start message of C kernel32.
	k_printStr(0, y++, "- start protected mode C kernel..............pass");
	
	// check minimum memory size if over 64 MBytes.
	k_printStr(0, y, "- check mininum memory size (64 MB)..........");
	if (k_isMemEnough() == true) {
		k_printStr(45, y++, "pass");
		
	} else {
		k_printStr(45, y++, "fail");
		while (true);
	}
	
	// initialize the memory area of kernel64.
	k_printStr(0, y, "- initialize IA-32e mode kernel area.........");
	if (k_initKernel64Area() == true) {
		k_printStr(45, y++, "pass");
		
	} else {
		k_printStr(45, y++, "fail");
		while (true);
	}
	
	// initialize the page tables of kernel64.
	k_printStr(0, y, "- initialize IA-32e mode page tables.........");
	k_initPageTables();
	k_printStr(45, y++, "pass");
	
	// read processor vendor name.
	k_readCpuid(0x00000000, &eax, &ebx, &ecx, &edx);
	*((dword*)vendor) = ebx;
	*((dword*)vendor + 1) = edx;
	*((dword*)vendor + 2) = ecx;
	k_printStr(0, y, "- read processor vendor name.................");
	k_printStr(45, y++, vendor);
	
	// check if processor supports 64-bit mode.
	k_readCpuid(0x80000001, &eax, &ebx, &ecx, &edx);
	k_printStr(0, y, "- check 64-bit mode support..................");
	if (edx & (1 << 29)) {
		k_printStr(45, y++, "pass");
		
	} else {
		k_printStr(45, y++, "fail");
		while (true);
	}
	
	// print the last message of kernel32 at line 11.
	// copy kernel64 to the address <0x200000 (2 MB)>.
	k_printStr(0, y, "- copy IA-32e mode kernel to 2 MB address....");
	k_copyKernel64To2MB();
	k_printStr(45, y++, "pass");
	
	// switch to kernel64
	k_switchToKernel64();
	
	while (true);
}

void k_printStr(int x, int y, const char* str) {
	Char* screen = (Char*)0xB8000;
	int i;

	screen += (y*80) + x;

	for (i = 0; str[i] != null; i++) {
		screen[i].char_ = str[i];
	}
}

bool k_isMemEnough(void) {
	dword* currentAddr = (dword*)0x100000; // 1 MB

	while ((dword)currentAddr < 0x4000000) { // 64 MB
		*currentAddr = 0x12345678;

		if (*currentAddr != 0x12345678) {
			return false;
		}

		currentAddr += (0x100000 / 4); // increase by 1 MB.
	}

	return true;
}

bool k_initKernel64Area(void) {
	dword* currentAddr = (dword*)0x100000; // 1 MB

	while ((dword)currentAddr < 0x600000) { // 6 MB
		*currentAddr = 0x00;

		if (*currentAddr != 0x00) {
			return false;
		}

		currentAddr++;
	}

	return true;
}

void k_copyKernel64To2MB(void) {
	word totalSectorCount, kernel32SectorCount;
	dword* srcAddr, * destAddr;
	int i;

	totalSectorCount = *((word*)0x7C05);
	kernel32SectorCount = *((word*)0x7C07);

	srcAddr = (dword*)(0x10000 + (kernel32SectorCount * 512));
	destAddr = (dword*)0x200000;

	for (int i = 0; i < (((totalSectorCount - kernel32SectorCount) * 512) / 4); i++) {
		*destAddr = *srcAddr;
		destAddr++;
		srcAddr++;
	}
}
