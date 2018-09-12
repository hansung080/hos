#ifndef __TASK_H__
#define __TASK_H__

#include "types.h"
#include "list.h"
#include "sync.h"

// count and size of context registers
#define TASK_REGISTERCOUNT 24 // 24 = 5 (SS, RSP, RFLAGS, CS, RIP) + 19 (registers saved in ISR)
#define TASK_REGISTERSIZE  8

// register offset of Context structure
#define TASK_GS_OFFSET     0
#define TASK_FS_OFFSET     1
#define TASK_ES_OFFSET     2
#define TASK_DS_OFFSET     3
#define TASK_R15_OFFSET    4
#define TASK_R14_OFFSET    5
#define TASK_R13_OFFSET    6
#define TASK_R12_OFFSET    7
#define TASK_R11_OFFSET    8
#define TASK_R10_OFFSET    9
#define TASK_R9_OFFSET     10
#define TASK_R8_OFFSET     11
#define TASK_RSI_OFFSET    12
#define TASK_RDI_OFFSET    13
#define TASK_RDX_OFFSET    14
#define TASK_RCX_OFFSET    15
#define TASK_RBX_OFFSET    16
#define TASK_RAX_OFFSET    17
#define TASK_RBP_OFFSET    18
#define TASK_RIP_OFFSET    19
#define TASK_CS_OFFSET     20
#define TASK_RFLAGS_OFFSET 21
#define TASK_RSP_OFFSET    22
#define TASK_SS_OFFSET     23

// macros related with task pool
#define TASK_TASKPOOLADDRESS 0x800000 // 8M
#define TASK_MAXCOUNT        1024     // max task count

// macros related with stack pool
#define TASK_STACKPOOLADDRESS (TASK_TASKPOOLADDRESS + (sizeof(Task) * TASK_MAXCOUNT))
#define TASK_STACKSIZE        8192 // 8KB

// invalid task ID
#define TASK_INVALIDID     0xFFFFFFFFFFFFFFFF

// maximum processor time for task to use at once (5 milliseconds)
#define TASK_PROCESSORTIME 5

// max ready list count: same as task priority count.
#define TASK_MAXREADYLISTCOUNT 5

// task priority (low 8 bits of task flags)
#define TASK_FLAGS_HIGHEST 0x00 // highest
#define TASK_FLAGS_HIGH    0x01 // high
#define TASK_FLAGS_MEDIUM  0x02 // medium
#define TASK_FLAGS_LOW     0x03 // low
#define TASK_FLAGS_LOWEST  0x04 // lowest
#define TASK_FLAGS_END     0xFF // end task priority

// task flags
#define TASK_FLAGS_ENDTASK 0x8000000000000000 // end task flag
#define TASK_FLAGS_SYSTEM  0x4000000000000000 // system task flag
#define TASK_FLAGS_PROCESS 0x2000000000000000 // processor flag
#define TASK_FLAGS_THREAD  0x1000000000000000 // thread flag
#define TASK_FLAGS_IDLE    0x0800000000000000 // idle task flag

// affinity
#define TASK_AFFINITY_LOADBALANCING 0xFF // no affinity

/* macro functions */
#define GETTASKOFFSET(taskId)            ((taskId) & 0xFFFFFFFF)                                 // get low 32 bits of task.link.id (64 bits)
#define GETTASKPRIORITY(flags)           ((flags) & 0xFF)                                        // get low 8 bits of task.flags(64 bits)
#define SETTASKPRIORITY(flags, priority) ((flags) = ((flags) & 0xFFFFFFFFFFFFFF00) | (priority)) // set low 8 bits of task.flags(64 bits)
#define GETTASKFROMTHREADLINK(x)         (Task*)((qword)(x) - offsetof(Task, threadLink))        // get task address using task.threadLink address

#pragma pack(push, 1)

typedef struct k_Context {
	qword registers[TASK_REGISTERCOUNT];
} Context;

