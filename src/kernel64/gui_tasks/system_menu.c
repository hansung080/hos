#include "system_menu.h"
#include "../utils/util.h"
#include "../core/console.h"
#include "../utils/queue.h"
#include "../core/rtc.h"
#include "../core/task.h"
#include "app_panel.h"
#include "shell.h"
#include "alert.h"

Menu* g_systemMenu = null;

void k_systemMenuTask(void) {
	MenuItem systemMenuTable[] = {
		{"hOS", null, true},
		{"Apps", k_funcApps, false},
		{"Shell", k_funcShell, false}
	};

	MenuItem hosMenuTable[] = {
		{"About hOS", k_funcAboutHos, false},
		{"Shutdown", k_funcShutdown, false},
		{"Reboot", k_funcReboot, false}
	};

	MenuItem clockMenuTable[] = {
		{"hh", k_funcClockHh, false},
		{"hh AM", k_funcClockHham, false},
		{"hh:mm", k_funcClockHhmm, false},
		{"hh:mm AM", k_funcClockHhmmam, false},
		{"hh:mm:ss", k_funcClockHhmmss, false},
		{"hh:mm:ss AM", k_funcClockHhmmssam, false}
	};

	Menu systemMenu = {systemMenuTable, sizeof(systemMenuTable) / sizeof(MenuItem)};
	Menu hosMenu = {hosMenuTable, sizeof(hosMenuTable) / sizeof(MenuItem)};
	Menu clockMenu = {clockMenuTable, sizeof(clockMenuTable) / sizeof(MenuItem)};
	WindowManager* windowManager;
	qword windowId;
	Rect itemArea;
	Epoll epoll;
	Queue* equeues[4];
	Clock clock;

	/* check graphic mode */
	if (k_isGraphicMode() == false) {
		k_printf("[system menu error] not graphic mode\n");
		return;
	}

	/* create window */
	windowManager = k_getWindowManager();
	windowId = k_createWindow(0, 0, windowManager->screenArea.x2 + 1, SYSMENU_HEIGHT, 0, SYSMENU_TITLE, SYSMENU_COLOR_BACKGROUND, null, null, WINDOW_INVALIDID);
	if (windowId == WINDOW_INVALIDID) {
		k_printf("[system menu error] window creation failure\n");
		return;
	}

	/* create menus */
	if (k_createMenu(&systemMenu, 10, 0, MENU_ITEMHEIGHT_SYSMENU, null, windowId, null, MENU_FLAGS_HORIZONTAL | MENU_FLAGS_VISIBLE) == false) {
		k_printf("[system menu error] system menu creation failure\n");
		return;
	}

	k_convertRectWindowToScreen(systemMenu.id, &systemMenu.table[0].area, &itemArea);
	if (k_createMenu(&hosMenu, itemArea.x1, itemArea.y2 + 1, MENU_ITEMHEIGHT_NORMAL, null, windowId, &systemMenu, 0) == false) {
		k_printf("[system menu error] system menu creation failure\n");
		return;
	}

	systemMenu.table[0].param = (qword)&hosMenu;
	g_systemMenu = &systemMenu;

	/* draw clock */
	k_setClock(&clock, windowId, windowManager->screenArea.x2 - CLOCK_MAXWIDTH - 10, (SYSMENU_HEIGHT - CLOCK_HEIGHT) / 2, RGB(255, 255, 255), SYSMENU_COLOR_BACKGROUND, CLOCK_FORMAT_HMA, false);
	k_addClock(&clock);

	k_convertRectWindowToScreen(clock.windowId, &clock.area, &itemArea);
	k_createMenu(&clockMenu, itemArea.x1 - MENU_ITEMWIDTHPADDING, itemArea.y2 + 1, MENU_ITEMHEIGHT_NORMAL, null, windowId, null, 0);
	clockMenu.table[0].param = (qword)&clock;
	clockMenu.table[1].param = (qword)&clock;
	clockMenu.table[2].param = (qword)&clock;
	clockMenu.table[3].param = (qword)&clock;
	clockMenu.table[4].param = (qword)&clock;
	clockMenu.table[5].param = (qword)&clock;

	k_showWindow(windowId, true);

	/* initialize epoll */
	equeues[0] = &k_getWindow(windowId)->eventQueue;
	equeues[1] = &k_getWindow(systemMenu.id)->eventQueue;
	equeues[2] = &k_getWindow(hosMenu.id)->eventQueue;
	equeues[3] = &k_getWindow(clockMenu.id)->eventQueue;
	k_initEpoll(&epoll, equeues, 4);

	/* event processing loop */
	while (true) {
		k_waitEpoll(&epoll, null);
		k_processSystemMenuEvent(windowId, &clock, &clockMenu);
		k_processMenuEvent(&systemMenu);
		k_processMenuEvent(&hosMenu);
		k_processMenuEvent(&clockMenu);
	}
	
	g_systemMenu = null;
	k_closeEpoll(&epoll);
	k_removeClock(clock.link.id);
}

