#include "dynamic_mem.h"
#include "util.h"
#include "sync.h"
#include "console.h"

static DynamicMemManager g_dynamicMemManager;

void k_initDynamicMem(void) {
	qword dynamicMemSize;
	int i, j;
	byte* currentBitmapPos;
	int blockCountOfLevel, metaBlockCount;
	
	dynamicMemSize = k_calcDynamicMemSize();
	metaBlockCount = k_calcMetaBlockCount(dynamicMemSize);
	
	// set smallest block count.
	g_dynamicMemManager.smallestBlockCount = (dynamicMemSize / DMEM_MIN_SIZE) - metaBlockCount;
	
	// divide smallest block count by 2^i gradually, and get block list count.
	for (i = 0; (g_dynamicMemManager.smallestBlockCount >> i) > 0; i++) {
		;
	}
	g_dynamicMemManager.maxLevelCount = i;
	
	//----------------------------------------------------------------------------------------------------
	// Index Area Initialization
	//----------------------------------------------------------------------------------------------------
	g_dynamicMemManager.allocatedBlockListIndex = (byte*)DMEM_START_ADDRESS;
	for (i = 0; i < g_dynamicMemManager.smallestBlockCount; i++) {
		g_dynamicMemManager.allocatedBlockListIndex[i] = 0xFF;
	}
	
	// set the address of bitmap structure.
	g_dynamicMemManager.bitmapOfLevel = (Bitmap*)(DMEM_START_ADDRESS + (sizeof(byte) * g_dynamicMemManager.smallestBlockCount));
	
	// set the address of real bitmap.
	currentBitmapPos = ((byte*)g_dynamicMemManager.bitmapOfLevel) + (sizeof(Bitmap) * g_dynamicMemManager.maxLevelCount);
	
	// initialize bitmap looping by block list.
	// set EXIST to the biggest block and the leftover blocks, and set EMPTY to the other blocks.
	for (j = 0; j < g_dynamicMemManager.maxLevelCount; j++) {
		
		//----------------------------------------------------------------------------------------------------
		// Bit Map Structure Initialization
		//----------------------------------------------------------------------------------------------------
		g_dynamicMemManager.bitmapOfLevel[j].bitmap = currentBitmapPos;
		g_dynamicMemManager.bitmapOfLevel[j].existBitCount = 0;
		
		//----------------------------------------------------------------------------------------------------
		// Real Bit Map Initialization
		//----------------------------------------------------------------------------------------------------
		blockCountOfLevel = g_dynamicMemManager.smallestBlockCount >> j;
		
		// If block count >= 8, set EMPTY, because these blocks can be combined with high blocks.
		for (i = 0; i < (blockCountOfLevel / 8); i++) {
			*currentBitmapPos = DMEM_EMPTY;
			currentBitmapPos++;
		}
		
		// process the remaining blocks with block count which is not divided by 8.
		if ((blockCountOfLevel % 8) != 0) {
			*currentBitmapPos = DMEM_EMPTY;
			
			// If remaining block count is odd, the last block can't be combined with high block.
			// Thus, it sets the last block as a leftover block in current block list.
			i = blockCountOfLevel % 8;
			if ((i % 2) == 1) {
				*currentBitmapPos |= (DMEM_EXIST << (i - 1));
				g_dynamicMemManager.bitmapOfLevel[j].existBitCount = 1;
			}
			
			currentBitmapPos++;
		}
	}
	
	// set block pool address and used memory size.
	g_dynamicMemManager.startAddr = DMEM_START_ADDRESS + (metaBlockCount * DMEM_MIN_SIZE);
	g_dynamicMemManager.endAddr = DMEM_START_ADDRESS + k_calcDynamicMemSize();
	g_dynamicMemManager.usedSize = 0;
	
	// initialize spinlock.
	k_initSpinlock(&(g_dynamicMemManager.spinlock));
}

static qword k_calcDynamicMemSize(void) {
	qword ramSize;
	
	ramSize = (k_getTotalRamSize() * 1024 * 1024);
	
	// use dynamic memory area up to the maximum 3GB,
	// because graphic video memory and processor-using registers exist in the above of 3GB.
	if (ramSize > ((qword)3 * 1024 * 1024 * 1024)) {
		ramSize = ((qword)3 * 1024 * 1024 * 1024);
	}
	
	return ramSize - DMEM_START_ADDRESS;
}

