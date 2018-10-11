#ifndef __PAGE_H__
#define __PAGE_H__

#include "types.h"
#include "../kernel64/core/task.h"

// page table entry fields
#define PAGE_FLAGS_P   0x00000001 // present: 1: valid, 0: invalid
#define PAGE_FLAGS_RW  0x00000002 // read/write: 1: read/write, 0: read olny
#define PAGE_FLAGS_US  0x00000004 // user/supervisor: 1: user (Ring 3), 0: supervisor (Ring 0~2)
#define PAGE_FLAGS_PWT 0x00000008 // page-level write through: 1: write through, 0: write back
#define PAGE_FLAGS_PCD 0x00000010 // page-level cache disable: 1: cache disable, 0: cache enable
#define PAGE_FLAGS_A   0x00000020 // accessed: 1: read or wrote, 0: not read or wrote
#define PAGE_FLAGS_D   0x00000040 // dirty: 1: wrote, 0: not wrote
#define PAGE_FLAGS_PS  0x00000080 // page size: 1: 2 MB (CR4.PAE == 1), 4 MB (CR4.PAE == 0), 0: 4 KB
#define PAGE_FLAGS_G   0x00000100 // global: 1: do not replace page in TLB when replacing CR3, 0: replace page (TLB: page table cache)
#define PAGE_FLAGS_PAT 0x00001000 // page attribute table index: If processor supports PAT, select PAT using PAT, PCD, PWD. If not, reserved as 0. 
#define PAGE_FLAGS_EXB 0x80000000 // execute disable: 1: execute disable, 0: execute enable

// useful macros
#define PAGE_FLAGS_DEFAULT (PAGE_FLAGS_P | PAGE_FLAGS_RW)

// etc macros
#define PAGE_TABLESIZE     0x1000   // 4 KB
#define PAGE_MAXENTRYCOUNT 512
#define PAGE_DEFAULTSIZE   0x200000 // 2 MB

// dynamic memory start address (0xA00000, 10 MB)
#define PAGE_DMEM_STARTADDRESS ((TASK_TASKPOOLENDADDRESS + 0x1FFFFF) & 0xFFE00000)

#pragma pack(push, 1)

typedef struct k_PageTableEntry {
	dword attrAndLowerBaseAddr;
	dword upperBaseAddrAndExb;
} Pml4tEntry, PdptEntry, PdEntry, PtEntry;

#pragma pack(pop)

void k_initPageTables(void);
void k_setPageTableEntry(PtEntry* entry, dword upperBaseAddr, dword lowerBaseAddr, dword lowerFlags, dword upperFlags);

#endif // __PAGE_H__
