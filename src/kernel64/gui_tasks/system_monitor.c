#include "system_monitor.h"
#include "../core/window.h"
#include "../utils/util.h"
#include "../core/console.h"
#include "../core/mp_config_table.h"
#include "../core/task.h"
#include "../core/dynamic_mem.h"
#include "../fonts/fonts.h"

void k_systemMonitorTask(void) {
	WindowManager* windowManager;
	int processorsHeight;
	int windowWidth;
	int windowHeight;
	qword windowId;
	qword lastTickCount;
	qword lastDynamicMemUsedSize;
	qword dynamicMemUsedSize;
	Event event;
	bool changed;
	int i;
	int processorCount = k_getProcessorCount();
	int lastTaskCounts[processorCount];
	qword lastProcessorLoads[processorCount];

	/* check graphic mode */
	if (k_isGraphicMode() == false) {
		k_printf("[system monitor error] not graphic mode\n");
		return;
	}

	/* create window */
	windowManager = k_getWindowManager();
	processorsHeight = (SYSTEMMONITOR_PROCESSOR_HEIGHT + SYSTEMMONITOR_PROCESSOR_BOTTOMMARGIN) * ((processorCount + 3) / 4);
	windowWidth = (SYSTEMMONITOR_PROCESSOR_WIDTH + SYSTEMMONITOR_PROCESSOR_SIDEMARGIN) * ((processorCount > 4) ? 4 : processorCount) + SYSTEMMONITOR_PROCESSOR_SIDEMARGIN;
	windowHeight = WINDOW_TITLEBAR_HEIGHT + processorsHeight + SYSTEMMONITOR_MEMORY_HEIGHT + 40;

	if (processorCount == 1) {
		windowWidth += 40;
	}

	windowId = k_createWindow(windowManager->screenArea.x2 - windowWidth, windowManager->screenArea.y1 + WINDOW_APPPANEL_HEIGHT, windowWidth, windowHeight, WINDOW_FLAGS_DEFAULT & ~WINDOW_FLAGS_SHOW, "System Monitor");
	if (windowId == WINDOW_INVALIDID) {
		return;
	}

	// draw processor into lines (3 pixels-thick).
	k_drawLine(windowId, 5, WINDOW_TITLEBAR_HEIGHT + 15, windowWidth - 5, WINDOW_TITLEBAR_HEIGHT + 15, RGB(0, 0, 0));
	k_drawLine(windowId, 5, WINDOW_TITLEBAR_HEIGHT + 16, windowWidth - 5, WINDOW_TITLEBAR_HEIGHT + 16, RGB(0, 0, 0));
	k_drawLine(windowId, 5, WINDOW_TITLEBAR_HEIGHT + 17, windowWidth - 5, WINDOW_TITLEBAR_HEIGHT + 17, RGB(0, 0, 0));
	k_drawText(windowId, 15, WINDOW_TITLEBAR_HEIGHT + 8, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "Processor Info", 14);

	// draw memory info lines (3 pixels-thick).
	k_drawLine(windowId, 5, WINDOW_TITLEBAR_HEIGHT + processorsHeight + 40, windowWidth - 5, WINDOW_TITLEBAR_HEIGHT + processorsHeight + 40, RGB(0, 0, 0));
	k_drawLine(windowId, 5, WINDOW_TITLEBAR_HEIGHT + processorsHeight + 41, windowWidth - 5, WINDOW_TITLEBAR_HEIGHT + processorsHeight + 41, RGB(0, 0, 0));
	k_drawLine(windowId, 5, WINDOW_TITLEBAR_HEIGHT + processorsHeight + 42, windowWidth - 5, WINDOW_TITLEBAR_HEIGHT + processorsHeight + 42, RGB(0, 0, 0));
	k_drawText(windowId, 15, WINDOW_TITLEBAR_HEIGHT + processorsHeight + 33, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "Memory Info", 11);

	k_showWindow(windowId, true);

	lastTickCount = 0;
	k_memset(lastTaskCounts, 0, sizeof(lastTaskCounts));
	k_memset(lastProcessorLoads, 0, sizeof(lastProcessorLoads));
	lastDynamicMemUsedSize = 0;

	/* event processing loop */
	while (true) {
		if (k_recvEventFromWindow(&event, windowId) == true) {
			switch (event.type) {
			case EVENT_WINDOW_CLOSE:
				k_deleteWindow(windowId);
				return;

			default:
				break;
			}
		}

		// check system status every 0.5 second.
		if (k_getTickCount() - lastTickCount < 500) {
			k_sleep(1);
			continue;
		}

		lastTickCount = k_getTickCount();

		/* print processor info */
		for (i = 0; i < processorCount; i++) {
			changed = false;

			if (k_getTaskCount(i) != lastTaskCounts[i]) {
				lastTaskCounts[i] = k_getTaskCount(i);
				changed = true;
			}

			if (k_getProcessorLoad(i) != lastProcessorLoads[i]) {
				lastProcessorLoads[i] = k_getProcessorLoad(i);
				changed = true;
			}

			if (changed == true) {
				k_drawProcessorInfo(windowId
					               ,SYSTEMMONITOR_PROCESSOR_WIDTH * (i % 4) + SYSTEMMONITOR_PROCESSOR_SIDEMARGIN * ((i % 4) + 1)
					               ,WINDOW_TITLEBAR_HEIGHT + (SYSTEMMONITOR_PROCESSOR_HEIGHT + SYSTEMMONITOR_PROCESSOR_BOTTOMMARGIN) * (i / 4) + 28
					               ,i);
			}
		}

		/* print memory info */
		k_getDynamicMemInfo(null, null, null, &dynamicMemUsedSize);

		if (dynamicMemUsedSize != lastDynamicMemUsedSize) {
			lastDynamicMemUsedSize = dynamicMemUsedSize;
			k_drawMemoryInfo(windowId
							,WINDOW_TITLEBAR_HEIGHT + processorsHeight + 50
							,windowWidth);
		}
	}
}

