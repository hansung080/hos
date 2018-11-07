#include "app_panel.h"
#include "base.h"
#include "event_monitor.h"
#include "system_monitor.h"
#include "shell.h"
#include "image_viewer.h"
#include "../core/rtc.h"
#include "../core/task.h"
#include "../utils/util.h"
#include "../core/console.h"

static AppEntry g_appTable[] = {
	#if __DEBUG__
	{"Base", k_baseTask},
	#endif // __DEBUG__
	{"Event Monitor", k_eventMonitorTask},
	{"System Monitor", k_systemMonitorTask},
	{"Shell", k_guiShellTask},
	{"Image Viewer", k_imageViewerTask}
};

static AppPanelManager g_appPanelManager;

void k_appPanelTask(void) {
	/* check graphic mode */
	if (k_isGraphicMode() == false) {
		k_printf("[app panel error] not graphic mode\n");
		return;
	}

	/* create windows */
	if ((k_createAppPanel() == false) || (k_createAppList() == false)) {
		return;
	}

	/* create app list task and digital clock task */
	k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_SYSTEM | TASK_FLAGS_THREAD, null, 0, (qword)k_appListTask, TASK_AFFINITY_LOADBALANCING);
	k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_SYSTEM | TASK_FLAGS_THREAD, null, 0, (qword)k_digitalClockTask, TASK_AFFINITY_LOADBALANCING);

	/* event processing loop */
	while (true) {
		if (k_processAppPanelEvent() == false) {
			k_sleep(0);
		}
	}
}

static void k_appListTask(void) {
	/* event processing loop */
	while (true) {
		if (k_processAppListEvent() == false) {
			k_sleep(0);
		}
	}
}

static void k_digitalClockTask(void) {
	while (true) {
		k_drawDigitalClock(g_appPanelManager.appPanelId);
		k_sleep(1000);
	}
}

static bool k_createAppPanel(void) {
	WindowManager* windowManager;
	qword windowId;

	windowManager = k_getWindowManager();

	windowId = k_createWindow(0, 0, windowManager->screenArea.x2 + 1, APPPANEL_HEIGHT, 0, APPPANEL_TITLE);
	if (windowId == WINDOW_INVALIDID) {
		return false;
	}

	// draw app panel border and background.
	k_drawRect(windowId, 0, 0, windowManager->screenArea.x2, APPPANEL_HEIGHT - 1, APPPANEL_COLOR_OUTERLINE, false);
	k_drawRect(windowId, 1, 1, windowManager->screenArea.x2 - 1, APPPANEL_HEIGHT - 2, APPPANEL_COLOR_MIDDLELINE, false);
	k_drawRect(windowId, 2, 2, windowManager->screenArea.x2 - 2, APPPANEL_HEIGHT - 3, APPPANEL_COLOR_INNERLINE, false);
	k_drawRect(windowId, 3, 3, windowManager->screenArea.x2 - 3, APPPANEL_HEIGHT - 4, APPPANEL_COLOR_BACKGROUND, true);

	// draw app list button.
	k_setRect(&g_appPanelManager.buttonArea, 5, 5, 120, 25);
	k_drawButton(windowId, &g_appPanelManager.buttonArea, APPPANEL_COLOR_BACKGROUND, "Applications", RGB(255, 255, 255));

	// draw digital clock.
	k_drawDigitalClock(windowId);

	k_showWindow(windowId, true);

	g_appPanelManager.appPanelId = windowId;

	return true;
}

static void k_drawDigitalClock(qword windowId) {
	Rect windowArea;
	Rect updateArea;
	static byte prevHour, prevMinute, prevSecond;
	byte hour, minute, second;
	char buffer[9] = "00:00 AM";

	k_readRtcTime(&hour, &minute, &second);

	if ((prevHour == hour) && (prevMinute == minute) && (prevSecond == second)) {
		return;
	}

	prevHour = hour;
	prevMinute = minute;
	prevSecond = second;

	if (hour >= 12) {
		if (hour > 12) {
			hour -= 12;
		}
		
		buffer[6] = 'P';
	}

	buffer[0] = hour / 10 + '0';
	buffer[1] = hour % 10 + '0';
	buffer[3] = minute / 10 + '0';
	buffer[4] = minute % 10 + '0';
	
	#if __DEBUG__
	if ((second % 2) == 1) {
		buffer[2] = ' ';

	} else {
		buffer[2] = ':';
	}
	#endif // __DEBUG__

	k_getWindowArea(windowId, &windowArea);
	k_setRect(&updateArea, windowArea.x2 - APPPANEL_CLOCKWIDTH - 13, 5, windowArea.x2 - 5, 25);
	k_drawRect(windowId, updateArea.x1, updateArea.y1, updateArea.x2, updateArea.y2, APPPANEL_COLOR_INNERLINE, false);
	k_drawText(windowId, updateArea.x1 + 4, updateArea.y1 + 3, RGB(255, 255, 255), APPPANEL_COLOR_BACKGROUND, buffer, k_strlen(buffer));
	k_updateScreenByWindowArea(windowId, &updateArea);
}

