#ifndef __UTILS_QUEUE_H__
#define __UTILS_QUEUE_H__

#include "../core/types.h"

#pragma pack(push, 1)

// general array queue
typedef struct k_Queue {
	int dataSize;
	int maxDataCount;
	void* array;  // array (buffer) address: save address of array user declared in order to make queue general.
	int putIndex; // put index: put data to tail of array.
	int getIndex; // get index: get data from head of array.
	bool lastOperationPut;
	bool blocking;
	qword waitGroupId;
	struct k_Epoll* epoll;
} Queue;

// epoll
typedef struct k_Epoll {
	Queue** equeues;
	int len;
	qword id;
	bool waiting;
} Epoll;

#pragma pack(pop)

/* Queue Functions */
void k_initQueue(Queue* queue, void* array, int dataSize, int maxDataCount, bool blocking);
bool k_isQueueFull(const Queue* queue);
bool k_isQueueEmpty(const Queue* queue);
bool k_putQueue(Queue* queue, const void* data);
bool k_getQueue(Queue* queue, void* data, void* lock);
void k_closeQueue(const Queue* queue);

/* Epoll Functions */
void k_initEpoll(Epoll* epoll, Queue** equeues, int len);
bool k_waitEpoll(Epoll* epoll, void* lock);
void k_closeEpoll(const Epoll* epoll);

#endif // __UTILS_QUEUE_H__