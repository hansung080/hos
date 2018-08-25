#include "list.h"

void k_initList(List* list) {
	list->count = 0;
	list->head = null;
	list->tail = null;
}

int k_getListCount(const List* list) {
	return list->count;
}

void k_addListToTail(List* list, void* item) {
	ListLink* link;
	
	// originally (not general), item->link->next = null;
	// changed the code from above to below in order to make ListItem general.
	link = (ListLink*)item; // get ListLink from ListItem pointed by item (This code is possible, because ListLink must be the first field.)
	link->next = null;
	
	// If list items == 0 (list is empty).
	if (list->head == null) {
		list->head = item;
		list->tail = item;
		
	// If list items >= 1.
	} else {
		link = (ListLink*)list->tail;
		link->next = item;
		list->tail = item;
	}
	
	list->count++;
}

void k_addListToHead(List* list, void* item) {
	ListLink* link;
	
	link = (ListLink*)item;
	link->next = list->head;
	
	// If list items == 0 (list is empty).
	if (list->head == null) {
		list->head = item;
		list->tail = item;
		
	// If list items >= 1.
	} else {
		list->head = item;
	}
	
	list->count++;
}

void* k_removeList(List* list, qword id) {
	ListLink* link;
	ListLink* prevLink;
	
	prevLink = (ListLink*)list->head;
	for (link = prevLink; link != null; link = link->next) {
		if (link->id == id) {
			
			// If list items == 1.
			if ((link == list->head) && (link == list->tail)) {
				list->head = null;
				list->tail = null;
				
			// If list items >= 2, and it's the first item.
			} else if (link == list->head) {
				list->head = link->next;
				
			// If list items >= 2, and it's the last item.
			} else if (link == list->tail) {
				list->tail = prevLink;
				
			// If list items >= 3, and it's a middle item.
			} else {
				prevLink->next = link->next;
			}
			
			list->count--;
			return link;
		}
		
		prevLink = link;
	}
	
	return null;
}

void* k_removeListFromHead(List* list) {
	ListLink* link;
	
	// If list items == 0 (list is empty).
	if (list->count == 0) {
		return null;
	}
	
	link = (ListLink*)list->head;
	return k_removeList(list, link->id);
}

void* k_removeListFromTail(List* list) {
	ListLink* link;
	
	// If list items == 0 (list is empty).
	if (list->count == 0) {
		return null;
	}
	
	link = (ListLink*)list->tail;
	return k_removeList(list, link->id);
}

void* k_findList(const List* list, qword id) {
	ListLink* link;
	
	for (link = (ListLink*)list->head; link != null; link = link->next) {
		if (link->id == id) {
			return link;
		}
	}
	
	return null;
}

void* k_getHeadFromList(const List* list) {
	return list->head;
}

void* k_getTailFromList(const List* list) {
	return list->tail;
}

void* k_getNextFromList(const List* list, void* current) {
	ListLink* link;
	
	link = (ListLink*)current;
	
	return link->next;
}
