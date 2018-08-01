#ifndef __SYNC_H__
#define __SYNC_H__

#include "types.h"

#pragma pack(push, 1)

/**
  < Systemlock >
  - Systemlock synchronizes data used among tasks and interrupt handlers.
  - Systemlock disables interrupts only in the current core.
  - Systemlock is not safe in multi-core processor.
  
  < Mutex >
  - Mutex synchronizes data used among tasks.
  - Mutex dose not disable interrupts.
  - Mutex uses lock flag to control race condition among tasks.
  - Mutex is safe in multi-core processor.
  - Mutex allows duplicated lock.
  - If it's already locked, task repeats task switching until it gets lock.
  
  < Spinlock >
  - Spinlock synchronizes data used among tasks and interrupt handlers.
  - Spinlock disables interrupts only in the current core.
  - Spinlock uses lock flag to control race condition among tasks and interrupt handlers.
  - Spinlock is safe in multi-core processor.
  - Spinlock allows duplicated lock.
  - If it's already locked, task or interrupt handler repeats retrying until it gets lock.
  - You can not do task switching after getting lock, because interrupts are disabled.
*/

typedef struct k_Mutex {
	volatile qword taskId;    // lock-executing task ID
	volatile dword lockCount; // lock count: Mutex allows duplicated lock.
	volatile bool lockFlag;   // lock flag
	byte padding[3];          // padding bytes: align structure size with 8 bytes.
} Mutex;

typedef struct k_Spinlock {
	volatile dword lockCount;    // lock count: Spinlock allows duplicated lock.
	volatile byte apicId;        // lock-executing core APIC ID
	volatile bool lockFlag;      // lock flag
	volatile bool interruptFlag; // interrupt flag: Spinlock disables interrupts and restore them.
	byte padding[1];             // padding bytes: align structure size with 8 bytes.
} Spinlock;

#pragma pack(pop)

#if 0
bool k_lockSystem(void); // [Note] Systemlock is not safe in multi-core processor.
void k_unlockSystem(bool interruptFlag); // [Note] Systemlock is not safe in multi-core processor.
#endif

void k_initMutex(Mutex* mutex);
void k_lock(Mutex* mutex);
void k_unlock(Mutex* mutex);
void k_initSpinlock(Spinlock* spinlock);
void k_lockSpin(Spinlock* spinlock);
void k_unlockSpin(Spinlock* spinlock);

#endif // __SYNC_H__
