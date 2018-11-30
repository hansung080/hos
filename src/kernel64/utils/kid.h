#ifndef __UTILS_KID_H__
#define __UTILS_KID_H__

#include "../core/types.h"
#include "../core/sync.h"
#include "queue.h"

#define KID_INVALID           0
#define KID_MAXFREEQUEUECOUNT 1024
#define KID_RESETINDEX        1024

#pragma pack(push, 1)

typedef struct k_KidManager {
	Mutex mutex;       // mutex
	dword index;       // index (reusable)
	dword freeIndex;   // free index
	dword count;       // count
	Queue freeQueue;   // free index queue
	dword* freeBuffer; // free index buffer
} KidManager;

#pragma pack(pop)

bool k_initKidManager(void);
qword k_allocKid(void);
void k_freeKid(qword id);

#if 0
/**
  [REF] KID_MAXREUSABLEINDEX must be the multiple of 8.
        If not, bitmap size = (KID_MAXREUSABLEINDEX + 7) / 8
                overIndex = (KID_MAXREUSABLEINDEX + 0x7) & 0xFFFFFFF8
*/
#define KID_INVALID          0
#define KID_MAXREUSABLEINDEX 16 // 8192

/* Macro Function */
// [REF] KID consists of count (high 32 bits) and index (low 32 bits).
#define GETKIDINDEX(id) ((id) & 0xFFFFFFFF) // get index (low 32 bits) of KID (64 bits).

#pragma pack(push, 1)

typedef struct k_KidManager {
	Mutex mutex;     // mutex
	byte* bitmap;    // reusable index bitmap
	dword index;     // reusable index
	dword overIndex; // not-reusable index
	dword count;     // count
} KidManager;

#pragma pack(pop)

bool k_initKidManager(void);
qword k_allocKid(void);
void k_freeKid(qword id);
#endif

#endif // __UTILS_KID_H__