#include "queue.h"
#include "util.h"
#include "../core/task.h"

void k_initQueue(Queue* queue, void* array, int dataSize, int maxDataCount, bool blocking) {
	queue->dataSize = dataSize;
	queue->maxDataCount = maxDataCount;
	queue->array = array;
	queue->putIndex = 0;
	queue->getIndex = 0;
	queue->lastOperationPut = false;
	queue->blocking = blocking;
	if (blocking == true) {
		queue->waitGroupId = k_getTaskGroupId();
	}
	queue->epoll = null;
}

bool k_isQueueFull(const Queue* queue) {
	if ((queue->putIndex == queue->getIndex) && (queue->lastOperationPut == true)) {
		return true;
	}
	
	return false;
}

bool k_isQueueEmpty(const Queue* queue) {
	if ((queue->putIndex == queue->getIndex) && (queue->lastOperationPut == false)) {
		return true;
	}
	
	return false;
}

bool k_putQueue(Queue* queue, const void* data) {
	if (k_isQueueFull(queue) == true) {
		return false;
	}
	
	k_memcpy((char*)queue->array + (queue->putIndex * queue->dataSize), data, queue->dataSize);
	queue->putIndex = (queue->putIndex + 1) % queue->maxDataCount;
	queue->lastOperationPut = true;
	
	if (queue->blocking == true) {
		k_notifyOneInWaitGroup(queue->waitGroupId);
	}
	
	if ((queue->epoll != null) && (queue->epoll->waiting == true)) {
		k_notifyOneInWaitGroup(queue->epoll->id);
	}

	return true;
}

bool k_getQueue(Queue* queue, void* data, void* lock) {
	if (queue->blocking == true) {
		while (k_isQueueEmpty(queue) == true) {
			k_waitGroup(queue->waitGroupId, lock);
		}

	} else {
		if (k_isQueueEmpty(queue) == true) {
			return false;
		}
	}

	k_memcpy(data, (char*)queue->array + (queue->getIndex * queue->dataSize), queue->dataSize);
	queue->getIndex = (queue->getIndex + 1) % queue->maxDataCount;
	queue->lastOperationPut = false;
	
	return true;
}

void k_closeQueue(const Queue* queue) {
	if (queue->blocking == true) {
		k_returnTaskGroupId(queue->waitGroupId);
	}
}

void k_initEpoll(Epoll* epoll, Queue** equeues, int len) {
	int i;

	epoll->equeues = equeues;
	epoll->len = len;
	epoll->id = k_getTaskGroupId();
	epoll->waiting = false;

	for (i = 0; i < len; i++) {
		equeues[i]->epoll = epoll;
	}
}

bool k_waitEpoll(Epoll* epoll, void* lock) {
	int i;
	bool exist = false;

	for (i = 0; i < epoll->len; i++) {
		if (k_isQueueEmpty(epoll->equeues[i]) == false) {
			exist = true;
			break;
		}
	}

	if (exist == false) {
		epoll->waiting = true;
		k_waitGroup(epoll->id, lock);
		epoll->waiting = false;
	}
}

void k_closeEpoll(const Epoll* epoll) {
	k_returnTaskGroupId(epoll->id);
}
