#include "queue.h"
#include "util.h"

void k_initQueue(Queue* queue, void* array, int maxDataCount, int dataSize) {
	queue->dataSize = dataSize;
	queue->maxDataCount = maxDataCount;
	queue->array = array;
	queue->getIndex = 0;
	queue->putIndex = 0;
	queue->lastOperationPut = false;
}

bool k_isQueueFull(const Queue* queue) {
	if ((queue->getIndex == queue->putIndex) && (queue->lastOperationPut == true)) {
		return true;
	}

	return false;
}

bool k_isQueueEmpty(const Queue* queue) {
	if ((queue->getIndex == queue->putIndex) && (queue->lastOperationPut == false)) {
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

	return true;
}

bool k_getQueue(Queue* queue, void* data) {
	if (k_isQueueEmpty(queue) == true) {
		return false;
	}

	k_memcpy(data, (char*)queue->array + (queue->getIndex * queue->dataSize), queue->dataSize);
	queue->getIndex = (queue->getIndex + 1) % queue->maxDataCount;
	queue->lastOperationPut = false;

	return true;
}
