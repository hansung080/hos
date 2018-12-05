#ifndef __GUITASKS_EVENTMONITOR_H__
#define __GUITASKS_EVENTMONITOR_H__

#include "../core/types.h"
#include "../core/2d_graphics.h"

// title
#define EVTMTR_TITLE "Event Monitor"

// color
#define EVTMTR_COLOR_BUTTONACTIVE RGB(109, 213, 237)

// user event: set bit 31 to 1 in order not to duplicate with the already defined events.
#define EVENT_USER_TESTMESSAGE 0x80000001

void k_eventMonitorTask(void);

#endif // __GUITASKS_EVENTMONITOR_H__