static int k_calcMetaBlockCount(qword dynamicRamSize) {
	long smallestBlockCount;
	dword allocatedBlockListIndexSize;
	dword bitmapSize;
	long i;
	
	smallestBlockCount = dynamicRamSize / DMEM_MIN_SIZE;
	
	// calculate the size of index area.
	allocatedBlockListIndexSize = smallestBlockCount * sizeof(byte);
	
	bitmapSize = 0;
	for (i = 0; (smallestBlockCount >> i) > 0; i++) {
		// calculate the size of bitmap structure area.
		bitmapSize += sizeof(Bitmap);
		
		// calculate the size of real bitmap area (aligned with byte unit, rounding up)
		bitmapSize += ((smallestBlockCount >> i) + 7) / 8;
	}
	
	// align the size of meta block area with smallest block count. (rounding up)
	return (allocatedBlockListIndexSize + bitmapSize + (DMEM_MIN_SIZE - 1)) / DMEM_MIN_SIZE;
}

void* k_allocMem(qword size) {
	qword alignedSize;   // aligned with buddy block size
	qword relativeAddr;  // relative address from block pool start address
	long offset;         // bitmap offset of allocated block
	int sizeArrayOffset; // byte unit offset of allocated block
	int blockListIndex;  // block list index matching block size
	
	// search buddy block size which is the closest one to the allocating memory size.
	alignedSize = k_getBuddyBlockSize(size);
	if (alignedSize == 0) {
		k_printf("dynamic memory error: can not get buddy block size\n");
		return null;
	}
	
	// fail if free memory is not enough.
	if ((g_dynamicMemManager.startAddr + g_dynamicMemManager.usedSize + alignedSize) > g_dynamicMemManager.endAddr) {
		k_printf("dynamic memory error: not enough free memory\n");
		return null;
	}
	
	// allocate buddy block, and return bitmap offset of block list of the allocated block.
	offset = k_allocBuddyBlock(alignedSize);
	if (offset == -1) {
		k_printf("dynamic memory error: buddy block allocation error\n");
		return null;
	}
	
	// search block list index of block list matching block size.
	blockListIndex = k_getBlockListIndexByMatchSize(alignedSize);
	
	// save block list index to the byte unit offset position of the allocated block in index area.
	// When freeing memory, get used memory size using block list index.
	relativeAddr = alignedSize * offset;
	sizeArrayOffset = relativeAddr / DMEM_MIN_SIZE;
	g_dynamicMemManager.allocatedBlockListIndex[sizeArrayOffset] = (byte)blockListIndex;
	
	// increase used memory size.
	g_dynamicMemManager.usedSize += alignedSize;
	
	// return the absolute address of allocated memory. (absolute address = block pool start address + relative address)
	return (void*)(g_dynamicMemManager.startAddr + relativeAddr);
}

static qword k_getBuddyBlockSize(qword size) {
	long i;
	
	// search buddy block size which is the closest one to the parameter size.
	for (i = 0; i < g_dynamicMemManager.maxLevelCount; i++) {
		if (size <= (DMEM_MIN_SIZE << i)) {
			return (DMEM_MIN_SIZE << i);
		}
	}
	
	return 0;
}

