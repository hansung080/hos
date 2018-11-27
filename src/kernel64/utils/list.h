#ifndef __UTILS_LIST_H__
#define __UTILS_LIST_H__

#include "../core/types.h"

#pragma pack(push, 1)

typedef struct k_ListLink {
	void* next;
	qword id;
} ListLink;

/**
  < List Item Definition Example >
      typedef struct k_ListItem {
          ListLink link; // [NOTE] ListLink must be the first field.
          char data1;
          int data2;
      } ListItem; // ListItem is a node of list.
*/

// general linked list
typedef struct k_List {
	int count; // item count
	void* head;
	void* tail;
} List;

#pragma pack(pop)

void k_initList(List* list);
int k_getListCount(const List* list);
void k_addListToHead(List* list, void* item);
void k_addListToTail(List* list, void* item);
void* k_removeListById(List* list, qword id);
void* k_removeListFromHead(List* list);
void* k_removeListFromTail(List* list);
void* k_findListById(const List* list, qword id);
void* k_getHeadFromList(const List* list);
void* k_getTailFromList(const List* list);
void* k_getNextFromList(const List* list, void* current);

#endif // __UTILS_LIST_H__
