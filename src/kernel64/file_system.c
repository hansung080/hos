#include "file_system.h"
#include "dynamic_mem.h"
#include "util.h"
#include "rdd.h"
#include "console.h"

static FileSystemManager g_fileSystemManager;
static byte g_tempBuffer[FS_CLUSTERSIZE];

static ReadHddInfo g_readHddInfo = null;
static ReadHddSector g_readHddSector = null;
static WriteHddSector g_writeHddSector = null;

bool k_initFileSystem(void) {
	bool cacheEnabled = false;

	// initialize file system manager.
	k_memset(&g_fileSystemManager, 0, sizeof(g_fileSystemManager));

	// initialize mutex.
	k_initMutex(&(g_fileSystemManager.mutex));

	// initialize hard disk.
	if (k_initHdd() == true) {
		// set function pointer related with hard disk control.
		g_readHddInfo = k_readHddInfo;
		g_readHddSector = k_readHddSector;
		g_writeHddSector = k_writeHddSector;

		// set true to cache enable flag.
		cacheEnabled = true;

	// initialize ram disk if initialization of hard disk fails.
	} else if (k_initRdd(RDD_TOTALSECTORCOUNT) == true) {
		// set function pointer related with ram disk control.
		g_readHddInfo = k_readRddInfo;
		g_readHddSector = k_readRddSector;
		g_writeHddSector = k_writeRddSector;

		// create file system every when booting, because ram disk is volatile.
		if (k_format() == false) {
			return false;
		}

	} else {
		return false;
	}

	// connect file system
	if (k_mount() == false) {
		return false;
	}

	// allocate memory of handle pool.
	g_fileSystemManager.handlePool = (File*)k_allocMem(sizeof(File) * FS_HANDLE_MAXCOUNT);
	if (g_fileSystemManager.handlePool == null) {
		g_fileSystemManager.mounted = false;
		return false;
	}

	// initialize handle pool.
	k_memset(g_fileSystemManager.handlePool, 0, sizeof(File) * FS_HANDLE_MAXCOUNT);

	// If cache enable flag == true, initialize cache.
	if (cacheEnabled == true) {
		g_fileSystemManager.cacheEnabled = k_initCacheManager();
	}

	return true;
}

bool k_mount(void) {
	Mbr* mbr;

	k_lock(&(g_fileSystemManager.mutex));

	// read MBR area(LBA 0, 1 sector-sized)
	if (g_readHddSector(true, true, 0, 1, g_tempBuffer) == false) {
		k_unlock(&(g_fileSystemManager.mutex));
		return false;
	}

	// check file system signature.
	mbr = (Mbr*)g_tempBuffer;
	if (mbr->signature != FS_SIGNATURE) {
		k_unlock(&(g_fileSystemManager.mutex));
		return false;
	}

	// recognize file system successfully.
	g_fileSystemManager.mounted = true;

	// set start LBA address and sector count of each areas.
	g_fileSystemManager.reservedSectorCount = mbr->reservedSectorCount;
	g_fileSystemManager.clusterLinkAreaStartAddr = 1 + mbr->reservedSectorCount;
	g_fileSystemManager.clusterLinkAreaSize = mbr->clusterLinkSectorCount;
	g_fileSystemManager.dataAreaStartAddr = 1 + mbr->reservedSectorCount + mbr->clusterLinkSectorCount;
	g_fileSystemManager.totalClusterCount = mbr->totalClusterCount;

	k_unlock(&(g_fileSystemManager.mutex));
	return true;
}

bool k_format(void) {
	HddInfo* hddInfo;
	Mbr* mbr;
	dword totalSectorCount;       // total sector count of hard disk
	dword remainSectorCount;      // sector count of general data area
	dword maxClusterCount;        // max cluster count (total cluster count of hard disk)
	dword clusterCount;           // real cluster count (cluster count of general data area)
	dword clusterLinkSectorCount; // sector count of cluster link table area
	dword i;

	k_lock(&(g_fileSystemManager.mutex));

	//----------------------------------------------------------------------------------------------------
	// get real cluster count (cluster count of general data area), sector count of cluster link table area.
	//----------------------------------------------------------------------------------------------------

	// read hard disk info, and get total sector count of hard disk.
	hddInfo = (HddInfo*)g_tempBuffer;
	if (g_readHddInfo(true, true, hddInfo) == false) {
		k_unlock(&(g_fileSystemManager.mutex));
		return false;
	}
	totalSectorCount = hddInfo->totalSectors;

	// total cluster count = total sector count of hard disk / sector count per a cluster (8)
	maxClusterCount = totalSectorCount / FS_SECTORSPERCLUSTER;

	// Fist of all, calculate sector count of cluster link table area with max cluster count.
	// aligned with 128 (sector unit, rounding up), because 128 cluster links (4B) can be created in a sector (512B).
	clusterLinkSectorCount = (maxClusterCount + 127) / 128;

	// sector count of general data area = total sector count of hard disk - sector count of MBR area (1) - sector count of reserved area (0) - sector count of cluster link table area
	// real cluster count = sector count of general data area / sector count per a cluster (8)
	remainSectorCount = totalSectorCount - 1 - clusterLinkSectorCount;
	clusterCount = remainSectorCount / FS_SECTORSPERCLUSTER;

	// Finally, calculate sector count of cluster link table area with real cluster count again.
	clusterLinkSectorCount = (clusterCount + 127) / 128;

	//----------------------------------------------------------------------------------------------------
	// initialize MBR area
	//----------------------------------------------------------------------------------------------------

	// read MBR area (LBA 0, 1 sector-sized)
	if (g_readHddSector(true, true, 0, 1, g_tempBuffer) == false) {
		k_unlock(&(g_fileSystemManager.mutex));
		return false;
	}

	// initialize partition table (0), file system info (calculated from the code above).
	mbr = (Mbr*)g_tempBuffer;
	k_memset(mbr->partition, 0, sizeof(mbr->partition));
	mbr->signature = FS_SIGNATURE;
	mbr->reservedSectorCount = 0;
	mbr->clusterLinkSectorCount = clusterLinkSectorCount;
	mbr->totalClusterCount = clusterCount;

	// write MBR area(LBA 0, 1 sector-sized)
	if (g_writeHddSector(true, true, 0, 1, g_tempBuffer) == false) {
		k_unlock(&(g_fileSystemManager.mutex));
		return false;
	}

	//----------------------------------------------------------------------------------------------------
	// initialize cluster link table area, root directory.
	//----------------------------------------------------------------------------------------------------

	// initialize cluster link table area, root directory as 0.
	k_memset(g_tempBuffer, 0, 512);
	for (i = 0; i < (clusterLinkSectorCount + FS_SECTORSPERCLUSTER); i++) {
		// But, set the first cluster link which means root directory (cluster 0) to allocated cluster (last cluster).
		if (i == 0) {
			((dword*)(g_tempBuffer))[0] = FS_LASTCLUSTER;

		} else {
			((dword*)(g_tempBuffer))[0] = FS_FREECLUSTER;
		}

		// write by 1 sector from LBA 1.
		if (g_writeHddSector(true, true, i + 1, 1, g_tempBuffer) == false) {
			k_unlock(&(g_fileSystemManager.mutex));
			return false;
		}
	}

	//----------------------------------------------------------------------------------------------------
	// flush all cache buffers.
	//----------------------------------------------------------------------------------------------------
	if (g_fileSystemManager.cacheEnabled == true) {
		k_discardAllCacheBuffer(CACHE_CLUSTERLINKTABLEAREA);
		k_discardAllCacheBuffer(CACHE_DATAAREA);
	}

	k_unlock(&(g_fileSystemManager.mutex));
	return true;
}

