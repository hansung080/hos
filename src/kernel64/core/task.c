#include "task.h"
#include "descriptors.h"
#include "../utils/util.h"
#include "asm_util.h"
#include "sync.h"
#include "console.h"
#include "multiprocessor.h"
#include "mp_config_table.h"
#include "window.h"
#include "dynamic_mem.h"

static TaskPoolManager g_taskPoolManager;
static Scheduler g_schedulers[MAXPROCESSORCOUNT];
static CommonScheduler g_commonScheduler;

static void k_initTaskPool(void) {
	int i;
	
	// initialize task pool manager.
	k_memset(&g_taskPoolManager, 0, sizeof(g_taskPoolManager));
	
	// initialize task pool.
	g_taskPoolManager.startAddr = (Task*)TASK_TASKPOOLSTARTADDRESS;
	k_memset((void*)TASK_TASKPOOLSTARTADDRESS, 0, sizeof(Task) * TASK_MAXCOUNT);
	
	// set task ID (offset) to tasks in pool.
	for(i = 0; i < TASK_MAXCOUNT; i++){
		g_taskPoolManager.startAddr[i].link.id = i;
	}
	
	// initialize max task count and allocated task count.
	g_taskPoolManager.maxCount = TASK_MAXCOUNT;
	g_taskPoolManager.allocatedCount = 1;
	
	// initialize spinlock of task pool manager.
	k_initSpinlock(&(g_taskPoolManager.spinlock));
}

static Task* k_allocTask(void) {
	Task* emptyTask;
	int i; // task offset
	
	k_lockSpin(&(g_taskPoolManager.spinlock));
	
	if (g_taskPoolManager.usedCount >= g_taskPoolManager.maxCount) {
		k_unlockSpin(&(g_taskPoolManager.spinlock));
		return null;
	}
	
	for (i = 0; i < g_taskPoolManager.maxCount; i++) {
		// If allocated task count (high 32 bits) of task ID == 0, it's not allocated.
		if ((g_taskPoolManager.startAddr[i].link.id >> 32) == 0) {
			emptyTask = &(g_taskPoolManager.startAddr[i]);
			break;
		}
	}
	
	// set not-0 to allocated task count (high 32 bits) of task ID in order to mark it allocated.
	// task ID consists of allocated task count (high 32 bits) and task offset (low 32 bits).
	emptyTask->link.id = (((qword)g_taskPoolManager.allocatedCount) << 32) | i;

	g_taskPoolManager.usedCount++;
	g_taskPoolManager.allocatedCount++;
	if (g_taskPoolManager.allocatedCount == 0) {
		g_taskPoolManager.allocatedCount = 1;
	}
	
	k_unlockSpin(&(g_taskPoolManager.spinlock));
	
	return emptyTask;
}

static void k_freeTask(qword taskId) {
	int i; // task offset
	
	// get task offset (low 32 bits) of task ID.
	i = GETTASKOFFSET(taskId);
	
	// initialize task context.
	k_memset(&(g_taskPoolManager.startAddr[i].context), 0, sizeof(Context));
	
	k_lockSpin(&(g_taskPoolManager.spinlock));
	
	// initialize task ID.
	// set 0 to allocated task count (high 32 bits) of task ID in order to mark it free.
	g_taskPoolManager.startAddr[i].link.id = i;
	
	g_taskPoolManager.usedCount--;
	
	k_unlockSpin(&(g_taskPoolManager.spinlock));
}

/**
  < Task Management >
  1. Create Task
    -------------------
    |        -------- |  -> Outer one is ready list.
    |P1->P2->|T3=>T4| |  -> Inner one is child thread list.
    |        -------- |
    -------------------
  
  2. End P2 Process
    ------------
    |T3->T4->P2|  -> end list
    ------------
  
  3. End T3 Thread
    ----
    |T3|  -> end list
    ----
*/

Task* k_createTask(qword flags, void* memAddr, qword memSize, qword entryPointAddr, byte affinity) {
	Task* task;    // task to create (task means process or thread)
	Task* process; // process with running task in it (It means process which has created the task, or it means parent process.)
	void* stackAddr;
	byte currentApicId;
	
	currentApicId = k_getApicId();
	
	// allocate task.
	task = k_allocTask();
	if (task == null) {
		return null;
	}
	
	// allocate stack.
	stackAddr = k_allocMem(TASK_STACKSIZE);
	if (stackAddr == null) {
		k_freeTask(task->link.id);
		return null;
	}

	k_lockSpin(&(g_schedulers[currentApicId].spinlock));
	
	// get process with running task in it.
	process = k_getProcessByThread(k_getRunningTask(currentApicId));
	if (process == null) {
		k_freeMem(stackAddr);
		k_freeTask(task->link.id);
		k_unlockSpin(&(g_schedulers[currentApicId].spinlock));
		return null;
	}
		
	// If creating thread.
	if (flags & TASK_FLAGS_THREAD) {
		
		// set ID of process which has created this thread to [thread.parent process ID],
		// and set memory area of process which has created this thread to [thread.memory area].
		task->parentProcessId = process->link.id;
		task->memAddr = process->memAddr;
		task->memSize = process->memSize;
		
		// add the created thread to [parent process.child thread list].
		k_addListToTail(&(process->childThreadList), &(task->threadLink));
		
	// If creating process.
	} else {
		// set ID of process which has created this process to [process.parent process ID],
		// and set parameter memory area to [process.memory area]
		task->parentProcessId = process->link.id;
		task->memAddr = memAddr;
		task->memSize = memSize;
	}
	
	// set thread ID equaled to task ID.
	task->threadLink.id = task->link.id;
	
	k_unlockSpin(&(g_schedulers[currentApicId].spinlock));
		
	// set task.
	k_setTask(task, flags, entryPointAddr, stackAddr, TASK_STACKSIZE);
	
	// initialize child thread list.
	k_initList(&(task->childThreadList));
	
	// initialize FPU used flag, APIC ID, and affinity.
	task->fpuUsed = false;
	task->apicId = currentApicId;
	task->affinity = affinity;
	task->waitGroupId = 0;
	task->joinGroupId = 0;
	task->joinCount = 0;
	
	// add task to scheduler with load balancing.
	k_addTaskToSchedulerWithLoadBalancing(task);
	
	return task;
}

