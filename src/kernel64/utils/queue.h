#ifndef __UTILS_QUEUE_H__
#define __UTILS_QUEUE_H__

#include "../core/types.h"

// max reusable index count
// - The real max reusable index count is aligned with 8.
#define QUEUE_MAXREUSABLEINDEXCOUNT 8192 

/* macro function */
#define GETQUEUEINDEX(id) ((id) & 0xFFFFFFFF) // get queue index (low 32 bits) of queue id (64 bits).

#pragma pack(push, 1)

// General Array Queue
typedef struct k_Queue {
	qword id; // queue id: consist of queue count (high 32 bits) and queue index (low 32 bits).
	int dataSize;
	int maxDataCount;
	void* array;  // array (buffer) address: save address of array user declared in order to make queue general.
	int putIndex; // put index: put data to tail of array.
	int getIndex; // get index: get data from head of array.
	bool lastOperationPut;
} Queue;

#pragma pack(pop)

static qword k_getQueueId(void);
static void k_returnQueueId(qword id);
void k_initQueue(Queue* queue, void* array, int dataSize, int maxDataCount);
bool k_isQueueFull(const Queue* queue);
bool k_isQueueEmpty(const Queue* queue);
bool k_putQueue(Queue* queue, const void* data);
bool k_getQueue(Queue* queue, void* data);
bool k_putQueueBlocking(Queue* queue, const void* data);
bool k_getQueueBlocking(Queue* queue, void* data, void* lock);
void k_closeQueue(const Queue* queue);

#endif // __UTILS_QUEUE_H__