bool k_getHddInfo(HddInfo* info) {
	bool result;

	k_lock(&(g_fileSystemManager.mutex));

	result = g_readHddInfo(true, true, info);

	k_unlock(&(g_fileSystemManager.mutex));

	return result;
}

static bool k_readClusterLinkTable(dword offset, byte* buffer) {
	if (g_fileSystemManager.cacheEnabled == false) {
		k_readClusterLinkTableWithoutCache(offset, buffer);

	} else {
		k_readClusterLinkTableWithCache(offset, buffer);
	}
}

static bool k_readClusterLinkTableWithoutCache(dword offset, byte* buffer) {
	return g_readHddSector(true, true, g_fileSystemManager.clusterLinkAreaStartAddr + offset, 1, buffer);
}

static bool k_readClusterLinkTableWithCache(dword offset, byte* buffer) {
	CacheBuffer* cacheBuffer;

	// search cache buffer matching offset
	cacheBuffer = k_findCacheBuffer(CACHE_CLUSTERLINKTABLEAREA, offset);

	// If cache buffer exists, read it.
	if (cacheBuffer != null) {
		k_memcpy(buffer, cacheBuffer->buffer, 512);
		return true;
	}

	// If cache buffer dosen't exist, read from hard disk.
	if (k_readClusterLinkTableWithoutCache(offset, buffer) == false) {
		return false;
	}

	// allocate cache buffer.
	cacheBuffer = k_allocCacheBufferWithFlush(CACHE_CLUSTERLINKTABLEAREA);
	if (cacheBuffer == null) {
		return false;
	}

	// write data from hard dist to the allocated cache buffer.
	k_memcpy(cacheBuffer->buffer, buffer, 512);
	cacheBuffer->tag = offset;
	cacheBuffer->changed = false;

	return true;
}

static CacheBuffer* k_allocCacheBufferWithFlush(int cacheTableIndex) {
	CacheBuffer* cacheBuffer;

	// search free cache buffer.
	cacheBuffer = k_allocCacheBuffer(cacheTableIndex);

	// If free cache buffer dosen't exist, flush old cache buffer, and allocate it.
	if (cacheBuffer == null) {

		// search old cache buffer.
		cacheBuffer = k_getVictimInCacheBuffer(cacheTableIndex);

		// If old cache buffer dosen't exist, do error handling.
		if (cacheBuffer == null) {
			k_printf("cache error: cache buffer allocation error\n");
			return null;
		}

		// If cache buffer changes, apply it to hard disk.
		if (cacheBuffer->changed == true) {
			switch(cacheTableIndex){
			case CACHE_CLUSTERLINKTABLEAREA:
				if (k_writeClusterLinkTableWithoutCache(cacheBuffer->tag, cacheBuffer->buffer) == false) {
					k_printf("cache error: cache buffer writing error (cluster link table area)\n");
					return null;
				}

				break;

			case CACHE_DATAAREA:
				if (k_writeClusterWithoutCache(cacheBuffer->tag, cacheBuffer->buffer) == false) {
					k_printf("cache error: cache buffer writing error (data area)\n");
					return null;
				}

				break;

			default:
				k_printf("cache error: cache table index error");
				return null;
				break;
			}
		}
	}

	return cacheBuffer;
}

static bool k_writeClusterLinkTable(dword offset, byte* buffer) {
	if (g_fileSystemManager.cacheEnabled == false) {
		k_writeClusterLinkTableWithoutCache(offset, buffer);

	} else {
		k_writeClusterLinkTableWithCache(offset, buffer);
	}
}

static bool k_writeClusterLinkTableWithoutCache(dword offset, byte* buffer) {
	return g_writeHddSector(true, true, g_fileSystemManager.clusterLinkAreaStartAddr + offset, 1, buffer);
}

static bool k_writeClusterLinkTableWithCache(dword offset, byte* buffer) {
	CacheBuffer* cacheBuffer;

	// search cache buffer matching offset.
	cacheBuffer = k_findCacheBuffer(CACHE_CLUSTERLINKTABLEAREA, offset);

	// If cache buffer exists, write to it.
	if (cacheBuffer != null) {
		k_memcpy(cacheBuffer->buffer, buffer, 512);
		cacheBuffer->changed = true;
		return true;
	}

	// If cache buffer dosen't exist, allocate cache buffer.
	cacheBuffer = k_allocCacheBufferWithFlush(CACHE_CLUSTERLINKTABLEAREA);
	if (cacheBuffer == null) {
		return false;
	}

	// write to the allocated cache buffer.
	k_memcpy(cacheBuffer->buffer, buffer, 512);
	cacheBuffer->tag = offset;
	cacheBuffer->changed = true;

	return true;
}