static void k_setTask(Task* task, qword flags, qword entryPointAddr, void* stackAddr, qword stackSize) {
	
	// initialize context
	k_memset(task->context.registers, 0, sizeof(task->context.registers));
	
	// set RSP, RBP.
	task->context.registers[TASK_INDEX_RSP] = (qword)stackAddr + stackSize - 8;
	task->context.registers[TASK_INDEX_RBP] = (qword)stackAddr + stackSize - 8;
	
	// push the address of k_exitTask function to the top (8 bytes) of stack as the return address,
	// in order to move to k_exitTask function when entry point function of task returns.
	*(qword*)((qword)stackAddr + stackSize - 8) = (qword)k_exitTask;
	
	// set segment selector.
	if (task->flags & TASK_FLAGS_USER) {
		task->context.registers[TASK_INDEX_CS] = GDT_OFFSET_USERCODESEGMENT | SELECTOR_RPL3;
		task->context.registers[TASK_INDEX_DS] = GDT_OFFSET_USERDATASEGMENT | SELECTOR_RPL3;
		task->context.registers[TASK_INDEX_ES] = GDT_OFFSET_USERDATASEGMENT | SELECTOR_RPL3;
		task->context.registers[TASK_INDEX_FS] = GDT_OFFSET_USERDATASEGMENT | SELECTOR_RPL3;
		task->context.registers[TASK_INDEX_GS] = GDT_OFFSET_USERDATASEGMENT | SELECTOR_RPL3;
		task->context.registers[TASK_INDEX_SS] = GDT_OFFSET_USERDATASEGMENT | SELECTOR_RPL3;

	} else {
		task->context.registers[TASK_INDEX_CS] = GDT_OFFSET_KERNELCODESEGMENT | SELECTOR_RPL0;
		task->context.registers[TASK_INDEX_DS] = GDT_OFFSET_KERNELDATASEGMENT | SELECTOR_RPL0;
		task->context.registers[TASK_INDEX_ES] = GDT_OFFSET_KERNELDATASEGMENT | SELECTOR_RPL0;
		task->context.registers[TASK_INDEX_FS] = GDT_OFFSET_KERNELDATASEGMENT | SELECTOR_RPL0;
		task->context.registers[TASK_INDEX_GS] = GDT_OFFSET_KERNELDATASEGMENT | SELECTOR_RPL0;
		task->context.registers[TASK_INDEX_SS] = GDT_OFFSET_KERNELDATASEGMENT | SELECTOR_RPL0;
	}
	
	// set RIP
	task->context.registers[TASK_INDEX_RIP] = entryPointAddr;

	// set RFLAGS
	// - set IF (bit 9) to 1 in order to enable interrupt.
	// - set IOPL (bit 12~13) to 3 (Ring 3) in oder to allow user applications to access IO port.
	//   [Ref] IOPL: input/output privilege level
	task->context.registers[TASK_INDEX_RFLAGS] |= 0x3200;
	

	// set etc
	task->stackAddr = stackAddr;
	task->stackSize = stackSize;
	task->flags = flags;
}

void k_initScheduler(void) {
	int i, j;
	byte currentApicId;
	Task* task;
	
	currentApicId = k_getApicId();
	
	// initialize task pool and scheduler only If current core is BSP.
	if (currentApicId == APICID_BSP) {
		// initialize task pool.
		k_initTaskPool();
		
		for (i = 0; i < MAXPROCESSORCOUNT; i++ ) {
			for (j = 0; j < TASK_MAXREADYLISTCOUNT; j++) {
				// initialize ready lists.
				k_initList(&(g_schedulers[i].readyLists[j]));
				
				// initialize executed task counts by task priority.
				g_schedulers[i].executedCounts[j] = 0;
			}
			
			// initialize end list.
			k_initList(&(g_schedulers[i].endList));
			
			// initialize spinlock.
			k_initSpinlock(&(g_schedulers[i].spinlock));
		}
	}
	
	// allocate task and set it as a running task. (This task is for the booting task.)
	// The booting task is the first task of kernel.
	task = k_allocTask();
	g_schedulers[currentApicId].runningTask = task;
	
	// The shell task of BSP and the idle task of AP have to be running only on current core.
	// They must not move to another core. Thus, their affinity is set to current APIC ID.
	task->apicId = currentApicId;
	task->affinity = currentApicId;
	task->waitGroupId = 0;
	task->joinGroupId = 0;
	task->joinCount = 0;
	
	// If current core is BSP, the booting task will become the shell task in text mode or the window manager task in graphic mode.
	// (The idle task of BSP will be created in k_main function.)
	if (currentApicId == APICID_BSP) {
		task->flags = TASK_PRIORITY_HIGHEST | TASK_FLAGS_SYSTEM | TASK_FLAGS_PROCESS;
		
	// If current core is AP, the booting task will become the idle task.
	} else {
		task->flags = TASK_PRIORITY_LOWEST | TASK_FLAGS_SYSTEM | TASK_FLAGS_PROCESS | TASK_FLAGS_IDLE;
	}
	
	// set task itself ID to parent process ID, because the first task of kernel does not have parent process.
	task->parentProcessId = task->link.id;
	task->memAddr = (void*)0x100000;
	task->memSize = 0x500000;
	task->stackAddr = (void*)0x600000;
	task->stackSize = 0x100000;
	
	// initialize fields related with processor load.
	g_schedulers[currentApicId].processorTimeInIdleTask = 0;
	g_schedulers[currentApicId].processorLoad = 0;
	
	// initialize last FPU-used task ID.
	g_schedulers[currentApicId].lastFpuUsedTaskId = TASK_INVALIDID;
}

void k_setRunningTask(byte apicId, Task* task) {
	k_lockSpin(&(g_schedulers[apicId].spinlock));
	
	g_schedulers[apicId].runningTask = task;
	
	k_unlockSpin(&(g_schedulers[apicId].spinlock));
}

Task* k_getRunningTask(byte apicId) {
	Task* runningTask;
	
	k_lockSpin(&(g_schedulers[apicId].spinlock));
	
	runningTask = g_schedulers[apicId].runningTask;
	
	k_unlockSpin(&(g_schedulers[apicId].spinlock));
	
	return runningTask;
}

