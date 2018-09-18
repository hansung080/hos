#include "cache.h"
#include "util.h"
#include "dynamic_mem.h"
#include "file_system.h"
#include "console.h"

static CacheManager g_cacheManager;

bool k_initCacheManager(void) {
	int i;
	
	k_memset(&g_cacheManager, 0, sizeof(g_cacheManager));
	
	// initialize access time.
	g_cacheManager.accessTime[CACHE_CLUSTERLINKTABLEAREA] = 0;
	g_cacheManager.accessTime[CACHE_DATAAREA] = 0;
	
	// initialize max cache buffer count.
	g_cacheManager.maxCount[CACHE_CLUSTERLINKTABLEAREA] = CACHE_MAXCLUSTERLINKTABLEAREACOUNT;
	g_cacheManager.maxCount[CACHE_DATAAREA] = CACHE_MAXDATAAREACOUNT;
	
	// allocate data buffer (the cache buffer count of cluster link table area is 16, the size is 512B).
	g_cacheManager.buffer[CACHE_CLUSTERLINKTABLEAREA] = (byte*)k_allocMem(CACHE_MAXCLUSTERLINKTABLEAREACOUNT * 512);
	if (g_cacheManager.buffer[CACHE_CLUSTERLINKTABLEAREA] == null) {
		return false;
	}
	
	// initialize cache buffer (divide data buffer, and use it as cache buffer.)
	for (i = 0; i < CACHE_MAXCLUSTERLINKTABLEAREACOUNT; i++) {
		g_cacheManager.cacheBuffer[CACHE_CLUSTERLINKTABLEAREA][i].buffer = g_cacheManager.buffer[CACHE_CLUSTERLINKTABLEAREA] + (i * 512);
		g_cacheManager.cacheBuffer[CACHE_CLUSTERLINKTABLEAREA][i].tag = CACHE_INVALIDTAG;
	}
	
	// allocate data buffer (the cache buffer count of data area is 32, the size is 4KB.)
	g_cacheManager.buffer[CACHE_DATAAREA] = (byte*)k_allocMem(CACHE_MAXDATAAREACOUNT * FS_CLUSTERSIZE);
	if (g_cacheManager.buffer[CACHE_DATAAREA] == null) {
		k_freeMem(g_cacheManager.buffer[CACHE_CLUSTERLINKTABLEAREA]);
		return false;
	}
	
	// initialize cache buffer (divide data buffer, and use it as cache buffer.)
	for (i = 0; i < CACHE_MAXDATAAREACOUNT; i++) {
		g_cacheManager.cacheBuffer[CACHE_DATAAREA][i].buffer = g_cacheManager.buffer[CACHE_DATAAREA] + (i * FS_CLUSTERSIZE);
		g_cacheManager.cacheBuffer[CACHE_DATAAREA][i].tag = CACHE_INVALIDTAG;
	}
	
	return true;
}

CacheBuffer* k_allocCacheBuffer(int cacheTableIndex) {
	CacheBuffer* cacheBuffer;
	int i;
	
	if (cacheTableIndex >= CACHE_MAXCACHETABLEINDEX) {
		return null;
	}
	
	// decrease whole access time when access time increases up to the max value.
	k_cutdownAccessTime(cacheTableIndex);
	
	// search free cache buffer, and return it.
	cacheBuffer = g_cacheManager.cacheBuffer[cacheTableIndex];
	for (i = 0; i < g_cacheManager.maxCount[cacheTableIndex]; i++) {
		if (cacheBuffer[i].tag == CACHE_INVALIDTAG) {
			// set allocated cache buffer using temporary tag.
			cacheBuffer[i].tag = CACHE_INVALIDTAG - 1;
			
			// update access time.
			cacheBuffer[i].accessTime = g_cacheManager.accessTime[cacheTableIndex]++;
			
			// return the address of free cache buffer.
			return &(cacheBuffer[i]);
		}
	}
	
	return null;
}

CacheBuffer* k_findCacheBuffer(int cacheTableIndex, dword tag) {
	CacheBuffer* cacheBuffer;
	int i;
	
	if (cacheTableIndex >= CACHE_MAXCACHETABLEINDEX) {
		return null;
	}
	
	// decrease whole access time when access time increases up to the max value.
	k_cutdownAccessTime(cacheTableIndex);
	
	// search cache buffer identified by tag, return it.
	cacheBuffer = g_cacheManager.cacheBuffer[cacheTableIndex];
	for (i = 0; i < g_cacheManager.maxCount[cacheTableIndex]; i++) {
		if (cacheBuffer[i].tag == tag) {
			// update access time.
			cacheBuffer[i].accessTime = g_cacheManager.accessTime[cacheTableIndex]++;
			
			// return the address of cache buffer identified by tag.
			return &(cacheBuffer[i]);
		}
	}
	
	return null;
}