static bool k_readCluster(dword offset, byte* buffer) {
	if (g_fileSystemManager.cacheEnabled == false) {
		k_readClusterWithoutCache(offset, buffer);

	} else {
		k_readClusterWithCache(offset, buffer);
	}
}

static bool k_readClusterWithoutCache(dword offset, byte* buffer) {
	return g_readHddSector(true, true, g_fileSystemManager.dataAreaStartAddr + (offset * FS_SECTORSPERCLUSTER), FS_SECTORSPERCLUSTER, buffer);
}

static bool k_readClusterWithCache(dword offset, byte* buffer) {
	CacheBuffer* cacheBuffer;

	// search cache buffer matching offset.
	cacheBuffer = k_findCacheBuffer(CACHE_DATAAREA, offset);

	// If cache buffer exists, read from it.
	if (cacheBuffer != null) {
		k_memcpy(buffer, cacheBuffer->buffer, FS_CLUSTERSIZE);
		return true;
	}

	// If cache buffer dosen't exist, read from hard disk.
	if (k_readClusterWithoutCache(offset, buffer) == false) {
		return false;
	}

	// allocate cache buffer.
	cacheBuffer = k_allocCacheBufferWithFlush(CACHE_DATAAREA);
	if (cacheBuffer == null) {
		return false;
	}

	// write data from hard disk to the allocated cache buffer.
	k_memcpy(cacheBuffer->buffer, buffer, FS_CLUSTERSIZE);
	cacheBuffer->tag = offset;
	cacheBuffer->changed = false;

	return true;
}

static bool k_writeCluster(dword offset, byte* buffer) {
	if (g_fileSystemManager.cacheEnabled == false) {
		k_writeClusterWithoutCache(offset, buffer);

	} else {
		k_writeClusterWithCache(offset, buffer);
	}
}

static bool k_writeClusterWithoutCache(dword offset, byte* buffer) {
	return g_writeHddSector(true, true, g_fileSystemManager.dataAreaStartAddr + (offset * FS_SECTORSPERCLUSTER), FS_SECTORSPERCLUSTER, buffer);
}

static bool k_writeClusterWithCache(dword offset, byte* buffer) {
	CacheBuffer* cacheBuffer;

	// search cache buffer matching offset.
	cacheBuffer = k_findCacheBuffer(CACHE_DATAAREA, offset);

	// If cache buffer exists, write to it.
	if (cacheBuffer != null) {
		k_memcpy(cacheBuffer->buffer, buffer, FS_CLUSTERSIZE);
		cacheBuffer->changed = true;
		return true;
	}

	// If cache buffer dosen't exits, allocate cache buffer.
	cacheBuffer = k_allocCacheBufferWithFlush(CACHE_DATAAREA);
	if (cacheBuffer == null) {
		return false;
	}

	// write to the allocated cache buffer.
	k_memcpy(cacheBuffer->buffer, buffer, FS_CLUSTERSIZE);
	cacheBuffer->tag = offset;
	cacheBuffer->changed = true;

	return true;
}

static dword k_findFreeCluster(void) {
	dword linkCountInSector;
	dword lastSectorOffset, currentSectorOffset;
	dword i, j;

	if (g_fileSystemManager.mounted == false) {
		return FS_LASTCLUSTER;
	}

	lastSectorOffset = g_fileSystemManager.lastAllocedClusterLinkSectorOffset;

	// search free cluster looping from the last allocated position.
	for (i = 0; i < g_fileSystemManager.clusterLinkAreaSize; i++) {

		// If it's the last sector of cluster link table, 128 cluster links might not exist.
		if ((lastSectorOffset + i) == (g_fileSystemManager.clusterLinkAreaSize - 1)) {
			linkCountInSector = g_fileSystemManager.totalClusterCount % 128;

		} else {
			linkCountInSector = 128;
		}

		// read current sector.
		currentSectorOffset = (lastSectorOffset + i) % g_fileSystemManager.clusterLinkAreaSize;
		if (k_readClusterLinkTable(currentSectorOffset, g_tempBuffer) == false) {
			return FS_LASTCLUSTER;
		}

		// search free cluster inside current sector.
		for (j = 0; j < linkCountInSector; j++) {
			if (((dword*)g_tempBuffer)[j] == FS_FREECLUSTER) {
				break;
			}
		}

		// If free cluster is searched successfully, return cluster index.
		if (j != linkCountInSector) {
			g_fileSystemManager.lastAllocedClusterLinkSectorOffset = currentSectorOffset;

			return (currentSectorOffset * 128) + j; // return free cluster index.
		}
	}

	return FS_LASTCLUSTER;
}

static bool k_setClusterLinkData(dword clusterIndex, dword data) {
	dword sectorOffset;

	if (g_fileSystemManager.mounted == false) {
		return false;
	}

	sectorOffset = clusterIndex / 128;

	// [Note] it's added by hs.kwon
	if ((sectorOffset < 0) || (sectorOffset >= g_fileSystemManager.clusterLinkAreaSize)) {
		return false;
	}

	// read the sector.
	if (k_readClusterLinkTable(sectorOffset, g_tempBuffer) == false) {
		return false;
	}

	// set data to the cluster link.
	((dword*)g_tempBuffer)[clusterIndex % 128] = data;

	// write the sector.
	if (k_writeClusterLinkTable(sectorOffset, g_tempBuffer) == false) {
		return false;
	}

	return true;
}

static bool k_getClusterLinkData(dword clusterIndex, dword* data) {
	dword sectorOffset;

	if (g_fileSystemManager.mounted == false) {
		return false;
	}

	sectorOffset = clusterIndex / 128;

	if ((sectorOffset < 0) || (sectorOffset >= g_fileSystemManager.clusterLinkAreaSize)) {
		return false;
	}

	// read the sector.
	if (k_readClusterLinkTable(sectorOffset, g_tempBuffer) == false) {
		return false;
	}

	// get data from the cluster link.
	*data = ((dword*)g_tempBuffer)[clusterIndex % 128];
	return true;
}