static Task* k_getNextTaskToRun(byte apicId) {
	Task* target = null;
	int taskCount;
	int i, j;
	
	// If task is null, loop once more.
	// It's because tasks can't be selected even though they exist in ready lists,
	// when all tasks in ready lists are executed once and hand processor over to other tasks.
	for (j = 0; j < 2; j++) {
		// select next running task in searching tasks from the highest ready list to lowest ready list.
		for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
			// get task count by priority.
			taskCount = k_getListCount(&(g_schedulers[apicId].readyLists[i]));
			
			// If executed task count < task count, select task with current priority.
			if (g_schedulers[apicId].executedCounts[i] < taskCount) {
				target = (Task*)k_removeListFromHead(&(g_schedulers[apicId].readyLists[i]));
				g_schedulers[apicId].executedCounts[i]++;
				break;
				
			// If executed task count >= task count, select task with next priority.
			} else {
				g_schedulers[apicId].executedCounts[i] = 0;
			}
		}
		
		// If task is not null, break.
		if (target != null) {
			break;
		}
	}
	
	return target;
}

static bool k_addTaskToReadyList(byte apicId, Task* task) {
	byte priority;
	
	priority = GETTASKPRIORITY(task->flags);
	if (task->flags & TASK_FLAGS_WAIT) {
		k_addTaskToWaitList(task);
		return true;

	} else if (task->flags & TASK_FLAGS_END) {
		k_addListToTail(&(g_schedulers[apicId].endList), task);
		return true;
		
	} else if (priority >= TASK_MAXREADYLISTCOUNT) {
		return false;
	}
	
	k_addListToTail(&(g_schedulers[apicId].readyLists[priority]), task);
	return true;
}

static Task* k_removeTaskFromReadyList(byte apicId, qword taskId) {
	Task* target;
	byte priority;
	
	// check if task ID is valid.
	if (GETTASKOFFSET(taskId) >= TASK_MAXCOUNT) {
		return null;
	}
	
	// check if task ID matches task ID which is searched from task pool.
	target = &(g_taskPoolManager.startAddr[GETTASKOFFSET(taskId)]);
	if (target->link.id != taskId) {
		return null;
	}
	
	priority = GETTASKPRIORITY(target->flags);
	if (priority >= TASK_MAXREADYLISTCOUNT) {
		return null;
	}
	
	// remove task from ready list with the priority.
	target = k_removeListById(&(g_schedulers[apicId].readyLists[priority]), taskId);
	
	return target;
}

static Task* k_getProcessByThread(Task* thread) {
	Task* process;
	
	// If parameter is process, return process itself.
	if (thread->flags & TASK_FLAGS_PROCESS) {
		return thread;
	}
	
	// If parameter is thread, return parent process.
	process = k_getTaskFromPool(GETTASKOFFSET(thread->parentProcessId));
	if ((process == null) || (process->link.id != thread->parentProcessId)) {
		return null;
	}
	
	return process;
}

static bool k_findSchedulerByTaskWithLock(qword taskId, byte* apicId) {
	Task* target;
	byte targetApicId;
	
	while (true) {
		target = &(g_taskPoolManager.startAddr[GETTASKOFFSET(taskId)]);
		if (target == null || target->link.id != taskId) {
			return false;
		}
		
		targetApicId = target->apicId;
		
		k_lockSpin(&(g_schedulers[targetApicId].spinlock));
		
		// If APIC ID has not changed (task has not moved to another core) while getting a lock,
		// got a right lock, so break the loop.
		target = &(g_taskPoolManager.startAddr[GETTASKOFFSET(taskId)]);
		if (target != null && target->apicId == targetApicId) {
			break;
		}
		
		// If APIC ID has changed (task has moved to another core) while getting a lock,
		// got a wrong lock, so return a wrong lock and continue the loop to get a lock again.
		k_unlockSpin(&(g_schedulers[targetApicId].spinlock));
	}
	
	*apicId = targetApicId;
	
	return true;
}

static byte k_findSchedulerByMinTaskCount(const Task* task) {
	byte priority;
	byte i;
	int currentTaskCount;
	int minTaskCount;
	byte minCoreIndex;
	int tempTaskCount;
	int coreCount;
	
	coreCount = k_getProcessorCount();
	if (coreCount == 1) {
		return task->apicId;
	}
	
	priority = GETTASKPRIORITY(task->flags);
	currentTaskCount = k_getListCount(&(g_schedulers[task->apicId].readyLists[priority]));
	
	/**
	  find scheduler with minimum task count out of schedulers whose task count is '2 or more' less than current task count.
	  (task count is only from ready list with the same priority.)
	*/
	minTaskCount = TASK_MAXCOUNT;
	minCoreIndex = task->apicId;
	for (i = 0; i < coreCount; i++) {
		if (i == task->apicId) {
			continue;
		}
		
		tempTaskCount = k_getListCount(&(g_schedulers[i].readyLists[priority]));
		if ((tempTaskCount + 2 <= currentTaskCount) && (tempTaskCount < minTaskCount)) {
			minCoreIndex = i;
			minTaskCount = tempTaskCount;
		}
	}
	
	return minCoreIndex;
}

static void k_addTaskToSchedulerWithLoadBalancing(Task* task) {
	byte currentApicId;
	byte targetApicId;
	
	currentApicId = task->apicId;
	
	/* find target scheduler */
	if ((g_schedulers[currentApicId].loadBalancing == true) && (task->affinity == TASK_AFFINITY_LOADBALANCING)) {
		targetApicId = k_findSchedulerByMinTaskCount(task);
		
	} else if ((task->affinity != currentApicId) && (task->affinity != TASK_AFFINITY_LOADBALANCING)) {
		targetApicId = task->affinity;
		
	} else {
		targetApicId = currentApicId;
	}
	
	/* save FPU context if it requires */
	k_lockSpin(&(g_schedulers[currentApicId].spinlock));
	
	// save FPU context to memory if task moves to another scheduler and it's the last FPU-used task.
	if ((currentApicId != targetApicId) && (task->link.id == g_schedulers[currentApicId].lastFpuUsedTaskId)) {
		// clear TS in order not to cause exception 7 (Device Not Available).
		// Otherwise, exception 7 occurs when FPU operation or FPU command (fxsave) is being executed, with TS being set to 1.
		k_clearTs();
		k_saveFpuContext(task->fpuContext);
		g_schedulers[currentApicId].lastFpuUsedTaskId = TASK_INVALIDID;
	}
	
	k_unlockSpin(&(g_schedulers[currentApicId].spinlock));
	
	/* add task to target scheduler */
	k_lockSpin(&(g_schedulers[targetApicId].spinlock));
	
	task->apicId = targetApicId;
	k_addTaskToReadyList(targetApicId, task);
	
	k_unlockSpin(&(g_schedulers[targetApicId].spinlock));
}

