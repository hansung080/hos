#ifndef __TYPES_LIST_H__
#define __TYPES_LIST_H__

#include "types.h"

#pragma pack(push, 1)

typedef struct k_ListLink {
	void* next;
	qword id;
} ListLink;

#pragma pack(pop)

#endif // __TYPES_LIST_H__