static int k_findFreeDirEntry(void) {
	DirEntry* entry;
	int i;

	if (g_fileSystemManager.mounted == false) {
		return -1;
	}

	// read root directory (cluster 0, 1 cluster-sized)
	if (k_readCluster(0, g_tempBuffer) == false) {
		return -1;
	}

	// search free directory entry (start cluster index=0x00) in root directory.
	entry = (DirEntry*)g_tempBuffer;
	for (i = 0; i < FS_MAXDIRECTORYENTRYCOUNT; i++) {
		if (entry[i].startClusterIndex == 0x00) {
			return i; // return free directory entry index.
		}
	}

	return -1;
}

static bool k_setDirEntryData(int index, DirEntry* entry) {
	DirEntry* rootEntry;

	if ((g_fileSystemManager.mounted == false) || (index < 0) || (index >= FS_MAXDIRECTORYENTRYCOUNT)) {
		return false;
	}

	// read root directory (cluster 0, 1 cluster-sized)
	if (k_readCluster(0, g_tempBuffer) == false) {
		return false;
	}

	// set to the directory entry of root directory.
	rootEntry = (DirEntry*)g_tempBuffer;
	k_memcpy(rootEntry + index, entry, sizeof(DirEntry));

	// write root directory (cluster 0, 1 cluster-sized)
	if (k_writeCluster(0, g_tempBuffer) == false) {
		return false;
	}

	return true;
}

static bool k_getDirEntryData(int index, DirEntry* entry) {
	DirEntry* rootEntry;

	if ((g_fileSystemManager.mounted == false) || (index < 0) || (index >= FS_MAXDIRECTORYENTRYCOUNT)) {
		return false;
	}

	// read root directory (cluster 0, 1 cluster-sized)
	if (k_readCluster(0, g_tempBuffer) == false) {
		return false;
	}

	// get the directory entry of root directory.
	rootEntry = (DirEntry*)g_tempBuffer;
	k_memcpy(entry, rootEntry + index, sizeof(DirEntry));

	return true;
}

static int k_findDirEntry(const char* fileName, DirEntry* entry) {
	DirEntry* rootEntry;
	int i;
	int len;

	if (g_fileSystemManager.mounted == false) {
		return -1;
	}

	// read root directory (cluster 0, 1 cluster-sized)
	if (k_readCluster(0, g_tempBuffer) == false) {
		return -1;
	}

	len = k_strlen(fileName);

	// search directory entry matching file name in root directory.
	rootEntry = (DirEntry*)g_tempBuffer;
	for (i = 0; i < FS_MAXDIRECTORYENTRYCOUNT; i++) {
		if (k_memcmp(rootEntry[i].fileName, fileName, len) == 0) {
			k_memcpy(entry, rootEntry + i, sizeof(DirEntry));
			return i; // return directory entry index matching file name.
		}
	}

	return -1;
}

void k_getFileSystemInfo(FileSystemManager* manager) {
	k_memcpy(manager, &g_fileSystemManager, sizeof(g_fileSystemManager));
}

static void* k_allocFileDirHandle(void) {
	int i;
	File* file;

	// search handle pool, and return free handle.
	file = g_fileSystemManager.handlePool;
	for (i = 0; i < FS_HANDLE_MAXCOUNT; i++) {

		// If it's free handle.
		if (file->type == FS_TYPE_FREE) {

			// set it to file handle.
			file->type = FS_TYPE_FILE;
			return file;
		}

		file++;
	}

	return null;
}

static void k_freeFileDirHandle(File* file) {
	// initialize file handle.
	k_memset(file, 0, sizeof(File));

	// set it to free handle.
	file->type = FS_TYPE_FREE;
}

static bool k_createFile(const char* fileName, DirEntry* entry, int* dirEntryIndex) {
	dword cluster; // free cluster index

	// search free cluster, and set it to allocated cluster (last cluster)
	cluster = k_findFreeCluster();
	if ((cluster == FS_LASTCLUSTER) || (k_setClusterLinkData(cluster, FS_LASTCLUSTER) == false)) {
		return false;
	}

	// search free directory entry.
	*dirEntryIndex = k_findFreeDirEntry();
	if (*dirEntryIndex == -1) {
		// If fails, free the allocated cluster.
		k_setClusterLinkData(cluster, FS_FREECLUSTER);
		return false;
	}

	// set directory entry.
	k_memcpy(entry->fileName, fileName, k_strlen(fileName) + 1);
	entry->fileSize = 0;
	entry->startClusterIndex = cluster;

	// register directory entry.
	if (k_setDirEntryData(*dirEntryIndex, entry) == false) {
		// If fails, free the allocated cluster.
		k_setClusterLinkData(cluster, FS_FREECLUSTER);
		return false;
	}

	return true;
}

static bool k_freeClusterUntilEnd(dword clusterIndex) {
	dword currentClusterIndex;
	dword nextClusterIndex;

	currentClusterIndex = clusterIndex;

	// free all clusters of file from parameter cluster to last cluster.
	while (currentClusterIndex != FS_LASTCLUSTER) {

		// get next cluster index.
		if (k_getClusterLinkData(currentClusterIndex, &nextClusterIndex) == false) {
			return false;
		}

		// free current cluster.
		if (k_setClusterLinkData(currentClusterIndex, FS_FREECLUSTER) == false) {
			return false;
		}

		// move to next cluster.
		currentClusterIndex = nextClusterIndex;
	}

	return true;
}