static void k_addTaskToWaitList(Task* task) {
	k_lockSpin(&g_commonScheduler.spinlock);

	k_addListToTail(&g_commonScheduler.waitList, task);

	k_unlockSpin(&g_commonScheduler.spinlock);
}

static Task* k_removeTaskFromWaitList(qword taskId) {
	Task* task;

	k_lockSpin(&g_commonScheduler.spinlock);

	task = k_removeListById(&g_commonScheduler.waitList, taskId);

	k_unlockSpin(&g_commonScheduler.spinlock);

	return task;
}

/**
  < Context Switching >
  [REF] - interrupt switching: context switching between task and interrupt handler.
        - task switching: context switching between task and task.
  
  1. interrupt switching: It occurs when interrupts occur.
  
     registers  processor/ISR   IST
     -----         --->        -----
     | A |                     | A |
     -----         <---        -----
     
     - process task A.
     - save task A context from registers to IST by processor and ISR.
     - process interrupt handler.
     - restore task A context from IST to registers by processor and ISR.
     - process task A.
     
  2. task switching: It occurs when task requests such as waiting, sleeping, and etc.
  
     registers    k_schedule   task pool
     ----------     --->       -------
     | A -> B |                | ... |
     ----------     <---       |  A  |
                               |  B  |
                               | ... |
                               -------
     - process task A.
     - save task A context from registers to task pool by k_schedule.
     - restore task B context from task pool to registers by k_schedule.
     - process task B.
  
  3. interrupt switching and task switching: It occurs when timer interrupt occurs.
  
     registers    processor/ISR     IST      k_scheduleInInterrupt   task pool
     ----------      --->       ----------          --->             -------
     | A -> B |                 | A -> B |                           | ... |
     ----------      <---       ----------          <---             |  A  |
                                                                     |  B  |
                                                                     | ... |
                                                                     -------
     - process task A.
     - save task A context from registers to IST by processor and ISR.
     - process timer interrupt handler.
     - save task A context from IST to task pool by k_scheduleInInterrupt.
     - restore task B context from task pool to IST by k_scheduleInInterrupt.
     - restore task B context from IST to registers by processor and ISR.
     - process task B.
  
  
  < Scheduling in Multi-core Processor >
  - There are a IST and a scheduler per a core.
  - process task load balancing when a task is added to ready list.
  
       core 0     processor/ISR    IST 0     k_scheduleInInterrupt   task pool
     ----------      --->       ----------          --->             -------
     | A -> B |                 | A -> B |                           | ... |
     ----------      <---       ----------          <---             |  A  |
                                                                     |  B  |
       core 1                      IST 1                             |  C  |
     ----------      --->       ----------          --->             |  D  |
     | C -> D |                 | C -> D |                           |  E  |
     ----------      <---       ----------          <---             |  F  |
                                                                     |  G  |
       core 2                      IST 2                             |  H  |
     ----------      --->       ----------          --->             |  I  |
     | F -> G |                 | F -> G |                           |  J  |
     ----------      <---       ----------          <---             | ... |
                                                                     -------
     
                   scheduler 0
     running task                    ready lists: task 1 -> 2
     ----------      <---       -----------------------------
     | A -> B |                 |         ...               |
     ----------      --->       | B (out) | A (in) | F (in) |
                                |         ...               |
                                -----------------------------
     
                   scheduler 1
     running task                    ready lists: task 2 -> 2
     ----------      <---       ------------------------
     | C -> D |                 |         ...          |
     ----------      --->       | D (out) | E | C (in) |
                                |         ...          |
                                ------------------------
     
                   scheduler 2
     running task                    ready lists: task 4 -> 3
     ----------      <---       -----------------------
     | F -> G |                 |         ...         |
     ----------      --->       | G (out) | H | I | J |
                to scheduler 0  |         ...         |
                                -----------------------
*/

bool k_schedule(void) {
	Task* runningTask, * nextTask;
	bool interruptFlag;
	byte currentApicId;
	
	// disable interrupt in order to prevent task switching from occuring again while processing task switching.
	interruptFlag = k_setInterruptFlag(false);
	
	currentApicId = k_getApicId();
	
	if (k_getReadyTaskCount(currentApicId) < 1) {
		k_setInterruptFlag(interruptFlag);
		return false;
	}
	
	k_lockSpin(&(g_schedulers[currentApicId].spinlock));
	
	nextTask = k_getNextTaskToRun(currentApicId);
	if (nextTask == null) {
		k_unlockSpin(&(g_schedulers[currentApicId].spinlock));
		k_setInterruptFlag(interruptFlag);
		return false;
	}
	
	runningTask = g_schedulers[currentApicId].runningTask;
	g_schedulers[currentApicId].runningTask = nextTask;
	
	// If it's switched from idle task, increase processor time used by idle task.
	if (runningTask->flags & TASK_FLAGS_IDLE) {
		g_schedulers[currentApicId].processorTimeInIdleTask += (TASK_PROCESSORTIME - g_schedulers[currentApicId].processorTime);
	}
	
	// If next task is not last FPU-used task, set CR0.TS=1.
	if (g_schedulers[currentApicId].lastFpuUsedTaskId != nextTask->link.id) {
		k_setTs();
		
	} else {
		k_clearTs();
	}
	
	/**
	  < Context Switching in Task >
	  - save context: registers -> running task (by k_switchContext)
	  - restore context: next task -> registers (by k_switchContext)
	 */
	
	// If it's switched from wait task, move the task to wait list, and switch context.
	if (runningTask->flags & TASK_FLAGS_WAIT) {
		k_unlockSpin(&(g_schedulers[currentApicId].spinlock));
		k_addTaskToWaitList(runningTask);
		// save running task context from registers to task pool,
		// and restore next task context from task pool to registers.
		k_switchContext(&(runningTask->context), &(nextTask->context));

	// If it's switched from end task, move the task to end list, and switch context.
	} else if (runningTask->flags & TASK_FLAGS_END) {
		k_addListToTail(&(g_schedulers[currentApicId].endList), runningTask);
		k_unlockSpin(&(g_schedulers[currentApicId].spinlock));
		// restore next task context from task pool to registers.
		k_switchContext(null, &(nextTask->context));
		
	// If it's switched from normal task, move the task to ready list, and switch context.
	} else {
		k_addTaskToReadyList(currentApicId, runningTask);
		k_unlockSpin(&(g_schedulers[currentApicId].spinlock));
		// save running task context from registers to task pool,
		// and restore next task context from task pool to registers.
		k_switchContext(&(runningTask->context), &(nextTask->context));
	}
	
	// update processor time.
	g_schedulers[currentApicId].processorTime = TASK_PROCESSORTIME;
	
	k_setInterruptFlag(interruptFlag);
	
	return true;
}

