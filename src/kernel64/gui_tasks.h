#ifndef __GUI_TASKS_H__
#define __GUI_TASKS_H__

#include "types.h"

// user event: set bit 31 to 1 in order not to duplicate with the already defined events.
#define EVENT_USER_TESTMESSAGE 0x80000001

void k_baseGuiTask(void);
void k_helloWorldGuiTask(void);

#endif // __GUI_TASKS_H__