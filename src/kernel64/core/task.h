#ifndef __CORE_TASK_H__
#define __CORE_TASK_H__

#include "types.h"
#include "../utils/list.h"
#include "sync.h"

/**
  < Task Life Cycle >
                              Start
                                | create
                  schedule      v
      ----------- <------- -----------
      | Running | -------> |  Ready  | -------  
      ----------- schedule -----------       |
          | |             wait | ^           |
          | | wait             v | notify    |
          | |              -----------       |
     exit | -------------> |  Wait   |       | end
          |                -----------       |
          |                     | end        |
          |                     v            |
          |                -----------       |
          ---------------> |   End   | <------
                           -----------
                                | idle
                                v
                           Complete End
    
    @ arrow functions
      - create   : k_createTask, k_initScheduler
      - schedule : k_schedule, k_scheduleInInterrupt 
      - wait     : k_waitTask, k_waitGroup, k_joinGroup
      - notify   : k_notifyTask, k_notifyOneInWaitGroup, k_notifyAllInWaitGroup, k_notifyOneInJoinGroup, k_notifyAllInJoinGroup
      - end      : k_endTask
      - exit     : k_exitTask
      - idle     : k_idleTask
*/

// context register count and size
#define TASK_REGISTERCOUNT 24 // 24 = 5 (SS, RSP, RFLAGS, CS, RIP) + 19 (registers saved in ISR)
#define TASK_REGISTERSIZE  8

// context register index
#define TASK_INDEX_GS     0
#define TASK_INDEX_FS     1
#define TASK_INDEX_ES     2
#define TASK_INDEX_DS     3
#define TASK_INDEX_R15    4
#define TASK_INDEX_R14    5
#define TASK_INDEX_R13    6
#define TASK_INDEX_R12    7
#define TASK_INDEX_R11    8
#define TASK_INDEX_R10    9
#define TASK_INDEX_R9     10
#define TASK_INDEX_R8     11
#define TASK_INDEX_RSI    12
#define TASK_INDEX_RDI    13
#define TASK_INDEX_RDX    14
#define TASK_INDEX_RCX    15
#define TASK_INDEX_RBX    16
#define TASK_INDEX_RAX    17
#define TASK_INDEX_RBP    18
#define TASK_INDEX_RIP    19
#define TASK_INDEX_CS     20
#define TASK_INDEX_RFLAGS 21
#define TASK_INDEX_RSP    22
#define TASK_INDEX_SS     23

// task pool and task stack-related macros
#define TASK_TASKPOOLSTARTADDRESS 0x800000    // 8 MB
#define TASK_MAXCOUNT             1024        // max task count
#define TASK_TASKPOOLENDADDRESS   (TASK_TASKPOOLSTARTADDRESS + (sizeof(Task) * TASK_MAXCOUNT))
#define TASK_STACKSIZE            (64 * 1024) // 64 KB

// invalid task ID
#define TASK_INVALIDID 0xFFFFFFFFFFFFFFFF

// maximum processor time for task to use at once (5 milliseconds)
#define TASK_PROCESSORTIME 5

// max ready list count: same as task priority count.
#define TASK_MAXREADYLISTCOUNT 5

// task priority (low 8 bits of task flags)
#define TASK_PRIORITY_HIGHEST 0x00 // highest
#define TASK_PRIORITY_HIGH    0x01 // high
#define TASK_PRIORITY_MEDIUM  0x02 // medium
#define TASK_PRIORITY_LOW     0x03 // low
#define TASK_PRIORITY_LOWEST  0x04 // lowest

// task flags
#define TASK_FLAGS_WAIT    0x8000000000000000 // wait task flag
#define TASK_FLAGS_END     0x4000000000000000 // end task flag
#define TASK_FLAGS_SYSTEM  0x2000000000000000 // system task flag
#define TASK_FLAGS_PROCESS 0x1000000000000000 // processor flag
#define TASK_FLAGS_THREAD  0x0800000000000000 // thread flag
#define TASK_FLAGS_IDLE    0x0400000000000000 // idle task flag
#define TASK_FLAGS_GUI     0x0200000000000000 // GUI task flag: set in k_createWindow.
#define TASK_FLAGS_USER    0x0100000000000000 // user task flag
#define TASK_FLAGS_JOIN    0x0080000000000000 // join task flag: set in k_joinGroup.

// affinity
#define TASK_AFFINITY_LOADBALANCING 0xFF // load balancing (no affinity)

// max reusable group index count
// - The real max reusable group index count is aligned with 8.
#define TASK_MAXREUSABLEGROUPINDEXCOUNT 8192 