bool k_scheduleInInterrupt(void) {
	Task* runningTask, * nextTask;
	char* contextAddr; // context address in IST
	byte currentApicId;
	qword istEndAddr; // IST end address of current core
	
	currentApicId = k_getApicId();
	
	k_lockSpin(&(g_schedulers[currentApicId].spinlock));
	
	nextTask = k_getNextTaskToRun(currentApicId);
	if (nextTask == null) {
		k_unlockSpin(&(g_schedulers[currentApicId].spinlock));
		return false;
	}
	
	// calculate context address from IST end address of current core.
	istEndAddr = IST_STARTADDRESS + IST_SIZE - (IST_SIZE / MAXPROCESSORCOUNT * currentApicId);
	contextAddr = (char*)istEndAddr - sizeof(Context);
	
	runningTask = g_schedulers[currentApicId].runningTask;
	g_schedulers[currentApicId].runningTask = nextTask;
	
	// If it's switched from idle task, increase processor time used by idle task.
	if (runningTask->flags & TASK_FLAGS_IDLE) {
		g_schedulers[currentApicId].processorTimeInIdleTask += TASK_PROCESSORTIME;
	}
	
	/**
	  < Context Switching in Interrupt Handler >
	  - save context: registers -> IST (by processor and ISR) -> running task (by k_memcpy)
	  - restore context: next task -> IST (by k_memcpy) -> registers (by processor and ISR)
	 */
	
	// If it's switched from wait task, move the task to wait list, and switch context.
	if (runningTask->flags & TASK_FLAGS_WAIT) {
		// save running task context from IST to task pool.
		k_memcpy(&(runningTask->context), contextAddr, sizeof(Context));
		k_addTaskToWaitList(runningTask);

	// If it's switched from end task, move the task to end list, and switch context.
	} else if (runningTask->flags & TASK_FLAGS_END) {
		k_addListToTail(&(g_schedulers[currentApicId].endList), runningTask);
		
	// If it's switched from normal task, move the task to ready list, and switch context.
	} else {
		// save running task context from IST to task pool.
		k_memcpy(&(runningTask->context), contextAddr, sizeof(Context));
	}
	
	// If next task is not last FPU-used task, set CR0.TS=1.
	if (g_schedulers[currentApicId].lastFpuUsedTaskId != nextTask->link.id) {
		k_setTs();
		
	} else {
		k_clearTs();
	}
	
	k_unlockSpin(&(g_schedulers[currentApicId].spinlock));
	
	// restore next task context from task pool to IST.
	k_memcpy(contextAddr, &(nextTask->context), sizeof(Context));
	
	if (((runningTask->flags & TASK_FLAGS_WAIT) != TASK_FLAGS_WAIT) && 
		((runningTask->flags & TASK_FLAGS_END) != TASK_FLAGS_END)) {
		k_addTaskToSchedulerWithLoadBalancing(runningTask);
	}
	
	// update processor time.
	g_schedulers[currentApicId].processorTime = TASK_PROCESSORTIME;
	
	return true;
}

void k_decreaseProcessorTime(byte apicId) {
	g_schedulers[apicId].processorTime--;
}

bool k_isProcessorTimeExpired(byte apicId) {
	if (g_schedulers[apicId].processorTime <= 0) {
		return true;
	}
	
	return false;
}

bool k_changeTaskPriority(qword taskId, byte priority) {
	byte apicId;
	Task* target;
	
	if (priority >= TASK_MAXREADYLISTCOUNT) {
		return false;
	}
	
	if (k_findSchedulerByTaskWithLock(taskId, &apicId) == false) {
		return false;
	}
	
	target = g_schedulers[apicId].runningTask;
	
	// If it's a running task, only change priority.
	// It's because the task will move to ready list with the changed priority, when timer interrupt (IRQ 0) causes task switching.
	if (target->link.id == taskId) {
		SETTASKPRIORITY(target->flags, priority);
		
	// If it's not a running task, remove it from ready list, change priority, and move it to ready list with the changed priority.
	} else {
		target = k_removeTaskFromReadyList(apicId, taskId);
		
		// If the task doesn't exist in ready list, change priority from task pool.
		if (target == null) {
			target = k_getTaskFromPool(GETTASKOFFSET(taskId));
			if (target != null) {
				SETTASKPRIORITY(target->flags, priority);
			}
			
		// If the task exists in ready list, change priority and move it to ready list with the changed priority.
		} else {
			SETTASKPRIORITY(target->flags, priority);
			k_addTaskToReadyList(apicId, target);
		}
	}
	
	k_unlockSpin(&(g_schedulers[apicId].spinlock));
	
	return true;
}

bool k_changeTaskAffinity(qword taskId, byte affinity) {
	byte apicId;
	Task* target;
	
	if (k_findSchedulerByTaskWithLock(taskId, &apicId) == false) {
		return false;
	}
	
	target = g_schedulers[apicId].runningTask;
	
	// If it's a running task, only change affinity.
	// It's because the task will move to scheduler with the changed affinity, when timer interrupt (IRQ 0) causes task switching.
	if (target->link.id == taskId) {
		target->affinity = affinity;
		k_unlockSpin(&(g_schedulers[apicId].spinlock));
		
	// If it's not a running task, remove it from ready list, change affinity, and move it to scheduler with the changed affinity.
	} else {
		target = k_removeTaskFromReadyList(apicId, taskId);
		
		// If the task doesn't exist in ready list, change affinity from task pool.
		if (target == null) {
			target = k_getTaskFromPool(GETTASKOFFSET(taskId));
			if (target != null) {
				target->affinity = affinity;
			}
			
		// If the task exists in ready list, change affinity.
		} else {
			target->affinity = affinity;
		}
		
		k_unlockSpin(&(g_schedulers[apicId].spinlock));
		
		// move the task to scheduler with the changed affinity.
		k_addTaskToSchedulerWithLoadBalancing(target);
	}
	
	return true;
}