static int k_allocBuddyBlock(qword alignedSize) {
	int blockListIndex; // block list index matching block size
	int freeOffset;     // bitmap offset of existing block
	int i;
	
	// search block list index matching block size.
	blockListIndex = k_getBlockListIndexByMatchSize(alignedSize);
	if (blockListIndex == -1) {
		return -1;
	}
	
	k_lockSpin(&(g_dynamicMemManager.spinlock));
	
	// search EXIST block going up from matching block list to the highest block list.
	for (i = blockListIndex; i < g_dynamicMemManager.maxLevelCount; i++) {
		
		// search EXIST block checking bitmap of block list.
		freeOffset = k_findFreeBlockInBitmap(i);
		if (freeOffset != -1) {
			break;
		}
	}
	
	// fail if EXIST blocks don't exist, after searching finishes
	if (freeOffset == -1) {
		k_unlockSpin(&(g_dynamicMemManager.spinlock));
		return -1;
	}
	
	// set the searched block to EMPTY.
	k_setFlagInBitmap(i, freeOffset, DMEM_EMPTY);
	
	// If the searched block was in high block list, divide high block.
	if (i > blockListIndex) {
		
		// divide block going down from searched block list to matching block list.
		for (i = i - 1; i >= blockListIndex; i--) {
			// set EMPTY to the left block.
			k_setFlagInBitmap(i, freeOffset * 2, DMEM_EMPTY);
			
			// set EXIST to the right block.
			k_setFlagInBitmap(i, freeOffset * 2 + 1, DMEM_EXIST);
			
			// device the left block again.
			freeOffset = freeOffset * 2;
		}
	}
	
	k_unlockSpin(&(g_dynamicMemManager.spinlock));
	
	return freeOffset;
}

static int k_getBlockListIndexByMatchSize(qword alignedSize) {
	int i;
	
	// search block list index matching block size (block list index means level)
	for (i = 0; i < g_dynamicMemManager.maxLevelCount; i++) {
		if (alignedSize <= (DMEM_MIN_SIZE << i)) {
			return i;
		}
	}
	
	return -1;
}

static int k_findFreeBlockInBitmap(int blockListIndex) {
	int i, maxCount;
	byte* bitmap;
	qword* bitmap_;
	
	// fail if bit 1 count in bitmap == 0
	if (g_dynamicMemManager.bitmapOfLevel[blockListIndex].existBitCount == 0) {
		return -1;
	}
	
	// get block count of block list, and search bitmap as many as block count.
	maxCount = g_dynamicMemManager.smallestBlockCount >> blockListIndex;
	bitmap = g_dynamicMemManager.bitmapOfLevel[blockListIndex].bitmap;
	
	for (i = 0; i < maxCount;) {
		
		// If block count >= 64, check if bit 1 exists through qword (64 bits).
		if (((maxCount - i) / 64) > 0) {
			
			bitmap_ = (qword*)&(bitmap[i/8]);
			
			// be except if every single bit in 64 bits == 0.
			if (*bitmap_ == 0) {
				i += 64;
				continue;
			}
		}
		
		// If offset bit in bitmap == 1, return the offset (return bitmap offset of EXIST block.)
		if ((bitmap[i/8] & (DMEM_EXIST << (i % 8))) != 0) {
			return i;
		}
		
		i++;
	}
	
	return -1;
}

static void k_setFlagInBitmap(int blockListIndex, int offset, byte flag) {
	byte* bitmap;
	
	bitmap = g_dynamicMemManager.bitmapOfLevel[blockListIndex].bitmap;
	
	// set EXIST to block
	if (flag == DMEM_EXIST) {
		// If offset bit in bitmap changes from 0 (EMPTY) to 1 (EXIST), increase EXIST bit count.
		if ((bitmap[offset/8] & (0x01 << (offset % 8))) == 0) {
			g_dynamicMemManager.bitmapOfLevel[blockListIndex].existBitCount++;
		}
		
		// set offset bit in bitmap to 1 (EXIST).
		bitmap[offset/8] |= (0x01 << (offset % 8));
		
	// set EMPTY to block
	} else {
		// If offset bit in bitmap changes from 1 (EXIST) to 0 (EMPTY), decrease EXIST bit count.
		if ((bitmap[offset/8] & (0x01 << (offset % 8))) != 0) {
			g_dynamicMemManager.bitmapOfLevel[blockListIndex].existBitCount--;
		}
		
		// set offset bit in bitmap to 0 (EMPTY).
		bitmap[offset/8] &= ~(0x01 << (offset % 8));
	}
}

