#ifndef __CORE_SYNC_H__
#define __CORE_SYNC_H__

#include "types.h"

/**
  < Systemlock >
  - Systemlock synchronizes data used among tasks and interrupt handlers.
  - Systemlock disables interrupts only in the current core.
  - Systemlock is not safe in multi-core processor.
  
  < Mutex >
  - Mutex synchronizes data used among tasks.
  - Mutex does not disable interrupts.
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

// lock type
#define LOCK_TYPE_MUTEX    1 // mutex
#define LOCK_TYPE_SPINLOCK 2 // spinlock

#pragma pack(push, 1)

typedef struct k_Mutex {
  volatile byte type;       // lock type (mutex): [NOTE] Lock type must be the first field.
	volatile qword taskId;    // lock-executing task ID
	volatile dword lockCount; // lock count: Mutex allows duplicated lock.
	volatile bool lockFlag;   // lock flag
	byte padding[2];          // padding bytes for alignment
} Mutex; // align structure size with 8 bytes.

typedef struct k_Spinlock {
  volatile byte type;          // lock type (spinlock): [NOTE] Lock type must be the first field.
	volatile dword lockCount;    // lock count: Spinlock allows duplicated lock.
	volatile byte apicId;        // lock-executing core APIC ID
	volatile bool lockFlag;      // lock flag
	volatile bool interruptFlag; // interrupt flag: Spinlock disables interrupts and restore them.
} Spinlock; // align structure size with 8 bytes.

#pragma pack(pop)

#if 0
bool k_lockSystem(void); // [NOTE] Systemlock is not safe in multi-core processor.
void k_unlockSystem(bool interruptFlag); // [NOTE] Systemlock is not safe in multi-core processor.
#endif

/* Mutex Functions */
void k_initMutex(Mutex* mutex);
void k_lock(Mutex* mutex);
void k_unlock(Mutex* mutex);

/* Spinlock Functions */
void k_initSpinlock(Spinlock* spinlock);
void k_lockSpin(Spinlock* spinlock);
void k_unlockSpin(Spinlock* spinlock);

/* Any Functions */
void k_lockAny(void* lock);
void k_unlockAny(void* lock);

#endif // __CORE_SYNC_H__