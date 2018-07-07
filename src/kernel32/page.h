#ifndef __PAGE_H__
#define __PAGE_H__

#include "types.h"

// page table entry fields
#define PAGE_FLAGS_P   0x00000001 // (1 << 0)  : Present
#define PAGE_FLAGS_RW  0x00000002 // (1 << 1)  : Read/Write
#define PAGE_FLAGS_US  0x00000004 // (1 << 2)  : User/Supervisor
#define PAGE_FLAGS_PWT 0x00000008 // (1 << 3)  : Page-level Write Through
#define PAGE_FLAGS_PCD 0x00000010 // (1 << 4)  : Page-level Cache Disable
#define PAGE_FLAGS_A   0x00000020 // (1 << 5)  : Accessed
#define PAGE_FLAGS_D   0x00000040 // (1 << 6)  : Dirty
#define PAGE_FLAGS_PS  0x00000080 // (1 << 7)  : Page Size
#define PAGE_FLAGS_G   0x00000100 // (1 << 8)  : Global
#define PAGE_FLAGS_PAT 0x00001000 // (1 << 12) : Page Attribute Table Index
#define PAGE_FLAGS_EXB 0x80000000 // (1 << 31) : Execute Disable

// useful macros
#define PAGE_FLAGS_DEFAULT (PAGE_FLAGS_P | PAGE_FLAGS_RW)

// etc macros
#define PAGE_TABLE_SIZE      0x1000   // 4KB
#define PAGE_MAX_ENTRY_COUNT 512
#define PAGE_DEFAULT_SIZE    0x200000 // 2MB

#pragma pack(push, 1)

typedef struct k_PageTableEntry {
	dword attrAndLowerBaseAddr;
	dword upperBaseAddrAndExb;
} Pml4tEntry, PdptEntry, PdEntry, PtEntry;

#pragma pack(pop)

void k_initPageTables(void);
void k_setPageTableEntry(PtEntry* entry, dword upperBaseAddr, dword lowerBaseAddr, dword lowerFlags, dword upperFlags);

#endif // __PAGE_H__