bool k_freeMem(void* addr) {
	qword relativeAddr;  // relative address of free block (relative address from block pool start address)
	int sizeArrayOffset; // byte unit offset of free block
	qword blockSize;     // size of free block
	int blockListIndex;  // block list index of free block
	int bitmapOffset;    // bitmap offset of free block
	
	if (addr == null) {
		k_printf("dynamic memory error: address is null\n");
		return false;
	}
	
	// calculate relative address and byte unit offset
	relativeAddr = ((qword)addr) - g_dynamicMemManager.startAddr;
	sizeArrayOffset = relativeAddr / DMEM_MIN_SIZE;
	
	// fail if it has not been allocated.
	if (g_dynamicMemManager.allocatedBlockListIndex[sizeArrayOffset] == 0xFF) {
		k_printf("dynamic memory error: not allocated memory\n");
		return false;
	}
	
	// get block list index, and initialize it.
	blockListIndex = (int)g_dynamicMemManager.allocatedBlockListIndex[sizeArrayOffset];
	g_dynamicMemManager.allocatedBlockListIndex[sizeArrayOffset] = 0xFF;
	
	// calculate the size of free block.
	blockSize = DMEM_MIN_SIZE << blockListIndex;
	
	// free buddy block using block list index and bitmap offset.
	bitmapOffset = relativeAddr / blockSize;
	if (k_freeBuddyBlock(blockListIndex, bitmapOffset) == true) {
		
		// decrease used memory size.
		g_dynamicMemManager.usedSize -= blockSize;
		return true;
	}
	
	k_printf("dynamic memory error: buddy block freeing error\n");
	
	return false;
}

static bool k_freeBuddyBlock(int blockListIndex, int blockOffset) {
	int buddyBlockOffset;   // buddy block offset
	int i;                  // index
	byte flag;              // buddy block state flag
	
	k_lockSpin(&(g_dynamicMemManager.spinlock));
	
	// combine blocks going up from free block list to the highest block list.
	for (i = blockListIndex; i < g_dynamicMemManager.maxLevelCount; i++) {
		
		// set current block to EXIST.
		k_setFlagInBitmap(i, blockOffset, DMEM_EXIST);
		
		// If block offset is even (left), check odd (right),
		// and if block offset is odd (right), check even (left),
		// and combine blocks if buddy block exists after check.
		if ((blockOffset % 2) == 0) {
			buddyBlockOffset = blockOffset + 1;
			
		} else {
			buddyBlockOffset = blockOffset - 1;
		}
		
		// get state flag of buddy block.
		flag = k_getFlagInBitmap(i, buddyBlockOffset);
		
		// break if buddy block is EMPTY.
		if (flag == DMEM_EMPTY) {
			break;
		}
		
		// combine blocks if buddy block is EXIST. (set both current block and buddy block to EMPTY, and go up to high block list.)
		k_setFlagInBitmap(i, buddyBlockOffset, DMEM_EMPTY);
		k_setFlagInBitmap(i, blockOffset, DMEM_EMPTY);
		
		// change block offset of high block list, do the process above again starting from high block list.
		blockOffset = blockOffset / 2;
	}
	
	k_unlockSpin(&(g_dynamicMemManager.spinlock));
	return true;
}

static byte k_getFlagInBitmap(int blockListIndex, int offset) {
	byte* bitmap;
	
	bitmap = g_dynamicMemManager.bitmapOfLevel[blockListIndex].bitmap;
	
	// If offset bit in bitmap == 1, return EXIST.
	if ((bitmap[offset/8] & (0x01 << (offset % 8))) != 0) {
		return DMEM_EXIST;
	}
	
	// If offset bit in bitmap == 0, return EMPTY.
	return DMEM_EMPTY;
}

void k_getDynamicMemInfo(qword* startAddr, qword* totalSize, qword* metaSize, qword* usedSize) {
	if (startAddr != null) {
		*startAddr = DMEM_START_ADDRESS;
	}
	
	if (totalSize != null) {
		*totalSize = k_calcDynamicMemSize();
	}
	
	if (metaSize != null) {
		if (totalSize != null) {
			*metaSize = k_calcMetaBlockCount(*totalSize) * DMEM_MIN_SIZE;

		} else {
			*metaSize = k_calcMetaBlockCount(k_calcDynamicMemSize()) * DMEM_MIN_SIZE;
		}
	}
		
	if (usedSize != null) {
		*usedSize = g_dynamicMemManager.usedSize;
	}
}

DynamicMemManager* k_getDynamicMemManager(void) {
	return &g_dynamicMemManager;
}