static void k_drawProcessorInfo(qword windowId, int x, int y, byte apicId) {
	char buffer[100];
	qword processorLoad; // processor load (processor usage, %)
	qword usageBarHeight;
	int middleX;
	int middleY;
	Rect area;

	/* print core ID and task count */
	k_sprintf(buffer, "- core : %d", apicId);
	k_drawText(windowId, x, y, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, buffer, k_strlen(buffer));
	k_sprintf(buffer, "- task : %d   ", k_getTaskCount(apicId));
	k_drawText(windowId, x, y + 18, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, buffer, k_strlen(buffer));

	/* draw processor load (usage) bar */
	processorLoad = k_getProcessorLoad(apicId);
	if (processorLoad > 100) {
		processorLoad = 100;
	}

	// draw bar border.
	k_drawRect(windowId, x, y + 36, x + SYSTEMMONITOR_PROCESSOR_WIDTH - 1, y + SYSTEMMONITOR_PROCESSOR_HEIGHT - 1, RGB(0, 0, 0), false);

	// usage bar height = total bar height * processor load / 100
	usageBarHeight = (SYSTEMMONITOR_PROCESSOR_HEIGHT - 40) * processorLoad / 100;

	// draw bar (usage/free): put 1 pixel-thick space from border.
	k_drawRect(windowId, x + 2, y + (SYSTEMMONITOR_PROCESSOR_HEIGHT - usageBarHeight) - 3, x + SYSTEMMONITOR_PROCESSOR_WIDTH - 3, y + SYSTEMMONITOR_PROCESSOR_HEIGHT - 3, SYSTEMMONITOR_COLOR_BAR, true);
	k_drawRect(windowId, x + 2, y + 38, x + SYSTEMMONITOR_PROCESSOR_WIDTH - 3, y + (SYSTEMMONITOR_PROCESSOR_HEIGHT - usageBarHeight) - 2, WINDOW_COLOR_BACKGROUND, true);

	// print processor load in the center of bar.
	k_sprintf(buffer, "usage: %d %%", processorLoad);
	middleX = (SYSTEMMONITOR_PROCESSOR_WIDTH - (FONT_DEFAULT_WIDTH * k_strlen(buffer))) / 2;
	middleY = 30 + ((SYSTEMMONITOR_PROCESSOR_HEIGHT - 40) / 2);
	k_drawText(windowId, x + middleX, y + middleY, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, buffer, k_strlen(buffer));

	// update screen.
	k_setRect(&area, x, y, x + SYSTEMMONITOR_PROCESSOR_WIDTH - 1, y + SYSTEMMONITOR_PROCESSOR_HEIGHT - 1);
	k_updateScreenByWindowArea(windowId, &area);
}

