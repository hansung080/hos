#ifndef __CORE_FILESYSTEM_H__
#define __CORE_FILESYSTEM_H__

#include "types.h"
#include "sync.h"
#include "hdd.h"
#include "cache.h"

// file system macros
#define FS_SIGNATURE              0x7E38CF10 // HansFS signature.
#define FS_SECTORSPERCLUSTER      8          // sector count per cluster (8)
#define FS_LASTCLUSTER            0xFFFFFFFF // last cluster
#define FS_FREECLUSTER            0x00       // free cluster
#define FS_CLUSTERSIZE            (FS_SECTORSPERCLUSTER * 512)        // cluster size (byte count, 4KB)
#define FS_MAXDIRECTORYENTRYCOUNT (FS_CLUSTERSIZE / sizeof(DirEntry)) // max directory entry count of root directory (128)
#define FS_HANDLE_MAXCOUNT        3072 // max handle count(max file count, max directory count): 3072 = 1024 (max task count) * 3
#define FS_MAXFILENAMELENGTH      24   // max file name length (including file extension and last null character)

// handle types
#define FS_TYPE_FREE      0 // free handle
#define FS_TYPE_FILE      1 // file handle
#define FS_TYPE_DIRECTORY 2 // directory handle

// SEEK options
#define FS_SEEK_SET 0 // the start of file
#define FS_SEEK_CUR 1 // current file pointer offset
#define FS_SEEK_END 2 // the end of file

// function pointers related with hard disk and ram disk control
typedef bool (* ReadHddInfo)(bool primary, bool master, HddInfo* hddInfo);
typedef int (* ReadHddSector)(bool primary, bool master, dword lba, int sectorCount, char* buffer);
typedef int (* WriteHddSector)(bool primary, bool master, dword lba, int sectorCount, char* buffer);

/* macros redefined as C standard in/out names  */
// redefine HansFS function names as C standard in/out function names.
#define fopen     k_openFile
#define fread     k_readFile
#define fwrite    k_writeFile
#define fseek     k_seekFile
#define fclose    k_closeFile
#define remove    k_removeFile
#define opendir   k_openDir
#define readdir   k_readDir
#define rewinddir k_rewindDir
#define closedir  k_closeDir

// redefine HansFS macro names as C standard in/out macro names.
#define SEEK_SET FS_SEEK_SET
#define SEEK_CUR FS_SEEK_CUR
#define SEEK_END FS_SEEK_END

// redefine HansFS type names as C standard in/out type names.
#define size_t dword
#define dirent DirEntry
#define d_name fileName
#define d_size fileSize

/**
  ====================================================================================================
   < HansFS Structure >
  ----------------------------------------------------------------------------------------------------
  |                    Meta Data Area                       |           General Data Area            |
  ----------------------------------------------------------------------------------------------------
  | MBR Area                | Reserved | Cluster Link Table | Root Directory               | Data    |
  | (LBA 0, 1 sector-sized) | Area     | Area               | (cluster 0, 1 cluster-sized) | Area    |
  ----------------------------------------------------------------------------------------------------
   -> MBR Area (512B): boot-loader code and file system info (446B), partition table(16B*4=64B), boot-loader signature(2B)
   -> Reserved Area: not used
   -> Cluster Link Table Area: can create 128 cluster links (4B) in a sector (512B).
                               the size of cluster link table area depends on the size of hard disk.
   -> Root Directory (4KB): can create 128 directory entries (32B) in root directory (4KB).
                            128 files can be created in root directory.
   -> Data Area: file data exists in this area.
  ====================================================================================================

  ====================================================================================================
   < HansFS Algorithm >
     MBR   Reserved   CL Table         C0(Root)     C1       C2       C3       C4       C5      ...
  ----------------------------------------------------------------------------------------------------
  |       |       | C0 : 0xFFFFFFFF | FN, FS, C1 |        |        |        |        |        |      |
  |       |       | C1 : C3         |            |        |        |        |        |        |      |
  |       |       | C2 : 0x00       |            |        |        |        |        |        |      |
  |  ...  |  (X)  | C3 : C5         |    ...     |  FD1   |        |  FD2   |        |  FD3   | ...  |
  |       |       | C4 : 0x00       |            |        |        |        |        |        |      |
  |       |       | C5 : 0xFFFFFFFF |            |        |        |        |        |        |      |
  |       |       |      ...        |            |        |        |        |        |        |      |
  ----------------------------------------------------------------------------------------------------
   -> C=Cluster, CL=ClusterLink, FN=FileName, FS=FileSize, FD=FileData
   -> cluster link -> current cluster index : next cluster index (but, 0xFFFFFFFF=last cluster, 0x00=free cluster)
  ====================================================================================================
*/

#pragma pack(push, 1)

typedef struct k_Partition {
	byte bootableFlag;       // [byte 0]     : booting enable flag: [0x80:booting enable], [0x00:booting disable]
	byte startingChsAddr[3]; // [byte 1~3]   : partition start CHS address: use LBA address instead of CHS address.
	byte partitionType;      // [byte 4]     : partition type: [0x00:not-used partition], [0x0C:FAT32 file system], [0x83:linux file system]
	byte endingChsAddr[3];   // [byte 5~7]   : partition end CHS address: use LBA address instead of CHS address.
	dword startingLbaAddr;   // [byte 8~11]  : partition start LBA address
	dword sizeInSector;      // [byte 12~15] : sector count of partition
} Partition; // 16 bytes-sized

