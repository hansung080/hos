#include "page.h"

/**
  < Intel 80x86 Memory Address >
                                           linear address: 4 GB
    logical address  --------------------- (virtual address)     --------------- physical address
    ---------------> | Segmentation Unit | --------------------> | Paging Unit | ---------------->
                     ---------------------                       ---------------

  < hOS Segmentation >
  - protected mode: divide physical memory into segments by segment descriptors.
  - IA-32e mode: consider whole physical memory as a segment.
                 Segmentation output is linear address which is paging input.
                 Linear address represents entry offset in each page tables and address offset in page.

  < hOS Paging: 4 level paging with 2 MB page >
  CR3 control register
  -> PML4 table
  -> page directory pointer table
  -> page directory
  -> page (2 MB) in physical memory 
*/

void k_initPageTables(void) {
	Pml4tEntry* pml4tEnty;
	PdptEntry* pdptEntry;
	PdEntry* pdEntry;
	dword mappingAddr;
	int i;
	
	// create PML4 table (4 KB): 1 table, 1 entry.
	pml4tEnty = (Pml4tEntry*)0x100000; // 1 MB
	k_setPageTableEntry(&(pml4tEnty[0]), 0x00, 0x101000, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);
	for (i = 1; i < PAGE_MAXENTRYCOUNT; i++) {
		k_setPageTableEntry(&(pml4tEnty[i]), 0, 0, 0, 0);
	}
	
	// create page directory pointer table (4 KB): 1 table, 64 entries.
	pdptEntry = (PdptEntry*)0x101000; // 1 MB + 4 KB
	for (i = 0; i < 64; i++) {
		k_setPageTableEntry(&(pdptEntry[i]), 0x00, 0x102000 + (i * PAGE_TABLESIZE), PAGE_FLAGS_DEFAULT | PAGE_FLAGS_US, 0);
	}
	
	for (i = 64; i < PAGE_MAXENTRYCOUNT; i++) {
		k_setPageTableEntry(&(pdptEntry[i]), 0, 0, 0, 0);
	}
	
	// create page directory (4 KB * 64 = 256 KB): 64 tables, 512 * 64 = 32768 entries.
	// - total 66 tables, 264 KB memory.
	// - supportable physical memory size: 1 GB * 64 = 64 GB
	pdEntry = (PdEntry*)0x102000; // 1 MB + 4 KB + 4 KB
	mappingAddr = 0;
	for (i = 0; i < (PAGE_MAXENTRYCOUNT * 64); i++) {
		// If page is in kernel area.
		if (i < ((dword)PAGE_DMEM_STARTADDRESS / PAGE_DEFAULTSIZE)) {
			k_setPageTableEntry(&(pdEntry[i]), (i * (PAGE_DEFAULTSIZE >> 20)) >> 12, mappingAddr, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS, 0);

		// If page is in user area.
		} else {
			k_setPageTableEntry(&(pdEntry[i]), (i * (PAGE_DEFAULTSIZE >> 20)) >> 12, mappingAddr, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS | PAGE_FLAGS_US, 0);
		}
		
		mappingAddr += PAGE_DEFAULTSIZE;
	}
}

void k_setPageTableEntry(PtEntry* entry, dword upperBaseAddr, dword lowerBaseAddr, dword lowerFlags, dword upperFlags) {
	entry->attrAndLowerBaseAddr = lowerBaseAddr | lowerFlags;
	entry->upperBaseAddrAndExb = (upperBaseAddr & 0xFF) | upperFlags;
}