static bool k_processSystemMenuEvent(qword windowId, Clock* clock, Menu* clockMenu) {
	Event event;
	MouseEvent* mouseEvent;

	while (true) {
		if (k_recvEventFromWindow(&event, windowId) == false) {
			return false;
		}

		switch (event.type) {
		case EVENT_MOUSE_LBUTTONDOWN:
			mouseEvent = &event.mouseEvent;

			if (k_isPointInRect(&clock->area, mouseEvent->point.x, mouseEvent->point.y) == true) {
				k_toggleMenuVisibility(clockMenu, null, -1);
			}

			break;
		}
	}

	return true;
}

static void k_funcApps(qword parentId) {
	if (g_appPanel == null) {
		k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_SYSTEM | TASK_FLAGS_THREAD, null, 0, (qword)k_appPanelTask, TASK_AFFINITY_LB);

	} else {
		k_togglePanelVisibility(g_appPanel, g_systemMenu, SYSMENU_INDEX_APPS);
	}
}

static void k_funcShell(qword parentId) {
	k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_guiShellTask, TASK_AFFINITY_LB);
}

static void k_funcAboutHos(qword parentId) {

}

static void k_funcShutdown(qword parentId) {

}

static void k_funcReboot(qword parentId) {
	const char* msg = "Do you want to reboot hOS?";
	k_createTaskWithArg(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_alertTask, TASK_AFFINITY_LB, (qword)msg);
}

static void k_funcClockHh(qword clock_) {
	Clock* clock = (Clock*)clock_;

	k_drawRect(clock->windowId, clock->area.x1, clock->area.y1, clock->area.x2, clock->area.y2, clock->backgroundColor, true);
	k_updateScreenByWindowArea(clock->windowId, &clock->area);
	k_setClock(clock, clock->windowId, clock->area.x1, clock->area.y1, clock->textColor, clock->backgroundColor, CLOCK_FORMAT_H, true);
}

static void k_funcClockHham(qword clock_) {
	Clock* clock = (Clock*)clock_;

	k_drawRect(clock->windowId, clock->area.x1, clock->area.y1, clock->area.x2, clock->area.y2, clock->backgroundColor, true);
	k_updateScreenByWindowArea(clock->windowId, &clock->area);
	k_setClock(clock, clock->windowId, clock->area.x1, clock->area.y1, clock->textColor, clock->backgroundColor, CLOCK_FORMAT_HA, true);
}

static void k_funcClockHhmm(qword clock_) {
	Clock* clock = (Clock*)clock_;

	k_drawRect(clock->windowId, clock->area.x1, clock->area.y1, clock->area.x2, clock->area.y2, clock->backgroundColor, true);
	k_updateScreenByWindowArea(clock->windowId, &clock->area);
	k_setClock(clock, clock->windowId, clock->area.x1, clock->area.y1, clock->textColor, clock->backgroundColor, CLOCK_FORMAT_HM, true);
}

static void k_funcClockHhmmam(qword clock_) {
	Clock* clock = (Clock*)clock_;

	k_drawRect(clock->windowId, clock->area.x1, clock->area.y1, clock->area.x2, clock->area.y2, clock->backgroundColor, true);
	k_updateScreenByWindowArea(clock->windowId, &clock->area);
	k_setClock(clock, clock->windowId, clock->area.x1, clock->area.y1, clock->textColor, clock->backgroundColor, CLOCK_FORMAT_HMA, true);
}

static void k_funcClockHhmmss(qword clock_) {
	Clock* clock = (Clock*)clock_;

	k_drawRect(clock->windowId, clock->area.x1, clock->area.y1, clock->area.x2, clock->area.y2, clock->backgroundColor, true);
	k_updateScreenByWindowArea(clock->windowId, &clock->area);
	k_setClock(clock, clock->windowId, clock->area.x1, clock->area.y1, clock->textColor, clock->backgroundColor, CLOCK_FORMAT_HMS, true);
}

static void k_funcClockHhmmssam(qword clock_) {
	Clock* clock = (Clock*)clock_;

	k_drawRect(clock->windowId, clock->area.x1, clock->area.y1, clock->area.x2, clock->area.y2, clock->backgroundColor, true);
	k_updateScreenByWindowArea(clock->windowId, &clock->area);
	k_setClock(clock, clock->windowId, clock->area.x1, clock->area.y1, clock->textColor, clock->backgroundColor, CLOCK_FORMAT_HMSA, true);
}
