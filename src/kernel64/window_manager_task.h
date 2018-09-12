#ifndef __WINDOW_MANAGER_TASK_H__
#define __WINDOW_MANAGER_TASK_H__

#include "types.h"

// data integration count: It's recommended to be the same number as WINDOW_MAXCOPIEDAREAARRAYCOUNT.
#define WINDOWMANAGER_DATAINTEGRATIONCOUNT 20

void k_startWindowManager(void);
bool k_processMouseData(void);
bool k_processKey(void);
bool k_processWindowManagerEvent(void);

#endif // __WINDOW_MANAGER_TASK_H__