/* macro functions */
#define GETTASKOFFSET(taskId)            ((taskId) & 0xFFFFFFFF)                                 // get task offset (low 32 bits) of task.link.id (64 bits).
#define GETTASKPRIORITY(flags)           ((flags) & 0xFF)                                        // get task priority (low 8 bits) of task.flags(64 bits).
#define SETTASKPRIORITY(flags, priority) ((flags) = ((flags) & 0xFFFFFFFFFFFFFF00) | (priority)) // set task priority (low 8 bits) of task.flags(64 bits).
#define GETTASKFROMTHREADLINK(x)         (Task*)((qword)(x) - offsetof(Task, threadLink))        // get task address using task.threadLink address.
// [REF] The group ID consists of group count (high 32 bits) and group index (low 32 bits).
#define GETTASKGROUPINDEX(id) ((id) & 0xFFFFFFFF) // get group index (low 32 bits) of group ID (64 bits).

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
	                         //                 [NOTE] ListLink must be the first field.
	qword flags;             // task flags: bit 0~7 : task priority
	                         //             bit 63  : wait task flag
							 //             bit 62  : end task flag
	                         //             bit 61  : system task flag
	                         //             bit 60  : processor flag
	                         //             bit 59  : thread flag
	                         //             bit 58  : idle task flag
							 //             bit 57  : GUI task flag
							 //             bit 56  : user task flag
							 //             bit 55  : join task flag
	void* memAddr;           // start address of process memory area (code/data area)
	qword memSize;           // size of process memory area (code/data area)
	
	//--------------------------------------------------
	// Thread-related Fields
	//--------------------------------------------------
	ListLink threadLink;     // child thread link: It consists of next child thread address (threadLink.next) and thread ID (threadLink.id).
	qword parentProcessId;   // parent process ID
	qword fpuContext[512/8]; // FPU context (512 bytes-fixed)
	                         //     : [NOTE] The start address of FPU context must be the multiple of 16 bytes.
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
	qword waitGroupId;       // wait group ID
	qword joinGroupId;       // join group ID
	int joinCount;           // join count
	char padding[5];         // padding bytes: According to Condition 2 of FPU context, align task size with the multiple of 16 bytes.
} Task; // Task is ListItem, and current task size is 832 bytes.

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

typedef struct k_CommonScheduler {
	Spinlock spinlock; // spinlock
	List waitList;     // wait list: Tasks which are waiting to be ready are in the list.
} CommonScheduler;

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
static Task* k_removeTaskFromReadyList(byte apicId, qword taskId); // remove task from ready list.
static Task* k_getProcessByThread(Task* thread); // get process by thread: process returns itself, and thread returns parent process.
static bool k_findSchedulerByTaskWithLock(qword taskId, byte* apicId);
static byte k_findSchedulerByMinTaskCount(const Task* task);
static void k_addTaskToSchedulerWithLoadBalancing(Task* task);
static void k_addTaskToWaitList(Task* task);
static Task* k_removeTaskFromWaitList(qword taskId);
bool k_schedule(void); // task switching in task.
bool k_scheduleInInterrupt(void); // task switching in interrupt handler.
void k_decreaseProcessorTime(byte apicId);
bool k_isProcessorTimeExpired(byte apicId);
bool k_changeTaskPriority(qword taskId, byte priority);
bool k_changeTaskAffinity(qword taskId, byte affinity);
bool k_waitTask(qword taskId);
bool k_notifyTask(qword taskId);
void k_printWaitTaskInfo(void);
bool k_endTask(qword taskId);
void k_exitTask(void);
int k_getReadyTaskCount(byte apicId); // get ready task count.
int k_getTaskCount(byte apicId); // get total task count. (total task count = ready task count + wait task count + running task count)
Task* k_getTaskFromPool(int offset);
bool k_existTask(qword taskId);
qword k_getProcessorLoad(byte apicId);
void k_setTaskLoadBalancing(byte apicId, bool loadBalancing);

/* Idle Task Functions */
void k_idleTask(void);
void k_haltProcessorByLoad(byte apicId);

/* FPU Functions */
qword k_getLastFpuUsedTaskId(byte apicId);
void k_setLastFpuUsedTaskId(byte apicId, qword taskId);

/* Task Group Functions */
qword k_getTaskGroupId(void);
void k_returnTaskGroupId(qword groupId);
bool k_waitGroup(qword groupId, void* lock);
bool k_notifyOneInWaitGroup(qword groupId);
bool k_notifyAllInWaitGroup(qword groupId);
bool k_joinGroup(qword* taskIds, int count);
bool k_notifyOneInJoinGroup(qword groupId);
bool k_notifyAllInJoinGroup(qword groupId);

/* Application Functions */
qword k_createThread(qword entryPointAddr, qword arg, byte affinity, qword exitFunc);

#endif // __CORE_TASK_H__