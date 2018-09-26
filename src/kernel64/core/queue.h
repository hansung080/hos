#ifndef __CORE_QUEUE_H__
#define __CORE_QUEUE_H__

#include "types.h"

#pragma pack(push, 1)

// General Array Queue
typedef struct k_Queue {
	int dataSize;
	int maxDataCount;
	void* array;  // array (buffer) address: save address of array user declared in order to make queue general.
	int getIndex; // get index: get data from head of array.
	int putIndex; // put index: put data to tail of array.
	bool lastOperationPut;
} Queue;

#pragma pack(pop)

void k_initQueue(Queue* queue, void* array, int maxDataCount, int dataSize);
bool k_isQueueFull(const Queue* queue);
bool k_isQueueEmpty(const Queue* queue);
bool k_putQueue(Queue* queue, const void* data);
bool k_getQueue(Queue* queue, void* data);

#endif // __CORE_QUEUE_H__