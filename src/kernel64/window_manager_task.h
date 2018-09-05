#ifndef __WINDOW_MANAGER_TASK_H__
#define __WINDOW_MANAGER_TASK_H__

#include "types.h"

void k_startWindowManager(void);
bool k_processMouseData(void);
bool k_processKey(void);
bool k_processWindowManagerEvent(void);

#endif // __WINDOW_MANAGER_TASK_H__