typedef struct k_Mbr {
	byte bootCode[430];           // boot-loader code
	dword signature;              // file system signature (0x7E38CF10)
	dword reservedSectorCount;    // sector count of reserved area
	dword clusterLinkSectorCount; // sector count of cluster link table area
	dword totalClusterCount;      // total cluster count of general data area
	Partition partition[4];       // partition table
	byte bootLoaderSignature[2];  // boot-loader signature (0x55, 0xAA)
} Mbr; // 1 sector-sized (512 bytes)

typedef struct k_DirEntry {
	char fileName[FS_MAXFILENAMELENGTH]; // [byte 0~23]  : file name (including file extension and last null character)
	dword fileSize;                      // [byte 24~27] : file size (byte unit)
	dword startClusterIndex;             // [byte 28~31] : start cluster index (0x00:free directory entry)
} DirEntry; // 32 bytes-sized

typedef struct k_FileHandle {
	int dirEntryOffset;        // directory entry offset (directory entry index matching file name)
	dword fileSize;            // file size (byte unit)
	dword startClusterIndex;   // start cluster index
	dword currentClusterIndex; // current cluster index (cluster index which current I/O is working)
	dword prevClusterIndex;    // previous cluster index
	dword currentOffset;       // current file pointer offset (byte unit)
} FileHandle;

typedef struct k_DirHandle {
	DirEntry* dirBuffer; // root directory buffer
	int currentOffset;   // current directory pointer offset
} DirHandle;

typedef struct k_FileDirHandle {
	byte type; // handle type: free handle, file handle, directory handle	
	union {
		FileHandle fileHandle; // file handle
		DirHandle dirHandle;   // directory handle
	};
} File, Dir;

typedef struct k_FileSystemManager {
	bool mounted;                             // file system mount flag
	dword reservedSectorCount;                // sector count of reserved area
	dword clusterLinkAreaStartAddr;           // start address of cluster link table area (sector unit)
	dword clusterLinkAreaSize;                // size of cluster link table area (sector count)
	dword dataAreaStartAddr;                  // start address of general data area (sector unit)
	dword totalClusterCount;                  // total cluster count of general data area
	dword lastAllocedClusterLinkSectorOffset; // sector offset of last allocated cluster link table
	Mutex mutex;                              // mutex: synchronization object
	File* handlePool;                         // file/directory handle pool address
	bool cacheEnabled;                        // cache enable flag
} FileSystemManager;

#pragma pack(pop)

/* General Functions */
bool k_initFileSystem(void);
bool k_format(void);
bool k_mount(void);
bool k_getHddInfo(HddInfo* info);

/* Low Level Functions */
static bool k_readClusterLinkTable(dword offset, byte* buffer);
static bool k_writeClusterLinkTable(dword offset, byte* buffer);
static bool k_readCluster(dword offset, byte* buffer);
static bool k_writeCluster(dword offset, byte* buffer);
static dword k_findFreeCluster(void);
static bool k_setClusterLinkData(dword clusterIndex, dword data);
static bool k_getClusterLinkData(dword clusterIndex, dword* data);
static int k_findFreeDirEntry(void);
static bool k_setDirEntryData(int index, DirEntry* entry);
static bool k_getDirEntryData(int index, DirEntry* entry);
static int k_findDirEntry(const char* fileName, DirEntry* entry);
void k_getFileSystemInfo(FileSystemManager* manager);

/* High Level Functions */
File* k_openFile(const char* fileName, const char* mode);
dword k_readFile(void* buffer, dword size, dword count, File* file);
dword k_writeFile(const void* buffer, dword size, dword count, File* file);
int k_seekFile(File* file, int offset, int origin);
int k_closeFile(File* file);
int k_removeFile(const char* fileName);
Dir* k_openDir(const char* dirName);
DirEntry* k_readDir(Dir* dir);
void k_rewindDir(Dir* dir);
int k_closeDir(Dir* dir);
bool k_writeZero(File* file, dword count);
bool k_isFileOpen(const DirEntry* entry);
static void* k_allocFileDirHandle(void);
static void k_freeFileDirHandle(File* file);
static bool k_createFile(const char* fileName, DirEntry* entry, int* dirEntryIndex);
static bool k_freeClusterUntilEnd(dword clusterIndex);
static bool k_updateDirEntry(FileHandle* fileHandle);

/* Cache-related Functions */
static bool k_readClusterLinkTableWithoutCache(dword offset, byte* buffer);
static bool k_readClusterLinkTableWithCache(dword offset, byte* buffer);
static bool k_writeClusterLinkTableWithoutCache(dword offset, byte* buffer);
static bool k_writeClusterLinkTableWithCache(dword offset, byte* buffer);
static bool k_readClusterWithoutCache(dword offset, byte* buffer);
static bool k_readClusterWithCache(dword offset, byte* buffer);
static bool k_writeClusterWithoutCache(dword offset, byte* buffer);
static bool k_writeClusterWithCache(dword offset, byte* buffer);
static CacheBuffer* k_allocCacheBufferWithFlush(int cacheTableIndex);
bool k_flushFileSystemCache(void);

#endif // __CORE_FILESYSTEM_H__
