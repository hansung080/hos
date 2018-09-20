#ifndef __SYSTEM_MONITOR_H__
#define __SYSTEM_MONITOR_H__

#include "../core/types.h"
#include "../core/2d_graphics.h"

#define SYSTEMMONITOR_WINDOW_HEIGHT    310
#define SYSTEMMONITOR_PROCESSOR_WIDTH  150
#define SYSTEMMONITOR_PROCESSOR_MARGIN 20
#define SYSTEMMONITOR_PROCESSOR_HEIGHT 150
#define SYSTEMMONITOR_MEMORY_HEIGHT    100

#define SYSTEMMONITOR_COLOR_BAR RGB(55, 215, 47)

void k_systemMonitorTask(void);
static void k_drawProcessorInfo(qword windowId, int x, int y, byte apicId);
static void k_drawMemoryInfo(qword windowId, int y, int windowWidth);

#endif // __SYSTEM_MONITOR_H__