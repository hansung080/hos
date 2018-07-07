#ifndef __SYNC_H__
#define __SYNC_H__

#include "types.h"

#pragma pack(push, 1)

typedef struct k_Mutex {
	volatile qword taskId;    // lock-executing Task ID
	volatile dword lockCount; // lock count
	volatile bool lockFlag;   // lock flag
	byte padding[3];          // padding bytes: This field is for aligning structure size with 8 bytes.
} Mutex;

#pragma pack(pop)

bool k_lockSystem(void); // lock: synchronize between task and interrupt.
void k_unlockSystem(bool interruptFlag); // unlock: synchronize between task and interrupt.
void k_initMutex(Mutex* mutex); // initialize mutex.
void k_lock(Mutex* mutex); // lock: synchronize between task and task using mutex.
void k_unlock(Mutex* mutex); // unlock: synchronize between task and task using mutex.

#endif // __SYNC_H__