bool k_waitTask(qword taskId) {
	byte apicId;
	Task* target;

	if (k_findSchedulerByTaskWithLock(taskId, &apicId) == false) {
		return false;
	}
	
	target = g_schedulers[apicId].runningTask;
	
	// If it's a running task, set wait task flag, and switch task.
	if (target->link.id == taskId) {
		target->flags |= TASK_FLAGS_WAIT;
		k_unlockSpin(&(g_schedulers[apicId].spinlock));
		
		// check if the task is running in current scheduler.
		if (k_getApicId() == apicId) {
			// switch task.
			k_schedule();			
		}
		
		return true;
	}
	
	// If it's not a running task, remove it from ready list, set wait task flag, move it to wait list.
	target = k_removeTaskFromReadyList(apicId, taskId);
	if (target == null) {
		target = k_getTaskFromPool(GETTASKOFFSET(taskId));
		if (target != null) {
			target->flags |= TASK_FLAGS_WAIT;
		}
		
		k_unlockSpin(&(g_schedulers[apicId].spinlock));

		return true;
	}
	
	target->flags |= TASK_FLAGS_WAIT;
	k_unlockSpin(&(g_schedulers[apicId].spinlock));
	k_addTaskToWaitList(target);
	
	return true;
}

bool k_notifyTask(qword taskId) {
	byte apicId;
	Task* target;

	if (k_findSchedulerByTaskWithLock(taskId, &apicId) == false) {
		return false;
	}
		
	target = g_schedulers[apicId].runningTask;
	
	// If it's a running task, clear wait task flag.
	if (target->link.id == taskId) {
		target->flags &= ~TASK_FLAGS_WAIT;
		k_unlockSpin(&(g_schedulers[apicId].spinlock));

		return true;
	}
	
	// If it's not a running task, remove it from wait list, clear wait task flag, move it to ready list.
	target = k_removeTaskFromWaitList(taskId);
	if (target == null) {
		target = k_getTaskFromPool(GETTASKOFFSET(taskId));
		if (target != null) {
			target->flags &= ~TASK_FLAGS_WAIT;
		}
		
		k_unlockSpin(&(g_schedulers[apicId].spinlock));
		
		return true;
	}
	
	target->flags &= ~TASK_FLAGS_WAIT;
	k_addTaskToReadyList(apicId, target);
	k_unlockSpin(&(g_schedulers[apicId].spinlock));
	
	return true;
}

void k_printWaitTaskInfo(void) {
	Task* task;
	int count = 0;
	
	k_printf("*** Wait Task Info ***\n");

	k_lockSpin(&g_commonScheduler.spinlock);

	task = k_getHeadFromList(&g_commonScheduler.waitList);
	while (task != null) {
		count++;
		k_printf("[wait task %d] core %d, task 0x%q, wait group 0x%q, join group 0x%q, join count %d\n", count, task->apicId, task->link.id, task->waitGroupId, task->joinGroupId, task->joinCount);
		task = k_getNextFromList(&g_commonScheduler.waitList, task);
	}

	k_unlockSpin(&g_commonScheduler.spinlock);

	k_printf("-> wait task count: %d\n", count);
}

bool k_endTask(qword taskId) {
	Task* target;
	byte apicId;

	if (k_findSchedulerByTaskWithLock(taskId, &apicId) == false) {
		return false;
	}
	
	target = g_schedulers[apicId].runningTask;
	
	// If it's a running task, set end task flag, and switch task.
	if (target->link.id == taskId) {
		if (target->flags & TASK_FLAGS_JOIN) {
			k_notifyOneInJoinGroup(target->joinGroupId);
		}

		target->flags |= TASK_FLAGS_END;		
		k_unlockSpin(&(g_schedulers[apicId].spinlock));
		
		// check if the task is running in current scheduler.
		if (k_getApicId() == apicId) {
			// switch task.
			k_schedule();
			
			// This infinite loop will be never executed, 
			// because task switching has already done before and the task will be never running again.
			while (true) {
				;
			}
		}
		
		return true;
	}
	
	// If it's not a running task, remove it from ready list or wait list, set end task flag, move it to end list.
	target = k_removeTaskFromReadyList(apicId, taskId);
	if (target == null) {
		target = k_removeTaskFromWaitList(taskId);
	}

	if (target == null) {
		target = k_getTaskFromPool(GETTASKOFFSET(taskId));
		if (target != null) {
			if (target->flags & TASK_FLAGS_JOIN) {
				k_notifyOneInJoinGroup(target->joinGroupId);
			}

			target->flags |= TASK_FLAGS_END;
		}
		
		k_unlockSpin(&(g_schedulers[apicId].spinlock));
		
		return true;
	}

	if (target->flags & TASK_FLAGS_JOIN) {
		k_notifyOneInJoinGroup(target->joinGroupId);
	}
	
	target->flags |= TASK_FLAGS_END;
	k_addListToTail(&(g_schedulers[apicId].endList), target);
	k_unlockSpin(&(g_schedulers[apicId].spinlock));
	
	return true;
}

void k_exitTask(void) {
	k_endTask(k_getRunningTask(k_getApicId())->link.id);
}

int k_getReadyTaskCount(byte apicId) {
	int totalCount = 0;
	int i;
	
	k_lockSpin(&(g_schedulers[apicId].spinlock));
	
	for (i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
		totalCount += k_getListCount(&(g_schedulers[apicId].readyLists[i]));
	}
	
	k_unlockSpin(&(g_schedulers[apicId].spinlock));
	
	return totalCount;
}

int k_getTaskCount(byte apicId) {
	int totalCount;
	
	totalCount = k_getReadyTaskCount(apicId);
	
	k_lockSpin(&(g_schedulers[apicId].spinlock));
	
	// total task count = ready task count + wait task count + running task count
	totalCount += k_getListCount(&(g_schedulers[apicId].endList)) + 1;
	
	k_unlockSpin(&(g_schedulers[apicId].spinlock));
	
	return totalCount;
}