/**
  ====================================================================================================
   < k_openFile Function's File Open Mode (mode) >
  ====================================================================================================
   File Open Mode  | Description
  ----------------------------------------------------------------------------------------------------
   "r"      | - read mode
            | - If file dosen't exist, open fails
  ----------------------------------------------------------------------------------------------------
   "w"      | - write mode
            | - If file dosen't exist, create file. If file exists, flush file.
  ----------------------------------------------------------------------------------------------------
   "a"      | - append mode
            | - If file dosen't exist, create file. If file exists, move to the end of file.
  ----------------------------------------------------------------------------------------------------
   "r+"     | - read write mode
            | - the same as "r" mode except writing is possible.
  ----------------------------------------------------------------------------------------------------
   "w+"     | - write read mode
            | - the same as "w" mode except reading is possible.
  ----------------------------------------------------------------------------------------------------
   "a+"     | - append read mode
            | - the same as "a" mode except reading is possible.
  ====================================================================================================
 */
File* k_openFile(const char* fileName, const char* mode) {
	DirEntry entry;      // directory entry of open file
	int dirEntryOffset;  // directory entry offset of open file
	int fileNameLen;     // file name length
	dword secondCluster; // second cluster index of open file
	File* file;          // file handle to return

	// check file name length
	fileNameLen = k_strlen(fileName);
	if ((fileNameLen > (sizeof(entry.fileName) - 1)) || (fileNameLen == 0)) {
		return null;
	}

	k_lock(&(g_fileSystemManager.mutex));

	//----------------------------------------------------------------------------------------------------
	// check file existing
	// and if file dosen't exist, check mode, and create file (w, w+, a, a+)
	//----------------------------------------------------------------------------------------------------

	// search directory entry matching file name.
	dirEntryOffset = k_findDirEntry(fileName, &entry);

	// If file dosen't exist.
	if (dirEntryOffset == -1) {

		// If it's read-related mode (r, r+), open fails.
		if (mode[0] == 'r') {
			k_unlock(&(g_fileSystemManager.mutex));
			return null;
		}

		// If it's other mode (w, w+, a, a+), create file.
		if (k_createFile(fileName, &entry, &dirEntryOffset) == false) {
			k_unlock(&(g_fileSystemManager.mutex));
			return null;
		}

	//----------------------------------------------------------------------------------------------------
	// If write-related mode (w, w+), flush file.
	// free all clusters of file except start cluster, set file size to 0.
	//----------------------------------------------------------------------------------------------------
	} else if (mode[0] == 'w') {

		// get second cluster index.
		if (k_getClusterLinkData(entry.startClusterIndex, &secondCluster) == false) {
			k_unlock(&(g_fileSystemManager.mutex));
			return null;
		}

		// set start cluster to last cluster.
		if (k_setClusterLinkData(entry.startClusterIndex, FS_LASTCLUSTER) == false) {
			k_unlock(&(g_fileSystemManager.mutex));
			return null;
		}

		// free all clusters from second cluster to last cluster.
		if (k_freeClusterUntilEnd(secondCluster) == false) {
			k_unlock(&(g_fileSystemManager.mutex));
			return null;
		}

		// set file size to 0.
		entry.fileSize = 0;
		if (k_setDirEntryData(dirEntryOffset, &entry) == false) {
			k_unlock(&(g_fileSystemManager.mutex));
			return null;
		}
	}

	//----------------------------------------------------------------------------------------------------
	// allocate file handle, set file info to it,
	// process append-related mode (a, a+), and return file handle.
	//----------------------------------------------------------------------------------------------------

	// allocate file handle.
	file = (File*)k_allocFileDirHandle();
	if (file == null) {
		k_unlock(&(g_fileSystemManager.mutex));
		return null;
	}

	// set file info to file handle.
	file->type = FS_TYPE_FILE;
	file->fileHandle.dirEntryOffset = dirEntryOffset;
	file->fileHandle.fileSize = entry.fileSize;
	file->fileHandle.startClusterIndex = entry.startClusterIndex;
	file->fileHandle.currentClusterIndex = entry.startClusterIndex;
	file->fileHandle.prevClusterIndex = entry.startClusterIndex;
	file->fileHandle.currentOffset = 0;

	// If it's append-related mode (a, a+), move file pointer to the end of file.
	if (mode[0] == 'a') {
		k_seekFile(file, 0, FS_SEEK_END);
	}

	k_unlock(&(g_fileSystemManager.mutex));

	// return file handle.
	return file;
}

dword k_readFile(void* buffer, dword size, dword count, File* file) {
	dword totalCount;       // total byte count
	dword readCount;        // read byte count
	dword offsetInCluster;  // file point offset in cluster
	dword copySize;         // byte count coping to buffer
	FileHandle* fileHandle; // file handle
	dword nextClusterIndex; // next cluster index

	// If handle == null or hanle type != file handle, return
	if ((file == null) || (file->type != FS_TYPE_FILE)) {
		return 0;
	}

	fileHandle = &(file->fileHandle);

	// If it's the end of file or the last cluster, return.
	if ((fileHandle->currentOffset == fileHandle->fileSize) || (fileHandle->currentClusterIndex == FS_LASTCLUSTER)) {
		return 0;
	}

	// total byte count = MIN(requested byte count, remained byte count of file)
	totalCount = MIN(size * count, fileHandle->fileSize - fileHandle->currentOffset);

	k_lock(&(g_fileSystemManager.mutex));

	// looping until finishing reading as many as total byte count.
	readCount = 0;
	while (readCount != totalCount) {

		//----------------------------------------------------------------------------------------------------
		// read current cluster, and copy it to buffer.
		//----------------------------------------------------------------------------------------------------

		// read current cluster.
		if (k_readCluster(fileHandle->currentClusterIndex, g_tempBuffer) == false) {
			break;
		}

		// calculate file pointer position in cluster.
		offsetInCluster = fileHandle->currentOffset % FS_CLUSTERSIZE;

		// If total byte count covers over many clusters, read as many as remained byte count in current cluster, and move to next cluster.
		// byte count coping to buffer = MIX(remained byte count in cluster, read byte count more)
		copySize = MIN(FS_CLUSTERSIZE - offsetInCluster, totalCount - readCount);

		// copy to buffer.
		k_memcpy((char*)buffer + readCount, g_tempBuffer + offsetInCluster, copySize);

		// update read byte count, current offset of file pointer.
		readCount += copySize;
		fileHandle->currentOffset += copySize;

		//----------------------------------------------------------------------------------------------------
		// finish reading current cluster, move to next cluster.
		//----------------------------------------------------------------------------------------------------
		if ((fileHandle->currentOffset % FS_CLUSTERSIZE) == 0) {

			// get next cluster index.
			if (k_getClusterLinkData(fileHandle->currentClusterIndex, &nextClusterIndex) == false) {
				break;
			}

		    // move to next cluster.
			fileHandle->prevClusterIndex = fileHandle->currentClusterIndex;
			fileHandle->currentClusterIndex = nextClusterIndex;
		}
	}

	k_unlock(&(g_fileSystemManager.mutex));

	// return read byte count.
	return readCount;
}

