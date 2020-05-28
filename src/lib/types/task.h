#ifndef __TYPES_TASK_H__
#define __TYPES_TASK_H__

// invalid task ID
#define TASK_INVALIDID 0xFFFFFFFFFFFFFFFF

// task priority (low 8 bits of task flags)
#define TASK_PRIORITY_HIGHEST 0x00
#define TASK_PRIORITY_HIGH    0x01
#define TASK_PRIORITY_MEDIUM  0x02
#define TASK_PRIORITY_LOW     0x03
#define TASK_PRIORITY_LOWEST  0x04

// task flags
#define TASK_FLAGS_WAIT    0x8000000000000000
#define TASK_FLAGS_END     0x4000000000000000
#define TASK_FLAGS_SYSTEM  0x2000000000000000
#define TASK_FLAGS_PROCESS 0x1000000000000000
#define TASK_FLAGS_THREAD  0x0800000000000000
#define TASK_FLAGS_IDLE    0x0400000000000000
#define TASK_FLAGS_GUI     0x0200000000000000
#define TASK_FLAGS_USER    0x0100000000000000
#define TASK_FLAGS_JOIN    0x0080000000000000

// task affinity
#define TASK_AFFINITY_LB 0xFF // load balancing (no affinity)

/* macro functions */
#define GETTASKOFFSET(taskId)            ((taskId) & 0xFFFFFFFF)                                 // get task offset (low 32 bits) of task.link.id (64 bits).
#define GETTASKPRIORITY(flags)           ((flags) & 0xFF)                                        // get task priority (low 8 bits) of task.flags(64 bits).
#define SETTASKPRIORITY(flags, priority) ((flags) = ((flags) & 0xFFFFFFFFFFFFFF00) | (priority)) // set task priority (low 8 bits) of task.flags(64 bits).

#endif // __TYPES_TASK_H__