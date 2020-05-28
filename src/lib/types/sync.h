#ifndef __TYPES_SYNC_H__
#define __TYPES_SYNC_H__

#include "types.h"

#pragma pack(push, 1)

typedef struct __Mutex {
	volatile qword taskId;    // lock-executing task ID
	volatile dword lockCount; // lock count: Mutex allows duplicated lock.
	volatile bool lockFlag;   // lock flag
	byte padding[3];          // padding bytes: align structure size with 8 bytes.
} Mutex;

#pragma pack(pop)

#endif // __TYPES_SYNC_H__