static bool k_updateDirEntry(FileHandle* fileHandle) {
	DirEntry entry;

	// get directory entry.
	if ((fileHandle == null) || (k_getDirEntryData(fileHandle->dirEntryOffset, &entry) == false)) {
		return false;
	}

	// update file size, start cluster index.
	entry.fileSize = fileHandle->fileSize;
	entry.startClusterIndex = fileHandle->startClusterIndex;

	// set directory entry.
	if (k_setDirEntryData(fileHandle->dirEntryOffset, &entry) == false) {
		return false;
	}

	return true;
}

dword k_writeFile(const void* buffer, dword size, dword count, File* file) {
	dword totalCount;          // total byte count
	dword writeCount;          // write byte count
	dword offsetInCluster;     // file pointer offset in cluster
	dword copySize;            // byte count copying to buffer
	FileHandle* fileHandle;    // file handle
	dword nextClusterIndex;    // next cluster index
	dword allocedClusterIndex; // allocated cluster index

	// If handle == null or handle type != file handle, return.
	if ((file == null) || (file->type != FS_TYPE_FILE)) {
		return 0;
	}

	fileHandle = &(file->fileHandle);

	// total byte count = requested byte count
	totalCount = size * count;

	k_lock(&(g_fileSystemManager.mutex));

	// loop until finishing writing as many as total byte count.
	writeCount = 0;
	while (writeCount != totalCount) {

		//----------------------------------------------------------------------------------------------------
		// If current cluster is last cluster, allocate new cluster, and connect it.
		//----------------------------------------------------------------------------------------------------
		if (fileHandle->currentClusterIndex == FS_LASTCLUSTER) {

			// allocate new cluster.
			allocedClusterIndex = k_findFreeCluster();
			if (allocedClusterIndex == FS_LASTCLUSTER) {
				break;
			}

			// set new cluster to last cluster.
			if (k_setClusterLinkData(allocedClusterIndex, FS_LASTCLUSTER) == false) {
				break;
			}

			// connect new cluster to last cluster
			if (k_setClusterLinkData(fileHandle->prevClusterIndex, allocedClusterIndex) == false) {
				// If fails, free the allocated cluster.
				k_setClusterLinkData(allocedClusterIndex, FS_FREECLUSTER);
				break;
			}

			// set current cluster to new cluster.
			fileHandle->currentClusterIndex = allocedClusterIndex;

			// initialize temporary buffer.
			k_memset(g_tempBuffer, 0, sizeof(g_tempBuffer));

		//----------------------------------------------------------------------------------------------------
		// If current cluster can't be written all, read current cluster, and copy it to temporary buffer.
		//----------------------------------------------------------------------------------------------------
		} else if (((fileHandle->currentOffset % FS_CLUSTERSIZE) != 0) || ((totalCount - writeCount) < FS_CLUSTERSIZE)) {

            // read current cluster and copy it to temporary buffer,
			//  because it overrides the part of cluster if it dosen't override the whole cluster.
			if (k_readCluster(fileHandle->currentClusterIndex, g_tempBuffer) == false) {
				break;
			}
		}

		// calculate file pointer offset in cluster.
		offsetInCluster = fileHandle->currentOffset % FS_CLUSTERSIZE;

		// If total byte count covers over many clusters, write as many as remained byte count of current cluster, move to next cluster.
		// byte count coping to buffer = MIN(remained byte count of cluster, write byte count more)
		copySize = MIN(FS_CLUSTERSIZE - offsetInCluster, totalCount - writeCount);

		// copy from buffer to temporary buffer.
		k_memcpy(g_tempBuffer + offsetInCluster, (char*)buffer + writeCount, copySize);

		// write temporary buffer to hard disk.
		if (k_writeCluster(fileHandle->currentClusterIndex, g_tempBuffer) == false) {
			break;
		}

		// update write byte count, current offset of file pointer.
		writeCount += copySize;
		fileHandle->currentOffset += copySize;

		//----------------------------------------------------------------------------------------------------
		// If it finishes writing current cluster, move to next cluster.
		//----------------------------------------------------------------------------------------------------
		if ((fileHandle->currentOffset % FS_CLUSTERSIZE) == 0) {

			// get next cluster index.
			if (k_getClusterLinkData(fileHandle->currentClusterIndex, &nextClusterIndex) == false) {
				break;
			}

			// move to next cluster.
			fileHandle->prevClusterIndex = fileHandle->currentClusterIndex;
			fileHandle->currentClusterIndex = nextClusterIndex;
		}
	}

	//----------------------------------------------------------------------------------------------------
	// If file size changes, update directory entry.
	//----------------------------------------------------------------------------------------------------
	if (fileHandle->fileSize < fileHandle->currentOffset) {
		fileHandle->fileSize = fileHandle->currentOffset;
		k_updateDirEntry(fileHandle);
	}

	k_unlock(&(g_fileSystemManager.mutex));

	// return write byte count.
	return writeCount;
}

