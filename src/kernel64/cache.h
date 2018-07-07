#ifndef __CACHE_H__
#define __CACHE_H__

#include "types.h"

#define CACHE_MAXCLUSTERLINKTABLEAREACOUNT 16         // max cache buffer count of cluster link table area
#define CACHE_MAXDATAAREACOUNT             32         // max cache buffer count of data area
#define CACHE_INVALIDTAG                   0xFFFFFFFF // invaild tag (free cache buffer)

// macros related with cache table
#define CACHE_MAXCACHETABLEINDEX   2 // max index count of cache table (max entry count)
#define CACHE_CLUSTERLINKTABLEAREA 0 // cluster link table area index of cache table (There are 16 cache buffers in No.0 entry of cache table.)
#define CACHE_DATAAREA             1 // data area index of cache table (There are 32 cache buffers in No.1 entry of cache table.)

#pragma pack(push, 1)

typedef struct k_CacheBuffer {
	dword tag;        // tag: the offset of cluster link table area (512B sector unit) or data area (4KB cluster unit) corresponding to cache buffer.
	dword accessTime; // access time: the time which accesses to cache buffer.
	bool changed;     // changed flag: It indicates whether data changed or not.
	byte* buffer;     // data buffer: the address of cache buffer.
} CacheBuffer;

typedef struct k_CacheManager {
	dword accessTime[CACHE_MAXCACHETABLEINDEX];                                 // access time: the max value + 1 of access time by cache buffer.
	byte* buffer[CACHE_MAXCACHETABLEINDEX];                                     // data buffer
	CacheBuffer cacheBuffer[CACHE_MAXCACHETABLEINDEX][CACHE_MAXDATAAREACOUNT]; // cache buffer: choose 32 as the column length, because 16 < 32.
	dword maxCount[CACHE_MAXCACHETABLEINDEX];                                   // max cache buffer count
} CacheManager;

#pragma pack(pop)

bool k_initCacheManager(void);
CacheBuffer* k_allocCacheBuffer(int cacheTableIndex); // search free cache buffer.
CacheBuffer* k_findCacheBuffer(int cacheTableIndex, dword tag); // search cache buffer identified by tag.
CacheBuffer* k_getVictimInCacheBuffer(int cacheTableIndex); // search free cache buffer or old cache buffer.
void k_discardAllCacheBuffer(int cacheTableIndex);
bool k_getCacheBufferAndCount(int cacheTableIndex, CacheBuffer** cacheBuffer, int* maxCount);
static void k_cutdownAccessTime(int cacheTableIndex);

#endif // __CACHE_H__
