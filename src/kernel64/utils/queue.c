#include "queue.h"
#include "util.h"
#include "../core/task.h"

byte g_queueIndexBitmap[(QUEUE_MAXREUSABLEINDEXCOUNT + 7) / 8];
bool g_firstQueueIndex = true;
dword g_maxQueueIndex = (QUEUE_MAXREUSABLEINDEXCOUNT + 0x7) & 0xFFFFFFF8;
dword g_queueCount = 0;

static qword k_getQueueId(void) {
	int i, j;
	byte data;	
	
	if (g_queueCount == 0xFFFFFFFF) {
		g_queueCount = 0;
	}

	g_queueCount++;

	/* reusable index */
	if (g_firstQueueIndex == true) {
		g_firstQueueIndex = false;
		k_memset(g_queueIndexBitmap, 0, sizeof(g_queueIndexBitmap));
		g_queueIndexBitmap[0] |= 1;
		return ((qword)g_queueCount << 32) | 0;
	}

	for (i = 0; i < ((QUEUE_MAXREUSABLEINDEXCOUNT + 7) / 8); i++) {
		data = g_queueIndexBitmap[i];

		for (j = 0; j < 8; j++) {
			if (!(data & (1 << j))) {
				data |= 1 << j;
				g_queueIndexBitmap[i] = data;
				return ((qword)g_queueCount << 32) | ((i * 8) + j);
			}
		}
	}

	/* not-reusable index */
	if (g_maxQueueIndex == 0xFFFFFFFF) {
		g_maxQueueIndex = (QUEUE_MAXREUSABLEINDEXCOUNT + 0x7) & 0xFFFFFFF8;
	}

	return ((qword)g_queueCount << 32) | (g_maxQueueIndex++);
}

static void k_returnQueueId(qword id) {
	dword index;
	byte data;
	
	index = GETQUEUEINDEX(id);
	if (index >= g_maxQueueIndex) {
		return;
	}

	data = g_queueIndexBitmap[index / 8];
	data &= ~(1 << (index % 8));
	g_queueIndexBitmap[index / 8] = data;
}

void k_initQueue(Queue* queue, void* array, int dataSize, int maxDataCount) {
	queue->id = k_getQueueId();
	queue->dataSize = dataSize;
	queue->maxDataCount = maxDataCount;
	queue->array = array;
	queue->putIndex = 0;
	queue->getIndex = 0;
	queue->lastOperationPut = false;
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

bool k_putQueueBlocking(Queue* queue, const void* data) {
	if (k_isQueueFull(queue) == true) {
		return false;
	}
	
	k_memcpy((char*)queue->array + (queue->putIndex * queue->dataSize), data, queue->dataSize);
	queue->putIndex = (queue->putIndex + 1) % queue->maxDataCount;
	queue->lastOperationPut = true;
	
	k_notifyOneInGroup(queue->id);

	return true;
}

bool k_getQueueBlocking(Queue* queue, void* data, void* lock) {
	while (k_isQueueEmpty(queue) == true) {
		k_waitGroup(queue->id, lock);
	}

	k_memcpy(data, (char*)queue->array + (queue->getIndex * queue->dataSize), queue->dataSize);
	queue->getIndex = (queue->getIndex + 1) % queue->maxDataCount;
	queue->lastOperationPut = false;
	
	return true;
}

void k_closeQueue(const Queue* queue) {
	k_returnQueueId(queue->id);
}
