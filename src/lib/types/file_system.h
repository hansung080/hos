#ifndef __TYPES_FILESYSTEM_H__
#define __TYPES_FILESYSTEM_H__

#include "types.h"

// max file name length
#define FS_MAXFILENAMELENGTH      24   // max file name length (include file extension and last null character)

// SEEK option
#define SEEK_SET 0 // start of file
#define SEEK_CUR 1 // current file pointer offset
#define SEEK_END 2 // end of file

// redefine hFS type names as C standard I/O type names.
#define size_t dword
#define dirent DirEntry
#define d_name fileName
#define d_size fileSize

#pragma pack(push, 1)

typedef struct __DirEntry {
	char fileName[FS_MAXFILENAMELENGTH]; // [byte 0~23]  : file name (include file extension and last null character)
	dword fileSize;                      // [byte 24~27] : file size (byte-level)
	dword startClusterIndex;             // [byte 28~31] : start cluster index (0x00:free directory entry)
} DirEntry; // 32 bytes-sized

typedef struct __FileHandle {
	int dirEntryOffset;        // directory entry offset (directory entry index matching file name)
	dword fileSize;            // file size (byte-level)
	dword startClusterIndex;   // start cluster index
	dword currentClusterIndex; // current cluster index (cluster index which current I/O is working)
	dword prevClusterIndex;    // previous cluster index
	dword currentOffset;       // current file pointer offset (byte-level)
} FileHandle;

typedef struct __DirHandle {
	DirEntry* dirBuffer; // root directory buffer
	int currentOffset;   // current directory pointer offset
} DirHandle;

typedef struct __FileDirHandle {
	byte type; // handle type: free handle, file handle, directory handle	
	union {
		FileHandle fileHandle; // file handle
		DirHandle dirHandle;   // directory handle
	};
} File, Dir;

#pragma pack(pop)

#endif // __TYPES_FILESYSTEM_H__