static void k_cutdownAccessTime(int cacheTableIndex) {
	CacheBuffer tempBuffer;
	CacheBuffer* cacheBuffer;
	bool sorted;
	int i, j;
	
	if (cacheTableIndex >= CACHE_MAXCACHETABLEINDEX) {
		return;
	}
	
	// return if access time has not increased up to the max value yet.
	if (g_cacheManager.accessTime[cacheTableIndex] < 0xFFFFFFFE) {
		return;
	}
	
	// sort cache buffer ascendingly by access time using Bubble Sort Algorithm.
	cacheBuffer = g_cacheManager.cacheBuffer[cacheTableIndex];
	for (i = 0; i < (g_cacheManager.maxCount[cacheTableIndex] - 1); i++) {
		sorted = true;
		
		for (j = 0; j < (g_cacheManager.maxCount[cacheTableIndex] - 1 - i); j++) {
			if (cacheBuffer[j].accessTime > cacheBuffer[j+1].accessTime) {
				sorted = false;
				
				k_memcpy(&tempBuffer, &(cacheBuffer[j]), sizeof(CacheBuffer));
				k_memcpy(&(cacheBuffer[j]), &(cacheBuffer[j+1]), sizeof(CacheBuffer));
				k_memcpy(&(cacheBuffer[j+1]), &tempBuffer, sizeof(CacheBuffer));
			}
		}
		
		if (sorted == true) {
			break;
		}
	}
	
	// decrease whole access time. (set access time to be started from 0)
	for (i = 0; i < g_cacheManager.maxCount[cacheTableIndex]; i++) {
		cacheBuffer[i].accessTime = i;
	}
	
	// update access time of cache manager.
	g_cacheManager.accessTime[cacheTableIndex] = i;
}

CacheBuffer* k_getVictimInCacheBuffer(int cacheTableIndex) {
	dword oldTime;
	CacheBuffer* cacheBuffer;
	int oldIndex;
	int i;
	
	if (cacheTableIndex >= CACHE_MAXCACHETABLEINDEX) {
		return null;
	}
	
	oldIndex = -1;
	oldTime = 0xFFFFFFFF;
	
	// search free cache buffer or old cache buffer.
	cacheBuffer = g_cacheManager.cacheBuffer[cacheTableIndex];
	for (i = 0; i < g_cacheManager.maxCount[cacheTableIndex]; i++) {
		
		// search free cache buffer.
		if (cacheBuffer[i].tag == CACHE_INVALIDTAG) {
			oldIndex = i;
			break;
		}
		
		// search old cache buffer.
		if (cacheBuffer[i].accessTime < oldTime) {
			oldTime = cacheBuffer[i].accessTime;
			oldIndex = i;
		}
	}
	
	// handle error when searching fails.
	if (oldIndex == -1) {
		k_printf("cache error: can not get victim in cache buffer\n");
		return null;
	}
	
	// update access time of the searched cache buffer.
	cacheBuffer[oldIndex].accessTime = g_cacheManager.accessTime[cacheTableIndex]++;
	
	// return the address of the searched cache buffer.
	return &(cacheBuffer[oldIndex]);
}

void k_discardAllCacheBuffer(int cacheTableIndex) {
	CacheBuffer* cacheBuffer;
	int i;
	
	if (cacheTableIndex >= CACHE_MAXCACHETABLEINDEX) {
		return;
	}
	
	// set all cache buffers to be free.
	cacheBuffer = g_cacheManager.cacheBuffer[cacheTableIndex];
	for (i = 0; i < g_cacheManager.maxCount[cacheTableIndex]; i++) {
		cacheBuffer[i].tag = CACHE_INVALIDTAG;
	}
	
	// initialize access time.
	g_cacheManager.accessTime[cacheTableIndex] = 0;
}

bool k_getCacheBufferAndCount(int cacheTableIndex, CacheBuffer** cacheBuffer, int* maxCount) {
	if (cacheTableIndex >= CACHE_MAXCACHETABLEINDEX) {
		return false;
	}
	
	// get the address and the max count of cache buffer.
	*cacheBuffer = g_cacheManager.cacheBuffer[cacheTableIndex];
	*maxCount = g_cacheManager.maxCount[cacheTableIndex];
	
	return true;
}