typedef struct k_Task {
	//--------------------------------------------------
	// Task-related Fields
	//--------------------------------------------------
	ListLink link;           // scheduler link: It consists of next task address (link.next) and task ID (link.id).
	                         //                 task ID consists of allocated task count (high 32 bits) and task offset (low 32 bits).
	                         //                 [Note] ListLink must be the first field.
	qword flags;             // task flags: bit 0~7 : task priority
							 //             bit 63  : end task flag
	                         //             bit 62  : system task flag
	                         //             bit 61  : processor flag
	                         //             bit 60  : thread flag
	                         //             bit 59  : idle task flag
	void* memAddr;           // start address of process memory area (code, data area)
	qword memSize;           // size of process memory area (code, data area)
	
	//--------------------------------------------------
	// Thread-related Fields
	//--------------------------------------------------
	ListLink threadLink;     // child thread link: It consists of next child thread address (threadLink.next) and thread ID (threadLink.id).
	qword parentProcessId;   // parent process ID
	qword fpuContext[512/8]; // FPU context (512 bytes-fixed)
	                         //     : [Note] The start address of FPU context must be the multiple of 16 bytes.
	                         //       To guarantee it, the conditions below must be satisfied.
	                         //       - Condition 1: The start address of task pool must be the multiple of 16 bytes. (currently, It's 0x800000 (8 MBytes).)
	                         //       - Condition 2: The size of each task must be the multiple of 16 bytes. (currently, It's 816 bytes.)
	                         //       - Condition 3: The FPU context offset of each task must be the multiple of 16 bytes. (currently, It's 64 bytes)
	                         //       Currently, the conditions above are satisfied. Thus, it's recommended to add fields below FPU context field.
	List childThreadList;    // child thread list
	
	//--------------------------------------------------
	// Task-related Fields
	//--------------------------------------------------
	Context context;         // context
	void* stackAddr;         // start address of stack
	qword stackSize;         // size of stack
	bool fpuUsed;            // FPU used flag: It indicates whether the task has used FPU operation before.
	byte apicId;             // APIC ID of core which task is running on
	byte affinity;           // task-processor affinity: APIC ID of core which has affinity with task
	char padding[9];         // padding bytes: According to Condition 2 of FPU context, align task size with the multiple of 16 bytes.
} Task; // Task is ListItem, and current task size is 816 bytes.

typedef struct k_TaskPoolManager {
	Spinlock spinlock;  // spinlock
	Task* startAddr;    // start address of task pool: You can consider it as task array.
	int maxCount;       // max task count
	int usedCount;      // used task count: currently being-used task count.
	int allocatedCount; // allocated task count: It only increases when allocating task. It's for task ID to be unique.
} TaskPoolManager;

typedef struct k_Scheduler {
	Spinlock spinlock;                          // spinlock
	Task* runningTask;                          // running task (current task)
	int processorTime;                          // processor time for running task to use
	List readyLists[TASK_MAXREADYLISTCOUNT];    // ready lists: Tasks which are ready to run are in the lists identified by task priority.
	List endList;                               // end list: Tasks which have ended are in the list. Idle task will free memory of tasks in end list.
	int executedCounts[TASK_MAXREADYLISTCOUNT]; // executed task counts by task priority
	qword processorLoad;                        // processor load (processor usage)
	qword processorTimeInIdleTask;              // processor time for idle task to use
	qword lastFpuUsedTaskId;                    // last FPU-used task ID
	bool loadBalancing;                         // task load balancing flag
} Scheduler;

#pragma pack(pop)

/* Task Pool Functions */
static void k_initTaskPool(void);
static Task* k_allocTask(void);
static void k_freeTask(qword taskId);

/* Task Functions */
Task* k_createTask(qword flags, void* memAddr, qword memSize, qword entryPointAddr, byte affinity);
static void k_setTask(Task* task, qword flags, qword entryPointAddr, void* stackAddr, qword stackSize);

/* Scheduler Functions */
void k_initScheduler(void);
void k_setRunningTask(byte apicId, Task* task);
Task* k_getRunningTask(byte apicId);
static Task* k_getNextTaskToRun(byte apicId); // get next running task from ready list.
static bool k_addTaskToReadyList(byte apicId, Task* task); // add task to ready list.
bool k_schedule(void); // task switching in task.
bool k_scheduleInInterrupt(void); // task switching in interrupt handler.
void k_decreaseProcessorTime(byte apicId);
bool k_isProcessorTimeExpired(byte apicId);
static Task* k_removeTaskFromReadyList(byte apicId, qword taskId); // remove task from ready list.
static bool k_findSchedulerByTaskWithLock(qword taskId, byte* apicId);
bool k_changeTaskPriority(qword taskId, byte priority); // change task priority.
bool k_endTask(qword taskId); // end task.
void k_exitTask(void); // end task by itself.
int k_getReadyTaskCount(byte apicId); // get ready task count.
int k_getTaskCount(byte apicId); // get total task count. (total task count = ready task count + wait task count + running task count)
Task* k_getTaskFromPool(int offset);
bool k_existTask(qword taskId);
qword k_getProcessorLoad(byte apicId);
static Task* k_getProcessByThread(Task* thread); // get process by thread: process returns itself, and thread returns parent process.
void k_addTaskToSchedulerWithLoadBalancing(Task* task);
static byte k_findSchedulerByMinTaskCount(const Task* task);
void k_setTaskLoadBalancing(byte apicId, bool loadBalancing);
bool k_changeTaskAffinity(qword taskId, byte affinity);

/* Idle Task Functions */
void k_idleTask(void);
void k_haltProcessorByLoad(byte apicId);

/* FPU Functions */
qword k_getLastFpuUsedTaskId(byte apicId);
void k_setLastFpuUsedTaskId(byte apicId, qword taskId);

#endif // __TASK_H__