bool k_writeZero(File* file, dword count) {
	byte* buffer;
	dword remainCount;
	dword writeCount;

	if (file == null) {
		return false;
	}

	// allocate memory and write by cluster unit in order to improve speed.
	buffer = (byte*)k_allocMem(FS_CLUSTERSIZE);
	if (buffer == null) {
		return false;
	}

	// put 0 to buffer.
	k_memset(buffer, 0, FS_CLUSTERSIZE);

	// write by cluster unit.
	remainCount = count;
	while (remainCount != 0) {
		writeCount = MIN(remainCount, FS_CLUSTERSIZE);
		if (k_writeFile(buffer, 1, writeCount, file) != writeCount) {
			k_freeMem(buffer);
			return false;
		}

		remainCount -= writeCount;
	}

	k_freeMem(buffer);
	return true;
}

int k_seekFile(File* file, int offset, int origin) {
	dword realOffset;           // real offset: file pointer offset from the start of file
	dword clusterOffsetToMove;  // moving cluster offset
	dword currentClusterOffset; // current cluster offset
	dword lastClusterOffset;    // last cluster offset
	dword moveCount;            // moving cluster count
	dword i;                    // index
	dword startClusterIndex;    // start cluster index
	dword prevClusterIndex;     // previous cluster index
	dword currentClusterIndex;  // current cluster index
	FileHandle* fileHandle;     // file handle

	// If handle == null or handle type != file handle, return
	if ((file == null) || (file->type != FS_TYPE_FILE)) {
		return 0;
	}

	fileHandle = &(file->fileHandle);

	//----------------------------------------------------------------------------------------------------
	// calculate file pointer offset from the start of file using parameter origin, offset.
	//----------------------------------------------------------------------------------------------------

	// calculate real file pointer offset using parameter origin.
	// If parameter origin is a negative number, move to the start of file.
	// If parameter origin is a positive number, move to the end of file.
	switch (origin) {
    // move file pointer from the start of file.
	case FS_SEEK_SET:
		// If parameter offset is a negative number, move out of file, so set real offset to the start of file (0).
		if (offset < 0) {
			realOffset = 0;

		} else {
			realOffset = offset;
		}

		break;

	// move file pointer from current file pointer offset.
	case FS_SEEK_CUR:
		// If parameter offset is a negative number and parameter offset is greater than current file pointer offset,
		// move out of file, so set real offset to the start of file (0).
		if ((offset < 0) && (fileHandle->currentOffset <= (dword)-offset)) {
			realOffset = 0;

		} else {
			realOffset = fileHandle->currentOffset + offset;
		}

		break;

	// move file pointer from the end of file.
	case FS_SEEK_END:
		// If parameter offset is a negative number and parameter offset is greater than file size.
		// move out of file, so set real offset to the start of file (0).
		if ((offset < 0) && (fileHandle->fileSize <= (dword)-offset)) {
			realOffset = 0;

		} else {
			realOffset = fileHandle->fileSize + offset;
		}

		break;
	}

	//----------------------------------------------------------------------------------------------------
	// search cluster to move using file cluster count and current file pointer offset.
	//----------------------------------------------------------------------------------------------------

	// calculate last cluster offset, moving cluster offset, current cluster offset.
	lastClusterOffset = fileHandle->fileSize / FS_CLUSTERSIZE;
	clusterOffsetToMove = realOffset / FS_CLUSTERSIZE;
	currentClusterOffset = fileHandle->currentOffset / FS_CLUSTERSIZE;

	// calculate moving cluster count, start cluster index.
	if (lastClusterOffset < clusterOffsetToMove) {
		moveCount = lastClusterOffset - currentClusterOffset;
		startClusterIndex = fileHandle->currentClusterIndex;

	} else if (currentClusterOffset <= clusterOffsetToMove) {
		moveCount = clusterOffsetToMove - currentClusterOffset;
		startClusterIndex = fileHandle->currentClusterIndex;

	} else {
		moveCount = clusterOffsetToMove;
		startClusterIndex = fileHandle->startClusterIndex;
	}

	k_lock(&(g_fileSystemManager.mutex));

	// move cluster
	currentClusterIndex = startClusterIndex;
	for (i = 0; i < moveCount; i++) {
		prevClusterIndex = currentClusterIndex;

		// get next cluster index.
		if (k_getClusterLinkData(prevClusterIndex, &currentClusterIndex) == false) {
			k_unlock(&(g_fileSystemManager.mutex));
			return -1;
		}
	}

	// update cluster info of file handle.
	if (moveCount > 0) {
		fileHandle->prevClusterIndex = prevClusterIndex;
		fileHandle->currentClusterIndex = currentClusterIndex;

	} else if (startClusterIndex == fileHandle->startClusterIndex) {
		fileHandle->prevClusterIndex = fileHandle->startClusterIndex;
		fileHandle->currentClusterIndex = fileHandle->startClusterIndex;
	}

	//----------------------------------------------------------------------------------------------------
	// If moving offset exceeds file size, put 0 to the remained part, update current file pointer offset.
	//----------------------------------------------------------------------------------------------------

	// If moving offset exceeds file size.
	if (lastClusterOffset < clusterOffsetToMove) {
		fileHandle->currentOffset = fileHandle->fileSize;

		k_unlock(&(g_fileSystemManager.mutex));

		// put 0 to the remained part in order to expand file size.
		if (k_writeZero(file, realOffset - fileHandle->fileSize) == false) {
			return 0;
		}
	}

	// update current file pointer offset.
	fileHandle->currentOffset = realOffset;

	k_unlock(&(g_fileSystemManager.mutex));

	return 0;
}

int k_closeFile(File* file) {
	// If handle == null of handle type != file handle, return.
	if ((file == null) || (file->type != FS_TYPE_FILE)) {
		return -1;
	}

	// free file handle.
	k_freeFileDirHandle(file);
	return 0;
}

bool k_isFileOpen(const DirEntry* entry) {
	int i;
	File* file;

	// search open file from start address to end address of handle pool.
	file = g_fileSystemManager.handlePool;
	for (i = 0; i < FS_HANDLE_MAXCOUNT; i++) {
		if ((file[i].type == FS_TYPE_FILE) && (file[i].fileHandle.startClusterIndex == entry->startClusterIndex)) {
			return true;
		}
	}

	return false;
}

