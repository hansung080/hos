#include "kid.h"
#include "../core/dynamic_mem.h"
#include "../core/console.h"
#include "util.h"

static KidManager g_kidManager;

bool k_initKidManager(void) {
	k_initMutex(&g_kidManager.mutex);
	g_kidManager.index = 0;
	g_kidManager.freeIndex = 0;
	g_kidManager.count = 0;

	g_kidManager.freeBuffer = (dword*)k_allocMem(sizeof(dword) * KID_MAXFREEQUEUECOUNT);
	if (g_kidManager.freeBuffer == null) {
		k_printf("KID error: free buffer allocation failure\n");
		return false;
	}

	k_memset(g_kidManager.freeBuffer, 0, sizeof(dword) * KID_MAXFREEQUEUECOUNT);
	k_initQueue(&g_kidManager.freeQueue, g_kidManager.freeBuffer, sizeof(dword), KID_MAXFREEQUEUECOUNT, false);
}

qword k_allocKid(void) {
	qword id;	

	k_lock(&g_kidManager.mutex);

	if (g_kidManager.count == 0xFFFFFFFF) {
		g_kidManager.count = 0;
	}

	g_kidManager.count++;

	if (k_getQueue(&g_kidManager.freeQueue, &g_kidManager.freeIndex, null) == true) {
		id = ((qword)g_kidManager.count << 32) | g_kidManager.freeIndex;
		k_unlock(&g_kidManager.mutex);
		return id;
	}

	id = ((qword)g_kidManager.count << 32) | g_kidManager.index++;
	if (g_kidManager.index == 0xFFFFFFFF) {
		// The reset index is not 0, but KID_RESETINDEX, 
		// because 0 ~ KID_RESETINDEX is allocated for the system.
		g_kidManager.index = KID_RESETINDEX;
	}

	k_unlock(&g_kidManager.mutex);

	return id;
}

void k_freeKid(qword id) {
	k_lock(&g_kidManager.mutex);

	k_putQueue(&g_kidManager.freeQueue, &id);

	k_unlock(&g_kidManager.mutex);
}

#if 0
static KidManager g_kidManager;

bool k_initKidManager(void) {
	k_initMutex(&g_kidManager.mutex);

	g_kidManager.bitmap = (byte*)k_allocMem(KID_MAXREUSABLEINDEX / 8);
	if (g_kidManager.bitmap == null) {
		k_printf("KID error: KID bitmap allocation failure\n");
		return false;
	}

	k_memset(g_kidManager.bitmap, 0, KID_MAXREUSABLEINDEX / 8);
	g_kidManager.index = 0;
	g_kidManager.overIndex = KID_MAXREUSABLEINDEX;
	g_kidManager.count = 0;

	return true;
}

qword k_allocKid(void) {
	int i, j;
	byte data;
	qword id;
	
	k_lock(&g_kidManager.mutex);

	if (g_kidManager.count == 0xFFFFFFFF) {
		g_kidManager.count = 0;
	}

	g_kidManager.count++;

	/* reusable index */
	for (i = g_kidManager.index / 8; i < (KID_MAXREUSABLEINDEX / 8); i++) {
		data = g_kidManager.bitmap[i];

		if (i == (g_kidManager.index / 8)) {
			j = g_kidManager.index % 8;

		} else {
			j = 0;
		}

		for ( ; j < 8; j++) {
			if (!(data & (1 << j))) {
				data |= 1 << j;
				g_kidManager.bitmap[i] = data;
				g_kidManager.index = ((i * 8) + j);
				id = ((qword)g_kidManager.count << 32) | g_kidManager.index++;
				if (g_kidManager.index == KID_MAXREUSABLEINDEX) {
					g_kidManager.index = 0;
				}

				k_unlock(&g_kidManager.mutex);
				return id;
			}
		}
	}

	for (i = 0; i < (g_kidManager.index / 8); i++) {
		data = g_kidManager.bitmap[i];

		for (j = 0 ; j < 8; j++) {
			if (!(data & (1 << j))) {
				data |= 1 << j;
				g_kidManager.bitmap[i] = data;
				g_kidManager.index = ((i * 8) + j);
				id = ((qword)g_kidManager.count << 32) | g_kidManager.index++;
				if (g_kidManager.index == KID_MAXREUSABLEINDEX) {
					g_kidManager.index = 0;
				}

				k_unlock(&g_kidManager.mutex);
				return id;
			}
		}
	}

	/* not-reusable index */
	if (g_kidManager.overIndex == 0xFFFFFFFF) {
		g_kidManager.overIndex = KID_MAXREUSABLEINDEX;
	}

	id = ((qword)g_kidManager.count << 32) | g_kidManager.overIndex++;

	k_unlock(&g_kidManager.mutex);

	return id;
}

void k_freeKid(qword id) {
	dword index;
	byte data;
	
	k_lock(&g_kidManager.mutex);

	index = GETKIDINDEX(id);
	if (index >= g_kidManager.overIndex) {
		return;
	}

	data = g_kidManager.bitmap[index / 8];
	data &= ~(1 << (index % 8));
	g_kidManager.bitmap[index / 8] = data;

	k_unlock(&g_kidManager.mutex);	
}
#endif