static bool k_processAppPanelEvent(void) {
	Event event;
	MouseEvent* mouseEvent;

	if (k_recvEventFromWindow(&event, g_appPanelManager.appPanelId) == false) {
		return false;
	}

	switch (event.type) {
	case EVENT_MOUSE_LBUTTONDOWN:
		mouseEvent = &event.mouseEvent;

		if (k_isPointInRect(&g_appPanelManager.buttonArea, mouseEvent->point.x, mouseEvent->point.y) == false) {
			break;
		}

		// app list button (toggle button): up -> down
		if (g_appPanelManager.appListVisible == false) {
			k_drawButton(g_appPanelManager.appPanelId, &g_appPanelManager.buttonArea, APPPANEL_COLOR_ACTIVE, "Applications", RGB(255, 255, 255));
			k_updateScreenByWindowArea(g_appPanelManager.appPanelId, &g_appPanelManager.buttonArea);

			if (g_appPanelManager.prevMouseOverIndex != -1) {
				k_drawAppItem(g_appPanelManager.prevMouseOverIndex, false);
				g_appPanelManager.prevMouseOverIndex = -1;
			}

			k_moveWindowToTop(g_appPanelManager.appListId);
			k_showWindow(g_appPanelManager.appListId, true);

			g_appPanelManager.appListVisible = true;

		// app list button (toggle button): down -> up
		} else {
			k_drawButton(g_appPanelManager.appPanelId, &g_appPanelManager.buttonArea, APPPANEL_COLOR_BACKGROUND, "Applications", RGB(255, 255, 255));
			k_updateScreenByWindowArea(g_appPanelManager.appPanelId, &g_appPanelManager.buttonArea);

			k_showWindow(g_appPanelManager.appListId, false);

			g_appPanelManager.appListVisible = false;
		}

		break;

	default:
		break;
	}

	return true;
}

static bool k_createAppList(void) {
	int i;
	int count;
	int maxNameLen;
	int nameLen;
	qword windowId;	
	int x, y;
	int windowWidth;

	maxNameLen = 0;
	count = sizeof(g_appTable) / sizeof(AppEntry);
	for (i = 0; i < count; i++) {
		nameLen = k_strlen(g_appTable[i].name);
		if (nameLen > maxNameLen) {
			maxNameLen = nameLen;
		}
	}

	windowWidth = FONT_DEFAULT_WIDTH * maxNameLen + 20;
	x = g_appPanelManager.buttonArea.x1;
	y = g_appPanelManager.buttonArea.y2 + 5;

	windowId = k_createWindow(x, y, windowWidth, APPLIST_ITEMHEIGHT * count, 0, APPLIST_TITLE);
	if (windowId == WINDOW_INVALIDID) {
		return false;
	}

	g_appPanelManager.appListWidth = windowWidth;
	g_appPanelManager.appListVisible = false;
	g_appPanelManager.appListId = windowId;
	g_appPanelManager.prevMouseOverIndex = -1;

	for (i = 0; i < count; i++) {
		k_drawAppItem(i, false);
	}

	k_moveWindow(windowId, x, y);

	return true;
}

static void k_drawAppItem(int index, bool mouseOver) {
	Color color;
	Rect itemArea;

	if (mouseOver == true) {
		color = APPPANEL_COLOR_ACTIVE;

	} else {
		color = APPPANEL_COLOR_BACKGROUND;
	}

	// draw app item border and background.
	k_setRect(&itemArea, 0, APPLIST_ITEMHEIGHT * index, g_appPanelManager.appListWidth - 1, APPLIST_ITEMHEIGHT * (index + 1) - 1);
	k_drawRect(g_appPanelManager.appListId, itemArea.x1, itemArea.y1, itemArea.x2, itemArea.y2, APPPANEL_COLOR_INNERLINE, false);
	k_drawRect(g_appPanelManager.appListId, itemArea.x1 + 1, itemArea.y1 + 1, itemArea.x2 - 1, itemArea.y2 - 1, color, true);

	// draw app name.
	k_drawText(g_appPanelManager.appListId, itemArea.x1 + 10, itemArea.y1 + 2, RGB(255, 255, 255), color, g_appTable[index].name, k_strlen(g_appTable[index].name));

	k_updateScreenByWindowArea(g_appPanelManager.appListId, &itemArea);
}

static bool k_processAppListEvent(void) {
	Event event;
	MouseEvent* mouseEvent;
	int mouseOverIndex;

	if (k_recvEventFromWindow(&event, g_appPanelManager.appListId) == false) {
		return false;
	}

	switch (event.type) {
	case EVENT_MOUSE_MOVE:
		mouseEvent = &event.mouseEvent;

		mouseOverIndex = k_getMouseOverItemIndex(mouseEvent->point.y);
		if ((mouseOverIndex == -1) || (mouseOverIndex == g_appPanelManager.prevMouseOverIndex)) {
			break;
		}

		if (g_appPanelManager.prevMouseOverIndex != -1) {
			k_drawAppItem(g_appPanelManager.prevMouseOverIndex, false);
		}

		k_drawAppItem(mouseOverIndex, true);

		g_appPanelManager.prevMouseOverIndex = mouseOverIndex;

		break;

	case EVENT_MOUSE_LBUTTONDOWN:
		mouseEvent = &event.mouseEvent;

		mouseOverIndex = k_getMouseOverItemIndex(mouseEvent->point.y);
		if (mouseOverIndex == -1) {
			break;
		}

		k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, null, 0, (qword)g_appTable[mouseOverIndex].entryPoint, TASK_AFFINITY_LOADBALANCING);

		// send mouse event in oder to make app list invisible.
		k_sendMouseEventToWindow(g_appPanelManager.appPanelId, EVENT_MOUSE_LBUTTONDOWN, g_appPanelManager.buttonArea.x1, g_appPanelManager.buttonArea.y1, 0);

		break;

	default:
		break;
	}
	
	return true;
}

static int k_getMouseOverItemIndex(int mouseY) {
	int index;

	index = mouseY / APPLIST_ITEMHEIGHT;
	if ((index < 0) || (index >= (sizeof(g_appTable) / sizeof(AppEntry)))) {
		return -1;
	}

	return index;
}