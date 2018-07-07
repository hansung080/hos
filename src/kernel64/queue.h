#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "types.h"

#pragma pack(push, 1)

typedef struct k_Queue {
	int dataSize;
	int maxDataCount;
	void* array;  // save address of array for general queue.
	int getIndex; // get data from head of array.
	int putIndex; // put data to tail of array.
	bool lastOperationPut;
} Queue;

#pragma pack(pop)

void k_initQueue(Queue* queue, void* array, int maxDataCount, int dataSize);
bool k_isQueueFull(const Queue* queue);
bool k_isQueueEmpty(const Queue* queue);
bool k_putQueue(Queue* queue, const void* data);
bool k_getQueue(Queue* queue, void* data);

#endif // __QUEUE_H__
