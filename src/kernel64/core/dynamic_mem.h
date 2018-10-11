#ifndef __CORE_DYNAMICMEM_H__
#define __CORE_DYNAMICMEM_H__

#include "types.h"
#include "task.h"
#include "sync.h"

/**
  < Useful Bit Operations >
  - dividend: 30, divisor: 8

  @ arithmetic operations
  1. quotient  : 30 / 8 = 3
  2. remainder : 30 % 8 = 6
  3. multiple of divisor less than dividend    : (30 / 8) * 8 = 24
  4. multiple of divisor greater than dividend : ((30 + 7) / 8) * 8 = 32

  @ bit operations: only in the case that divisor is multiplier of 2, such as 1, 2, 4, 8, 16, ...
  1. quotient : 30 >> 3 = 3
         11110
	   >>)     3 
	   ---------
	       00011

  2. remainder : 30 & 0x7 = 6
         11110
      &) 00111
      --------
         00110

  3. multiple of divisor less than dividend : 30 & 0xFFFFFFF8 = 24 (in the case that dividend is 4 bytes integer)
         11110
      &) 11000
      --------
         11000

  4. multiple of divisor greater than dividend : (30 + 0x7) & 0xFFFFFFF8 = 32 (in the case that dividend is 4 bytes integer)
         11110
      +) 00111
      --------
        100101
     &) 111000
     ---------
        100000
*/

// dynamic memory start address (0xA00000, 10 MB)
// - aligned with 2 MB-level (multiple of 2 MB, rounding up, 2 MB == page size)
// - It's 10 MB if task pool size <= 2 MB (sizeof(Task) <= 2 KB).
// - Below 10 MB is kernel area, and above 10 MB is user area.
// - Kernel area is used by only kernel task, and user area is used by kernel task and user task.
#define DMEM_STARTADDRESS ((TASK_TASKPOOLENDADDRESS + 0x1FFFFF) & 0xFFFFFFFFFFE00000)

// smallest block size (1 KB)
#define DMEM_MINSIZE 1024

// bitmap flag
#define DMEM_EXIST 0x01 // block EXIST: block can be allocated.
#define DMEM_EMPTY 0x00 // block EMPTY: block can't be allocated, because it's already allocated or combined.

#pragma pack(push, 1)

typedef struct k_Bitmap {
	byte* bitmap;        // address of real bitmap: A bit in bitmap represents a block in block list.
	                     //                         - 1: exist
	                     //                         - 0: not exist
	qword existBitCount; // exist bit count: bit 1 count in bitmap
} Bitmap;

typedef struct k_DynamicMemManager {
	Spinlock spinlock;             // spinlock
	int maxLevelCount;             // block list count (level count)
	int smallestBlockCount;        // smallest block count
	qword usedSize;                // used memory size
	qword startAddr;               // block pool start address
	qword endAddr;                 // block pool end address
	byte* allocatedBlockListIndex; // address of index area (address of area saving allocated block list index)
	Bitmap* bitmapOfLevel;         // address of bitmap structure
} DynamicMemManager;

#pragma pack(pop)

void k_initDynamicMem(void);
void* k_allocMem(qword size);
bool k_freeMem(void* addr);
void k_getDynamicMemInfo(qword* startAddr, qword* totalSize, qword* metaSize, qword* usedSize);
DynamicMemManager* k_getDynamicMemManager(void);
static qword k_calcDynamicMemSize(void);
static int k_calcMetaBlockCount(qword dynamicRamSize);
static int k_allocBuddyBlock(qword alignedSize);
static qword k_getBuddyBlockSize(qword size);
static int k_getBlockListIndexByMatchSize(qword alignedSize);
static int k_findFreeBlockInBitmap(int blockListIndex);
static void k_setFlagInBitmap(int blockListIndex, int offset, byte flag);
static bool k_freeBuddyBlock(int blockListIndex, int blockOffset);
static byte k_getFlagInBitmap(int blockListIndex, int offset);

#endif // __CORE_DYNAMICMEM_H__