int k_removeFile(const char* fileName) {
	DirEntry entry;
	int dirEntryOffset;
	int fileNameLen;

	// check file name length.
	fileNameLen = k_strlen(fileName);
	if ((fileNameLen > (sizeof(entry.fileName) - 1)) || (fileNameLen == 0)) {
		return -1;
	}

	k_lock(&(g_fileSystemManager.mutex));

	// If file dosen't exist, can't delete it.
	dirEntryOffset = k_findDirEntry(fileName, &entry);
	if (dirEntryOffset == -1) {
		k_unlock(&(g_fileSystemManager.mutex));
		return -1;
	}

	// If file is open, can't delete it.
	if (k_isFileOpen(&entry) == true) {
		k_unlock(&(g_fileSystemManager.mutex));
		return -1;
	}

	// free all clusters of file.
	if (k_freeClusterUntilEnd(entry.startClusterIndex) == false) {
		k_unlock(&(g_fileSystemManager.mutex));
		return -1;
	}

	// initialize directory entry as 0.
	k_memset(&entry, 0, sizeof(entry));
	if (k_setDirEntryData(dirEntryOffset, &entry) == false) {
		k_unlock(&(g_fileSystemManager.mutex));
		return -1;
	}

	k_unlock(&(g_fileSystemManager.mutex));
	return 0;
}

// ignore dirName, because only root directory exists.
Dir* k_openDir(const char* dirName) {
	Dir* dir;
	DirEntry* dirBuffer;

	k_lock(&(g_fileSystemManager.mutex));

	// allocate directory handle.
	dir = (Dir*)k_allocFileDirHandle();
	if (dir == null) {
		k_unlock(&(g_fileSystemManager.mutex));
		return null;
	}

	// allocate root directory buffer.
	dirBuffer = (DirEntry*)k_allocMem(FS_CLUSTERSIZE);
	if (dirBuffer == null) {
		k_freeFileDirHandle(dir);
		k_unlock(&(g_fileSystemManager.mutex));
		return null;
	}

	// read root directory.
	if (k_readCluster(0, (byte*)dirBuffer) == false) {
		k_freeFileDirHandle(dir);
		k_freeMem(dirBuffer);
		k_unlock(&(g_fileSystemManager.mutex));
		return null;
	}

	// initialize directory handle.
	dir->type = FS_TYPE_DIRECTORY;
	dir->dirHandle.currentOffset = 0;
	dir->dirHandle.dirBuffer = dirBuffer;

	k_unlock(&(g_fileSystemManager.mutex));

	return dir;
}

DirEntry* k_readDir(Dir* dir) {
	DirHandle* dirHandle;
	DirEntry* entry;

	// If handle == null or handle type != directory handle, return.
	if ((dir == null) || (dir->type != FS_TYPE_DIRECTORY)) {
		return null;
	}

	dirHandle = &(dir->dirHandle);

	// check the range of current directory pointer offset.
	if ((dirHandle->currentOffset < 0) || (dirHandle->currentOffset >= FS_MAXDIRECTORYENTRYCOUNT)) {
		return null;
	}

	k_lock(&(g_fileSystemManager.mutex));

	// search as many as max directory entry count of root directory.
	entry = dirHandle->dirBuffer;
	while (dirHandle->currentOffset < FS_MAXDIRECTORYENTRYCOUNT) {
		// If file exists, return the directory entry.
		if (entry[dirHandle->currentOffset].startClusterIndex != 0) {
			k_unlock(&(g_fileSystemManager.mutex));
			return &(entry[dirHandle->currentOffset++]);
		}

		dirHandle->currentOffset++;
	}

	k_unlock(&(g_fileSystemManager.mutex));
	return null;
}

void k_rewindDir(Dir* dir) {
	DirHandle* dirHandle;

	// If handle == null or handle type != directory handle, return.
	if ((dir == null) || (dir->type != FS_TYPE_DIRECTORY)) {
		return;
	}

	dirHandle = &(dir->dirHandle);

	k_lock(&(g_fileSystemManager.mutex));

	// set 0 to current directory pointer offset.
	dirHandle->currentOffset = 0;

	k_unlock(&(g_fileSystemManager.mutex));

}

int k_closeDir(Dir* dir) {
	DirHandle* dirHandle;

	// If handle == null or handle type != directory handle, return.
	if ((dir == null) || (dir->type != FS_TYPE_DIRECTORY)) {
		return -1;
	}

	dirHandle = &(dir->dirHandle);

	k_lock(&(g_fileSystemManager.mutex));

	// free root directory buffer.
	k_freeMem(dirHandle->dirBuffer);

	// free directory handle.
	k_freeFileDirHandle(dir);

	k_unlock(&(g_fileSystemManager.mutex));

	return 0;
}

bool k_flushFileSystemCache(void) {
	CacheBuffer* cacheBuffer;
	int cacheCount;
	int i;

	if (g_fileSystemManager.cacheEnabled == false) {
		return true;
	}

	k_lock(&(g_fileSystemManager.mutex));

	// write changed cache buffer to hard disk. (cluster link table area)
	k_getCacheBufferAndCount(CACHE_CLUSTERLINKTABLEAREA, &cacheBuffer, &cacheCount);
	for (i = 0; i < cacheCount; i++) {
		if (cacheBuffer[i].changed == true) {
			if (k_writeClusterLinkTableWithoutCache(cacheBuffer[i].tag, cacheBuffer[i].buffer) == false) {
				return false;
			}

			cacheBuffer[i].changed = false;
		}
	}

	// write changed cache buffer to hard disk. (data area)
	k_getCacheBufferAndCount(CACHE_DATAAREA, &cacheBuffer, &cacheCount);
	for (i = 0; i < cacheCount; i++) {
		if (cacheBuffer[i].changed == true) {
			if (k_writeClusterWithoutCache(cacheBuffer[i].tag, cacheBuffer[i].buffer) == false) {
				return false;
			}

			cacheBuffer[i].changed = false;
		}
	}

	k_unlock(&(g_fileSystemManager.mutex));

	return true;
}
