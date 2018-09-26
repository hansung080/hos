#ifndef __CORE_RDD_H__
#define __CORE_RDD_H__

#include "types.h"
#include "sync.h"
#include "hdd.h"

#define RDD_TOTALSECTORCOUNT (8 * 1024 * 1024 / 512) // total sector count of RAM disk (8 MB)

#pragma pack(push, 1)

typedef struct k_RddManager {
	byte* buffer;           // memory buffer for RAM disk
	dword totalSectorCount; // total sector count
	Mutex mutex;            // mutex
} RddManager;

#pragma pack(pop)

bool k_initRdd(dword totalSectorCount);
bool k_readRddInfo(bool primary, bool master, HddInfo* hddInfo);
int k_readRddSector(bool primary, bool master, dword lba, int sectorCount, char* buffer);
int k_writeRddSector(bool primary, bool master, dword lba, int sectorCount, char* buffer);

#endif // __CORE_RDD_H__