Task* k_getTaskFromPool(int offset) {
	if ((offset < 0) || (offset >= TASK_MAXCOUNT)) {
		return null;
	}
	
	return &(g_taskPoolManager.startAddr[offset]);
}

bool k_existTask(qword taskId) {
	Task* task;
	
	task = k_getTaskFromPool(GETTASKOFFSET(taskId));
	if ((task == null) || (task->link.id != taskId)) {
		return false;
	}
	
	return true;
}

qword k_getProcessorLoad(byte apicId) {
	return g_schedulers[apicId].processorLoad;
}

void k_setTaskLoadBalancing(byte apicId, bool loadBalancing) {
	g_schedulers[apicId].loadBalancing = loadBalancing;
}

void k_idleTask(void) {
	Task* task, * childThread, * process;
	qword lastMeasureTickCount, lastSpendTickInIdleTask;
	qword currentMeasureTickCount, currentSpendTickInIdleTask;
	qword taskId, childThreadId;
	int i, count;
	void* threadLink;
	byte currentApicId;
	byte processApicId;
	
	currentApicId = k_getApicId();
	
	lastSpendTickInIdleTask = g_schedulers[currentApicId].processorTimeInIdleTask;
	lastMeasureTickCount = k_getTickCount();
	
	// infinite loop
	while (true) {
		/* 1. calculate processor load */
		
		currentMeasureTickCount = k_getTickCount();
		currentSpendTickInIdleTask = g_schedulers[currentApicId].processorTimeInIdleTask;
		
		// processor load (%) = 100 - (processor time used by idle task * 100 / processor time used by whole system)
		if ((currentMeasureTickCount - lastMeasureTickCount) == 0) {
			g_schedulers[currentApicId].processorLoad = 0;
			
		} else {
			g_schedulers[currentApicId].processorLoad = 100 - ((currentSpendTickInIdleTask - lastSpendTickInIdleTask) * 100 / (currentMeasureTickCount - lastMeasureTickCount));
		}
		
		lastMeasureTickCount = currentMeasureTickCount;
		lastSpendTickInIdleTask = currentSpendTickInIdleTask;
		
		/* 2. halt processor by processor load */
		
		// halt processor by processor load.
		k_haltProcessorByLoad(currentApicId);
		
		/* 3. completely end the end tasks in end list */
		
		// If end task exists in end list, remove end task from end list, free memory of end task.
		if (k_getListCount(&(g_schedulers[currentApicId].endList)) > 0) {
			while (true) {
				k_lockSpin(&(g_schedulers[currentApicId].spinlock));
				
				// remove end task from end list.
				task = k_removeListFromHead(&(g_schedulers[currentApicId].endList));
				
				k_unlockSpin(&(g_schedulers[currentApicId].spinlock));
				
				if (task == null) {
					break;
				}
				
				/**
				  [Code Block 1] If end task is process, end all child threads of process which means to move them from ready list to end list,
				                 wait until child threads in end list are completely ended, and finally completely end process itself.
				                 (End means to move tasks from ready list to end list.)
				                 (Totally end means to free memory of tasks in end list.)
				  [REF] free memory of process: free code/data area, task, stack.
				        free memory of thread: free task, stack.
				*/
				if (task->flags & TASK_FLAGS_PROCESS) {
					count = k_getListCount(&(task->childThreadList));
					for (i = 0; i < count; i++) {
						k_lockSpin(&(g_schedulers[currentApicId].spinlock));
						
						threadLink = k_removeListFromHead(&(task->childThreadList));
						if (threadLink == null) {
							k_unlockSpin(&(g_schedulers[currentApicId].spinlock));
							break;
						}
						
						childThread = GETTASKFROMTHREADLINK(threadLink);
						
						// the reasons why re-put child thread to child thread list, after removing it from child thread list.
						// - first reason: When thread ends, it removes itself from child thread list in [Code Block 2].
						// - second reason: Under multi-core processor, If child thread is running on the other cores, it can't be moved to end list immediately.
						k_addListToTail(&(task->childThreadList), &(childThread->threadLink));
						
						childThreadId = childThread->link.id;
						
						k_unlockSpin(&(g_schedulers[currentApicId].spinlock));
						
						// end all child threads (move them from ready list to end list.)
						k_endTask(childThreadId);
					}
					
					// If child thread remains, it waits until all child thread will be completely ended by idle task.
					if (k_getListCount(&(task->childThreadList)) > 0) {
						k_lockSpin(&(g_schedulers[currentApicId].spinlock));
						
						k_addListToTail(&(g_schedulers[currentApicId].endList), task);
						
						k_unlockSpin(&(g_schedulers[currentApicId].spinlock));
						
						continue;
						
					// If all child threads are completely ended, end process itself completely.
					} else {
						// free code/data area of user process.
						if (task->flags & TASK_FLAGS_USER) {
							k_freeMem(task->memAddr);
						}
					}
					
				/**
				  [Code Block 2] If end task is thread, remove it from child thread list of process with thread in it,
				                 and completely end thread itself.
				*/
				} else if (task->flags & TASK_FLAGS_THREAD) {
					process = k_getProcessByThread(task);
					if (process != null) {
						if (k_findSchedulerByTaskWithLock(process->link.id, &processApicId) == true) {
							k_removeListById(&(process->childThreadList), task->link.id);
							k_unlockSpin(&(g_schedulers[processApicId].spinlock));
						}
					}
				}
				
				/* free resources of end task */
				if (task->flags & TASK_FLAGS_GUI) {
					k_deleteWindowsByTask(task->link.id);
				}

				k_freeMem(task->stackAddr);

				// free task of end task (If task is freed, then also stack is freed automatically.)
				taskId = task->link.id;
				k_freeTask(taskId);
				
				#if __DEBUG__
				k_printf("[idle debug] Task (0x%q) has completely ended.\n", taskId);
				#endif // __DEBUG__
			}
		}
		
		// switch task.
		k_schedule();
	}
}

void k_haltProcessorByLoad(byte apicId) {
	
	if (g_schedulers[apicId].processorLoad < 40) {
		k_halt();
		k_halt();
		k_halt();
		
	} else if (g_schedulers[apicId].processorLoad < 80) {
		k_halt();
		k_halt();
		
	} else if (g_schedulers[apicId].processorLoad < 95) {
		k_halt();
	}
}

qword k_getLastFpuUsedTaskId(byte apicId) {
	return g_schedulers[apicId].lastFpuUsedTaskId;
}

