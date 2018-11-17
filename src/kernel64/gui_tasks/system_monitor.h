#ifndef __GUITASKS_SYSTEMMONITOR_H__
#define __GUITASKS_SYSTEMMONITOR_H__

#include "../core/types.h"
#include "../core/2d_graphics.h"

#define SYSTEMMONITOR_PROCESSOR_WIDTH        110
#define SYSTEMMONITOR_PROCESSOR_HEIGHT       110
#define SYSTEMMONITOR_PROCESSOR_SIDEMARGIN   10
#define SYSTEMMONITOR_PROCESSOR_BOTTOMMARGIN 10
#define SYSTEMMONITOR_MEMORY_HEIGHT          100
#define SYSTEMMONITOR_MEMORY_SIDEMARGIN      10

#define SYSTEMMONITOR_COLOR_BAR RGB(109, 213, 237)

void k_systemMonitorTask(void);
static void k_drawProcessorInfo(qword windowId, int x, int y, byte apicId);
static void k_drawMemoryInfo(qword windowId, int y, int windowWidth);

#endif // __GUITASKS_SYSTEMMONITOR_H__