static void k_drawMemoryInfo(qword windowId, int y, int windowWidth) {
	char buffer[100];
	qword totalRamSize;        // total RAM size (MB)
	qword dynamicMemStartAddr; // dynamic memory start address (kernel used size)
	qword dynamicMemUsedSize;  // dynamic memory used size
	qword memoryUsage;         // memory usage (%)
	qword usageBarWidth;
	int middleX;
	Rect area;

	totalRamSize = k_getTotalRamSize();
	k_getDynamicMemInfo(&dynamicMemStartAddr, null, null, &dynamicMemUsedSize);

	/* print total size and used size */
	k_sprintf(buffer, "- total : %d MB", totalRamSize);
	k_drawText(windowId, SYSTEMMONITOR_MEMORY_SIDEMARGIN, y + 3, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, buffer, k_strlen(buffer));
	k_sprintf(buffer, "- used  : %d MB", (dynamicMemStartAddr + dynamicMemUsedSize) / 1024 / 1024);
	k_drawText(windowId, SYSTEMMONITOR_MEMORY_SIDEMARGIN, y + 21, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, buffer, k_strlen(buffer));

	/* draw memory usase bar */
	// draw bar border.
	k_drawRect(windowId, SYSTEMMONITOR_MEMORY_SIDEMARGIN, y + 40, windowWidth - SYSTEMMONITOR_MEMORY_SIDEMARGIN, y + SYSTEMMONITOR_MEMORY_HEIGHT - 32, RGB(0, 0, 0), false);

	// memory usage (%) = (kernel used size + dynamic memory used size) * 100 / total RAM size
	memoryUsage = (dynamicMemStartAddr + dynamicMemUsedSize) * 100 / 1024 / 1024 / totalRamSize;
	if (memoryUsage > 100) {
		memoryUsage = 100;
	}

	// usage bar width = total bar width * memory usage / 100
	usageBarWidth = (windowWidth - SYSTEMMONITOR_MEMORY_SIDEMARGIN * 2) * memoryUsage / 100;

	// draw bar (usage/free): put 1 pixel-thick space from border.
	k_drawRect(windowId, SYSTEMMONITOR_MEMORY_SIDEMARGIN + 2, y + 42, SYSTEMMONITOR_MEMORY_SIDEMARGIN + 2 + usageBarWidth, y + SYSTEMMONITOR_MEMORY_HEIGHT - 34, SYSTEMMONITOR_COLOR_BAR, true);
	k_drawRect(windowId, SYSTEMMONITOR_MEMORY_SIDEMARGIN + 2 + usageBarWidth, y + 42, windowWidth - SYSTEMMONITOR_MEMORY_SIDEMARGIN - 2, y + SYSTEMMONITOR_MEMORY_HEIGHT - 34, WINDOW_COLOR_BACKGROUND, true);

	// print memory usage in the center of bar.
	k_sprintf(buffer, "usage: %d %%", memoryUsage);
	middleX = (windowWidth - (FONT_DEFAULT_WIDTH * k_strlen(buffer))) / 2;
	k_drawText(windowId, middleX, y + 45, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, buffer, k_strlen(buffer));

	// update screen.
	k_setRect(&area, 0, y, windowWidth, y + SYSTEMMONITOR_MEMORY_HEIGHT);
	k_updateScreenByWindowArea(windowId, &area);
}