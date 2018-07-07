#include "sync.h"
#include "util.h"
#include "task.h"
#include "asm_util.h"

bool k_lockSystem(void) {
	return k_setInterruptFlag(false);
}

void k_unlockSystem(bool interruptFlag) {
	k_setInterruptFlag(interruptFlag);
}

void k_initMutex(Mutex* mutex) {
	mutex->lockFlag = false;
	mutex->lockCount = 0;
	mutex->taskId = TASK_INVALIDID;
}

void k_lock(Mutex* mutex) {

	// If it's already locked and it's locked by itself (current running task), increase lock count and return.
	if (k_testAndSet(&(mutex->lockFlag), false, true) == false) {
		// If it's locked by itself, increase lock count and return.
		if (mutex->taskId == k_getRunningTask()->link.id) {
			mutex->lockCount++;
			return;
		}

		// If it's locked by other tasks, wait until it will be unlocked.
		while (k_testAndSet(&(mutex->lockFlag), false, true) == false) {
			// hand processor over to other tasks while waiting in order to prevent unnecessary usage of processor.
			k_schedule();
		}
	}

	// If it's unlocked, lock it.
	// set <mutex->lockFlag = true> in k_testAndSet function using atomic operation.
	mutex->lockCount = 1;
	mutex->taskId = k_getRunningTask()->link.id;
}

void k_unlock(Mutex* mutex) {

	// If it's already unlocked or it's locked by other tasks, return.
	if ((mutex->lockFlag == false) || (mutex->taskId != k_getRunningTask()->link.id)) {
		return;
	}

	// If it's locked more than twice, decrease lock count and return.
	if (mutex->lockCount > 1) {
		mutex->lockCount--;
		return;
	}

	// If it's locked once, unlock it.
	// Setting lock flag to false must be done at the last.
	mutex->taskId = TASK_INVALIDID;
	mutex->lockCount = 0;
	mutex->lockFlag = false;
}
