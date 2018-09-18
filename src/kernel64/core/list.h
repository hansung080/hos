#ifndef __LIST_H__
#define __LIST_H__

#include "types.h"

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

// General Linked List
typedef struct k_List {
	int count; // item count
	void* head;
	void* tail;
} List;

#pragma pack(pop)

void k_initList(List* list);
int k_getListCount(const List* list);
void k_addListToTail(List* list, void* item);
void k_addListToHead(List* list, void* item);
void* k_removeList(List* list, qword id);
void* k_removeListFromHead(List* list);
void* k_removeListFromTail(List* list);
void* k_findList(const List* list, qword id);
void* k_getHeadFromList(const List* list);
void* k_getTailFromList(const List* list);
void* k_getNextFromList(const List* list, void* current);

#endif // __LIST_H__
