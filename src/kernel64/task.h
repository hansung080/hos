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
#define TASK_TCBPOOLADDRESS 0x800000 // 8M
#define TASK_MAXCOUNT       1024

// macros related with stack pool
#define TASK_STACKPOOLADDRESS (TASK_TCBPOOLADDRESS + (sizeof(Tcb) * TASK_MAXCOUNT))
#define TASK_STACKSIZE        8192 // 8KB

// invalid task ID
#define TASK_INVALIDID     0xFFFFFFFFFFFFFFFF

// maximum processor time for task to use at once (5 milliseconds)
#define TASK_PROCESSORTIME 5

// ready list count
#define TASK_MAXREADYLISTCOUNT 5

// task priority (low 8 bits of task flag (64 bits))
#define TASK_FLAGS_HIGHEST 0
#define TASK_FLAGS_HIGH    1
#define TASK_FLAGS_MEDIUM  2
#define TASK_FLAGS_LOW     3
#define TASK_FLAGS_LOWEST  4
#define TASK_FLAGS_WAIT    0xFF // priority of end task

// task flag
#define TASK_FLAGS_ENDTASK 0x8000000000000000 // end task
#define TASK_FLAGS_SYSTEM  0x4000000000000000 // system task
#define TASK_FLAGS_PROCESS 0x2000000000000000 // processor
#define TASK_FLAGS_THREAD  0x1000000000000000 // thread
#define TASK_FLAGS_IDLE    0x0800000000000000 // idle task

// macro functions
#define GETPRIORITY(x)           ((x) & 0xFF)                                    // get low 8 bits of TCB.flags(64 bits)
#define SETPRIORITY(x, priority) ((x) = ((x) & 0xFFFFFFFFFFFFFF00) | (priority)) // set low 8 bits of TCB.flags(64 bits)
#define GETTCBOFFSET(x)          ((x) & 0xFFFFFFFF)                              // get low 32 bits of TCB.link.id (64 bits)
#define GETTCBFROMTHREADLINK(x)  (Tcb*)((qword)(x) - offsetof(Tcb, threadLink))  // get TCB address using TCB.threadLink address

#pragma pack(push, 1)

typedef struct k_Context {
	qword registers[TASK_REGISTERCOUNT];
} Context;

typedef struct k_Tcb {
	//--------------------------------------------------
	// Task Info Fields
	//--------------------------------------------------
	ListLink link;           // scheduler link: It consists of next task position (link.next) and task ID (link.id).
	                         //                 task ID consists of TCB allocation count (high 32 bits) and TCB offset (low 32 bits).
	                         //                 [Note] ListLink must be positioned at the first of the structure.
	qword flags;             // task flag: bit 63 is end task flag.
                             //            bit 62 is system task flag.
	                         //            bit 61 is processor flag.
	                         //            bit 60 is thread flag.
	                         //            bit 59 is idle task flag.
	                         //            bit 7~0 is task priority.
	void* memAddr;           // start address of process memory area (code, data area)
	qword memSize;           // size of process memory area (code, data area)
	//--------------------------------------------------
	// Thread Info Fields
	//--------------------------------------------------
	ListLink threadLink;     // child thread link: It consists of next child thread position (threadLink.next) and thread ID (threadLink.id).
	qword parentProcessId;   // parent process ID
	qword fpuContext[512/8]; // FPU context (512 bytes-fixed)
	                         //     : [Note] The start address of FPU context must be the multiple of 16 bytes.
	                         //       To guarantee it, the conditions below must be satisfied.
	                         //       - Condition 1: The start address of TCB pool must be the multiple of 16 bytes. (currently, It's 0x800000 (8 MBytes).)
	                         //       - Condition 2: The size of each TCB must be the multiple of 16 bytes. (currently, It's 816 bytes.)
                             //       - Condition 3: The FPU context offset of each TCB must be the multiple of 16 bytes. (currently, It's 64 bytes)
	                         //       Currently, the conditions above are satisfied. Thus, it's recommended to add fields below FPU context field.
	List childThreadList;    // child thread list
	Context context;         // context
	void* stackAddr;         // start address of stack
	qword stackSize;         // size of stack
	bool fpuUsed;            // FPU used flag: It indicates whether the task has used FPU operation before.
	char padding[11];        // padding bytes: According to Condition 2 of FPU context, align TCB size with the multiple of 16 bytes.
} Tcb; // TCB is ListItem, and current TCB size is 816 bytes.

typedef struct k_TcbPoolManager {
	Tcb* startAddr;   // start address of TCB pool
	int maxCount;     // TCB max count: max TCB count in TCB pool
	int useCount;     // TCB use count: TCB count being used in TCB pool
	int allocedCount; // TCB allocated count: TCB-allocated times in TCB pool. It's accumulated.
} TcbPoolManager;

typedef struct k_Scheduler {
	Spinlock spinlock;                         // spinlock
	Tcb* runningTask;                          // current running task
	int processorTime;                         // process time for current running task to use
	List readyLists[TASK_MAXREADYLISTCOUNT];   // ready lists: Tasks which are waiting to run, are in the lists identified by task priority.
	List waitList;                             // wait list: Tasks which are waiting to end, are in the list.
	int executeCounts[TASK_MAXREADYLISTCOUNT]; // task execute counts by task priority
	qword processorLoad;                       // process load (process usage)
	qword spendProcessorTimeInIdleTask;        // process time for idle task to use
	qword lastFpuUsedTaskId;                   // last FPU-used task ID
} Scheduler;

#pragma pack(pop)

/* Task-related Functions */
static void k_initTcbPool(void);
static Tcb* k_allocTcb(void);
static void k_freeTcb(qword id);
Tcb* k_createTask(qword flags, void* memAddr, qword memSize, qword entryPointAddr);
static void k_setupTask(Tcb* task, qword flags, qword entryPointAddr, void* stackAddr, qword stackSize);

/* Scheduler-related Functions */
void k_initScheduler(void);
void k_setRunningTask(Tcb* task);
Tcb* k_getRunningTask(void);
static Tcb* k_getNextTaskToRun(void); // get next running task from ready list.
static bool k_addTaskToReadyList(Tcb* task); // add task to ready list.
void k_schedule(void); // task switching in task.
bool k_scheduleInInterrupt(void); // task switching in interrupt handler.
void k_decreaseProcessorTime(void);
bool k_isProcessorTimeExpired(void);
static Tcb* k_removeTaskFromReadyList(qword taskId); // remove task from ready list.
bool k_changePriority(qword taskId, byte priority); // change task priority.
bool k_endTask(qword taskId); // end task.
void k_exitTask(void); // end task by itself.
int k_getReadyTaskCount(void); // get ready task count.
int k_getTaskCount(void); // get total task count. (total task count = ready task count + end-waiting task count + current running task count)
Tcb* k_getTcbInTcbPool(int offset);
bool k_isTaskExist(qword taskId);
qword k_getProcessorLoad(void);
static Tcb* k_getProcessByThread(Tcb* thread); // get process by thread: process returns itself, and thread returns parent process.

/* Idle Task-related Functions */
void k_idleTask(void);
void k_haltProcessorByLoad(void);

/* FPU-related Functions */
qword k_getLastFpuUsedTaskId(void);
void k_setLastFpuUsedTaskId(qword taskId);

#endif // __TASK_H__