void k_setLastFpuUsedTaskId(byte apicId, qword taskId) {
	g_schedulers[apicId].lastFpuUsedTaskId = taskId;
}

// group ID global variables
static byte g_groupIndexBitmap[(TASK_MAXREUSABLEGROUPINDEXCOUNT + 7) / 8];
static bool g_firstGroupIndex = true;
static dword g_maxGroupIndex = (TASK_MAXREUSABLEGROUPINDEXCOUNT + 0x7) & 0xFFFFFFF8;
static dword g_groupCount = 0;

qword k_getTaskGroupId(void) {
	int i, j;
	byte data;	
	
	if (g_groupCount == 0xFFFFFFFF) {
		g_groupCount = 0;
	}

	g_groupCount++;

	/* reusable index */
	if (g_firstGroupIndex == true) {
		g_firstGroupIndex = false;
		k_memset(g_groupIndexBitmap, 0, sizeof(g_groupIndexBitmap));
		g_groupIndexBitmap[0] |= 1;
		return ((qword)g_groupCount << 32) | 0;
	}

	for (i = 0; i < ((TASK_MAXREUSABLEGROUPINDEXCOUNT + 7) / 8); i++) {
		data = g_groupIndexBitmap[i];

		for (j = 0; j < 8; j++) {
			if (!(data & (1 << j))) {
				data |= 1 << j;
				g_groupIndexBitmap[i] = data;
				return ((qword)g_groupCount << 32) | ((i * 8) + j);
			}
		}
	}

	/* not-reusable index */
	if (g_maxGroupIndex == 0xFFFFFFFF) {
		g_maxGroupIndex = (TASK_MAXREUSABLEGROUPINDEXCOUNT + 0x7) & 0xFFFFFFF8;
	}

	return ((qword)g_groupCount << 32) | (g_maxGroupIndex++);
}

void k_returnTaskGroupId(qword groupId) {
	dword index;
	byte data;
	
	index = GETTASKGROUPINDEX(groupId);
	if (index >= g_maxGroupIndex) {
		return;
	}

	data = g_groupIndexBitmap[index / 8];
	data &= ~(1 << (index % 8));
	g_groupIndexBitmap[index / 8] = data;
}

bool k_waitGroup(qword groupId, void* lock) {
	Task* target;
	bool result;

	target = k_getRunningTask(k_getApicId());
	target->waitGroupId = groupId;

	if (lock != null) {
		k_unlockAny(lock);
	}		

	result = k_waitTask(target->link.id);

	target->waitGroupId = 0;

	if (lock != null) {
		k_lockAny(lock);
	}

	return result;
}

bool k_notifyOneInWaitGroup(qword groupId) {
	Task* task;
	bool result = false;

	k_lockSpin(&g_commonScheduler.spinlock);

	task = k_getHeadFromList(&g_commonScheduler.waitList);
	while (task != null) {
		if (task->waitGroupId == groupId) {
			result = k_notifyTask(task->link.id);
			break;
		}

		task = k_getNextFromList(&g_commonScheduler.waitList, task);
	} 

	k_unlockSpin(&g_commonScheduler.spinlock);

	return result;
}

bool k_notifyAllInWaitGroup(qword groupId) {
	Task* task;
	bool result = false;

	k_lockSpin(&g_commonScheduler.spinlock);

	task = k_getHeadFromList(&g_commonScheduler.waitList);
	while (task != null) {
		if (task->waitGroupId == groupId) {
			result = k_notifyTask(task->link.id);
		}

		task = k_getNextFromList(&g_commonScheduler.waitList, task);
	} 

	k_unlockSpin(&g_commonScheduler.spinlock);

	return result;
}

bool k_joinGroup(qword* taskIds, int count) {
	qword groupId;
	int i;
	Task* joinTask;
	Task* waitTask;
	bool result;

	groupId = k_getTaskGroupId();

	for (i = 0; i < count; i++) {
		joinTask = k_getTaskFromPool(GETTASKOFFSET(taskIds[i]));
		joinTask->joinGroupId = groupId;
		joinTask->flags |= TASK_FLAGS_JOIN;
	}

	waitTask = k_getRunningTask(k_getApicId());
	waitTask->joinGroupId = groupId;
	waitTask->joinCount = count;

	result = k_waitTask(waitTask->link.id);

	waitTask->joinGroupId = 0;
	waitTask->joinCount = 0;

	k_returnTaskGroupId(groupId);

	return result;
}

bool k_notifyOneInJoinGroup(qword groupId) {
	Task* task;
	bool result = false;

	k_lockSpin(&g_commonScheduler.spinlock);

	task = k_getHeadFromList(&g_commonScheduler.waitList);
	while (task != null) {
		if (task->joinGroupId == groupId) {
			if (task->joinCount > 1) {
				task->joinCount--;
				break;
			}

			task->joinCount = 0;
			result = k_notifyTask(task->link.id);
			break;
		}

		task = k_getNextFromList(&g_commonScheduler.waitList, task);
	} 

	k_unlockSpin(&g_commonScheduler.spinlock);

	return result;
}

bool k_notifyAllInJoinGroup(qword groupId) {
	Task* task;
	bool result = false;

	k_lockSpin(&g_commonScheduler.spinlock);

	task = k_getHeadFromList(&g_commonScheduler.waitList);
	while (task != null) {
		if (task->joinGroupId == groupId) {
			if (task->joinCount > 1) {
				task->joinCount--;
				task = k_getNextFromList(&g_commonScheduler.waitList, task);
				continue;
			}

			task->joinCount = 0;
			result = k_notifyTask(task->link.id);
		}

		task = k_getNextFromList(&g_commonScheduler.waitList, task);
	} 

	k_unlockSpin(&g_commonScheduler.spinlock);

	return result;
}

qword k_createThread(qword entryPointAddr, qword arg, byte affinity, qword exitFunc) {
	Task* task;

	task = k_createTask(TASK_FLAGS_THREAD | TASK_FLAGS_USER, null, 0, entryPointAddr, affinity);
	if (task == null) {
		return TASK_INVALIDID;
	}

	// replace k_exitTask to exitFunc in stack.
	*(qword*)task->context.registers[TASK_INDEX_RSP] = exitFunc;

	// set argument to RDI (first parameter).
	task->context.registers[TASK_INDEX_RDI] = arg;

	return task->link.id;
}
