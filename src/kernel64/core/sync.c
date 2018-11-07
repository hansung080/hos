#include "sync.h"
#include "../utils/util.h"
#include "task.h"
#include "asm_util.h"
#include "multiprocessor.h"

#if 0
bool k_lockSystem(void) {
	return k_setInterruptFlag(false);
}

void k_unlockSystem(bool interruptFlag) {
	k_setInterruptFlag(interruptFlag);
}
#endif

void k_initMutex(Mutex* mutex) {
	mutex->type = LOCK_TYPE_MUTEX;
	mutex->lockFlag = false;
	mutex->lockCount = 0;
	mutex->taskId = TASK_INVALIDID;
}

void k_lock(Mutex* mutex) {
	byte currentApicId;
	bool interruptFlag;
	
	// disable interrupt while getting a lock.
	interruptFlag = k_setInterruptFlag(false);
	
	currentApicId = k_getApicId();
	
	// If it's already locked, process below.
	if (k_testAndSet(&(mutex->lockFlag), false, true) == false) {
		// If it's locked by itself (running task), increase lock count and return.
		if (mutex->taskId == k_getRunningTask(currentApicId)->link.id) {
			k_setInterruptFlag(interruptFlag);
			mutex->lockCount++;
			return;
		}
		
		// If it's locked by another task, wait until it will be unlocked.
		while (k_testAndSet(&(mutex->lockFlag), false, true) == false) {
			// hand processor over to another task while waiting in order to prevent unnecessary usage of processor.
			k_schedule();
		}
	}
	
	// If it's unlocked, lock it.
	// already set <mutex->lockFlag = true> in k_testAndSet using atomic operation.
	mutex->lockCount = 1;
	mutex->taskId = k_getRunningTask(currentApicId)->link.id;
	k_setInterruptFlag(interruptFlag);
}

void k_unlock(Mutex* mutex) {
	bool interruptFlag;
	
	// disable interrupt while returning a lock.
	interruptFlag = k_setInterruptFlag(false);
	
	// If it's already unlocked or it's locked by another task, return.
	if ((mutex->lockFlag == false) || (mutex->taskId != k_getRunningTask(k_getApicId())->link.id)) {
		k_setInterruptFlag(interruptFlag);
		return;
	}
	
	// If it's locked more than twice, decrease lock count and return.
	if (mutex->lockCount > 1) {
		mutex->lockCount--;
		
	// If it's locked once, unlock it.
	// Setting lock flag to false must be done at the last.
	} else {
		mutex->taskId = TASK_INVALIDID;
		mutex->lockCount = 0;
		mutex->lockFlag = false;
	}
	
	k_setInterruptFlag(interruptFlag);
}

void k_initSpinlock(Spinlock* spinlock) {
	spinlock->type = LOCK_TYPE_SPINLOCK;
	spinlock->lockFlag = false;
	spinlock->lockCount = 0;
	spinlock->apicId = APICID_INVALID;
	spinlock->interruptFlag = false;
}

void k_lockSpin(Spinlock* spinlock) {
	bool interruptFlag;
	
	// disable interrupt
	interruptFlag = k_setInterruptFlag(false);
	
	// If it's already locked, process below.
	if (k_testAndSet(&(spinlock->lockFlag), false, true) == false) {
		// If it's locked by itself (current core), increase lock count and return.
		if (spinlock->apicId == k_getApicId()) {
			spinlock->lockCount++;
			return;
		}
		
		// If it's locked by another task, wait until it will be unlocked.
		while (k_testAndSet(&(spinlock->lockFlag), false, true) == false) {
			/**
			  Spinlock do not do task switching here, but do retrying to get lock.
			  That's because spinlock can be used in interrupt handler,
			  and interrupt handler have to be processed quickly.
			*/
			
			// loop here to prevent memory bus from being locked by repeating k_testAndSet.
			while (spinlock->lockFlag == true) {
				k_pause();
			}
		}
	}
	
	// If it's unlocked, lock it.
	// already set <spinlock->lockFlag = true> in k_testAndSet using atomic operation.
	spinlock->lockCount = 1;
	spinlock->apicId = k_getApicId();
	spinlock->interruptFlag = interruptFlag;
}

void k_unlockSpin(Spinlock* spinlock) {
	bool interruptFlag;
	
	// disable interrupt
	interruptFlag = k_setInterruptFlag(false);
	
	// If it's already unlocked or it's locked by other cores, return.
	if ((spinlock->lockFlag == false) || (spinlock->apicId != k_getApicId())) {
		k_setInterruptFlag(interruptFlag);
		return;
	}
	
	// If it's locked more than twice, decrease lock count and return.
	if (spinlock->lockCount > 1) {
		spinlock->lockCount--;
		return;
	}
	
	// If it's locked once, unlock it.
	// back up interrupt flag before setting lock flag to false.
	interruptFlag = spinlock->interruptFlag;
	
	// Setting lock flag to false must be done at the last.
	spinlock->apicId = APICID_INVALID;
	spinlock->lockCount = 0;
	spinlock->interruptFlag = false;
	spinlock->lockFlag = false;
	
	k_setInterruptFlag(interruptFlag);
}

void k_lockAny(void* lock) {
	switch (*(byte*)lock) {
	case LOCK_TYPE_MUTEX:
		k_lock((Mutex*)lock);
		break;

	case LOCK_TYPE_SPINLOCK:
		k_lockSpin((Spinlock*)lock);
		break;

	default:
		break;
	}
}

void k_unlockAny(void* lock) {
	switch (*(byte*)lock) {
	case LOCK_TYPE_MUTEX:
		k_unlock((Mutex*)lock);
		break;

	case LOCK_TYPE_SPINLOCK:
		k_unlockSpin((Spinlock*)lock);
		break;

	default:
		break;
	}
}
