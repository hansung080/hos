#include "rdd.h"
#include "util.h"
#include "dynamic_mem.h"

static RddManager g_rddManager;

bool k_initRdd(dword totalSectorCount) {
	
	k_memset(&g_rddManager, 0, sizeof(g_rddManager));
	
	// allocate memory for RAM disk (8 MB).
	g_rddManager.buffer = (byte*)k_allocMem(totalSectorCount * 512);
	if (g_rddManager.buffer == null) {
		return false;
	}
	
	// initialize total sector count, mutex.
	g_rddManager.totalSectorCount = totalSectorCount;
	k_initMutex(&(g_rddManager.mutex));
	
	return true;
}

bool k_readRddInfo(bool primary, bool master, HddInfo* hddInfo) {
	
	k_memset(hddInfo, 0, sizeof(HddInfo));
	
	// set total sector count, model number, serial number.
	hddInfo->totalSectors = g_rddManager.totalSectorCount;
	k_memcpy(hddInfo->modelNumber, "hans-ram-disk-v1.0", 18);
	k_memcpy(hddInfo->serialNumber, "0000-0000", 9);
	
	return true;
}

int k_readRddSector(bool primary, bool master, dword lba, int sectorCount, char* buffer) {
	int realReadCount; // real read sector count
	
	// read read sector count = MIN(remained sector count of RAM disk, requested sector count)
	// [Note] modified by hs.kwon
	//realReadCount = MIN(g_rddManager->totalSectorCount - (dwLBA + sectorCount), sectorCount);
	realReadCount = MIN(g_rddManager.totalSectorCount - lba, sectorCount);
	
	// read sector: copy data from RAM disk to buffer.
	k_memcpy(buffer, g_rddManager.buffer + (lba * 512), realReadCount * 512);
	
	// return real read sector count.
	return realReadCount;
}

int k_writeRddSector(bool primary, bool master, dword lba, int sectorCount, char* buffer) {
	int realWriteCount; // real written sector count
	
	// real written sector count = MIN(remained sector count of RAM disk, requested sector count)
	// [Note] modified by hs.kwon
	//realWriteCount = MIN(g_rddManager.totalSectorCount - (dwLBA + sectorCount), sectorCount);
	realWriteCount = MIN(g_rddManager.totalSectorCount - lba, sectorCount);
	
	// write sector: copy data from buffer to RAM disk.
	k_memcpy(g_rddManager.buffer + (lba * 512), buffer, realWriteCount * 512);
	
	// return real written sector count.
	return realWriteCount;
}
