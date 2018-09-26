#ifndef __CORE_WINDOWMANAGER_H__
#define __CORE_WINDOWMANAGER_H__

#include "types.h"

// data integration count: It's recommended to be the same number as WINDOW_MAXCOPIEDAREAARRAYCOUNT.
#define WINDOWMANAGER_DATAINTEGRATIONCOUNT 20

void k_windowManagerTask(void);
bool k_processMouseData(void);
bool k_processKey(void);
bool k_processWindowManagerEvent(void);

#endif // __CORE_WINDOWMANAGER_H__