#include "page.h"

void k_initPageTables(void) {
	Pml4tEntry* pml4tEnty;
	PdptEntry* pdptEntry;
	PdEntry* pdEntry;
	dword mappingAddr;
	int i;
	
	// create PML4 table (4KB): 1 table, 1 entry.
	pml4tEnty = (Pml4tEntry*)0x100000; // 1MB
	k_setPageTableEntry(&(pml4tEnty[0]), 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0);
	for (i = 1; i < PAGE_MAX_ENTRY_COUNT; i++) {
		k_setPageTableEntry(&(pml4tEnty[i]), 0, 0, 0, 0);
	}
	
	// create page directory pointer table (4KB): 1 table, 64 entries.
	pdptEntry = (PdptEntry*)0x101000; // 1MB + 4KB
	for (i = 0; i < 64; i++) {
		k_setPageTableEntry(&(pdptEntry[i]), 0x00, 0x102000 + (i * PAGE_TABLE_SIZE), PAGE_FLAGS_DEFAULT, 0);
	}
	
	for (i = 64; i < PAGE_MAX_ENTRY_COUNT; i++) {
		k_setPageTableEntry(&(pdptEntry[i]), 0, 0, 0, 0);
	}
	
	// create page directory (4*64=256KB): 64 tables, 512*64=32768 entries.
	// - total 66 tables, 264KB memory.
	// - supportable physical memory size: 1GB*64=64GB
	pdEntry = (PdEntry*)0x102000; // 1MB + 4KB + 4KB
	mappingAddr = 0;
	for (i = 0; i < (PAGE_MAX_ENTRY_COUNT * 64); i++) {
		k_setPageTableEntry(&(pdEntry[i]), (i * (PAGE_DEFAULT_SIZE >> 20)) >> 12, mappingAddr, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS, 0);
		mappingAddr += PAGE_DEFAULT_SIZE;
	}
}

void k_setPageTableEntry(PtEntry* entry, dword upperBaseAddr, dword lowerBaseAddr, dword lowerFlags, dword upperFlags) {
	entry->attrAndLowerBaseAddr = lowerBaseAddr | lowerFlags;
	entry->upperBaseAddrAndExb = (upperBaseAddr & 0xFF) | upperFlags;
}
