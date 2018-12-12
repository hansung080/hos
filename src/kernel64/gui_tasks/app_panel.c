#include "app_panel.h"
#include "../utils/util.h"
#include "../core/console.h"
#include "../fonts/fonts.h"
#include "../core/task.h"
#include "../core/app_manager.h"
#include "system_menu.h"
#include "prototype.h"
#include "event_monitor.h"
#include "system_monitor.h"
#include "shell.h"
#include "image_viewer.h"
#include "color_picker.h"

static PanelItem g_appPanelTable[APPPANEL_MAXITEMCOUNT] = {
	{"Prototype", k_funcSystemApp, APPPANEL_COLOR_ITEM_PROTOTYPE},
	{"Event Monitor", k_funcSystemApp, APPPANEL_COLOR_ITEM_EVENTMONITOR},
	{"System Monitor", k_funcSystemApp, APPPANEL_COLOR_ITEM_SYSTEMMONITOR},
	{"Shell", k_funcSystemApp, APPPANEL_COLOR_ITEM_SHELL},
	{"Image Viewer", k_funcSystemApp, APPPANEL_COLOR_ITEM_IMAGEVIEWER},
	{"Color Picker", k_funcSystemApp, APPPANEL_COLOR_ITEM_COLORPICKER},
	{"Test0", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER0},
	{"Test1", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER1},
	{"Test2", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER2},
	{"Test3", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER3},
	{"Test4", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER4},
	{"Test5", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER5},
	{"Test6", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER6},
	{"Test7", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER7},
	{"Test8", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER8},
	{"Test9", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER9},
	{"Test10", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER10},
	{"Test11", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER11},
	{"Test12", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER12},
	{"Test13", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER13},
	{"Test14", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER14},
	{"Test15", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER15},
	{"Test16", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER16},
	{"Test17", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER17},
	{"Test18", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER18},
	{"Test19", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER19},
	{"Test20", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER20},
	{"Test21", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER21},
	{"Test22", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER22},
	{"Test23", k_funcSystemApp, APPPANEL_COLOR_ITEM_USER23}
};

Panel* g_appPanel = null;

void k_appPanelTask(void) {
	Panel appPanel = {g_appPanelTable, APPPANEL_SYSTEMAPPCOUNT};
	Rect screenArea;
	int screenWidth, screenHeight;
	Event event;
	KeyEvent* keyEvent;

	/* check graphic mode */
	if (k_isGraphicMode() == false) {
		k_printf("[app panel error] not graphic mode\n");
		return;
	}

	if (g_appPanel != null) {
		k_moveWindowToTop(g_appPanel->id);
		return;
	}
	
	/* create app panel */
	k_getScreenArea(&screenArea);
	screenWidth = k_getRectWidth(&screenArea);
	screenHeight = k_getRectHeight(&screenArea);

	if (k_createPanel(&appPanel, 0, WINDOW_SYSMENU_HEIGHT, screenWidth, screenHeight - WINDOW_SYSMENU_HEIGHT, APPPANEL_ITEMSIZE, APPPANEL_ITEMMARGIN, null, PANEL_FLAGS_DRAWLOGO | PANEL_FLAGS_BLOCKING | PANEL_FLAGS_VISIBLE | PANEL_FLAGS_COPYWITHOUTPARAM | PANEL_FLAGS_APPPANEL) == false) {
		k_printf("[app panel error] app panel creation failure\n");
		return;
	}

	appPanel.table[0].param = (qword)k_prototypeTask;
	appPanel.table[1].param = (qword)k_eventMonitorTask;
	appPanel.table[2].param = (qword)k_systemMonitorTask;
	appPanel.table[3].param = (qword)k_guiShellTask;
	appPanel.table[4].param = (qword)k_imageViewerTask;
	appPanel.table[5].param = (qword)k_colorPickerTask;

	int i;
	for (i = 6; i < appPanel.itemCount; i++) {
		appPanel.table[i].param = (qword)k_eventMonitorTask;
	}

	g_appPanel = &appPanel;

	/* event processing loop */
	while (true) {
		k_processPanelEvent(&appPanel);
	}

	g_appPanel = null;
}

bool k_createPanel(Panel* panel, int x, int y, int width, int height, int itemSize, int itemMargin, Color* colors, dword flags) {
	dword optionalFlags = 0;
	int originX, originY;
	int radius;
	int i;

	if (colors == null) {
		panel->textColor = PANEL_COLOR_TEXT;
		panel->backgroundColor = PANEL_COLOR_BACKGROUND;
		panel->activeColor = PANEL_COLOR_ACTIVE;

	} else {
		panel->textColor = colors[0];
		panel->backgroundColor = colors[1];
		panel->activeColor = colors[2];
	}

	if (flags & PANEL_FLAGS_BLOCKING) {
		optionalFlags |= WINDOW_FLAGS_BLOCKING;
	}

	if (flags & PANEL_FLAGS_VISIBLE) {
		optionalFlags |= WINDOW_FLAGS_VISIBLE;
	}

	panel->id = k_createWindow(x, y, width, height, WINDOW_FLAGS_PANEL | optionalFlags, PANEL_TITLE, panel->backgroundColor, null, panel, WINDOW_INVALIDID);
	if (panel->id == WINDOW_INVALIDID) {
		return false;
	}

	if (flags & PANEL_FLAGS_DRAWLOGO) {
		k_drawHansLogo(panel->id, (width - PANEL_LOGOWIDTH) / 2, ((height + WINDOW_SYSMENU_HEIGHT - PANEL_LOGOHEIGHT) / 2) - WINDOW_SYSMENU_HEIGHT, PANEL_LOGOWIDTH, PANEL_LOGOHEIGHT, PANEL_COLOR_LOGOBRIGHT, PANEL_COLOR_LOGODARK);
	}

	if (flags & PANEL_FLAGS_VISIBLE) {
		k_showWindow(panel->id, true);
		panel->visible = true;
	}

	panel->width = width;
	panel->height = height;
	panel->itemSize = itemSize;
	panel->itemMargin = itemMargin;
	panel->prevIndex = -1;
	panel->flags = flags;

	radius = itemSize / 2;
	originX = itemMargin + radius;
	originY = itemMargin + radius;
	panel->columns = 1;
	panel->rows = 1;

	for (i = 0; i < panel->itemCount; i++) {
		panel->table[i].param = panel->id;
		k_setCircle(&panel->table[i].area, originX, originY, radius);
		k_drawPanelItem(panel, i, false);

		originX += (itemMargin + radius) * 2;
		if ((originX + radius) >= width) {
			originX = itemMargin + radius;
			originY += (itemMargin + radius) * 2;
			if (panel->rows == 1) {
				panel->columns = i + 1;
			}

			panel->rows++;
		}
	}

	return true;
}

bool k_addPanelItem(Panel* panel, const char* name, PanelFunc func, Color color, qword param) {
	int originX, originY;
	int radius;

	if (panel->itemCount >= APPPANEL_MAXITEMCOUNT) {
		return false;
	}

	/* add item to panel table */
	k_strcpy(panel->table[panel->itemCount].name, name);
	panel->table[panel->itemCount].func = func;
	panel->table[panel->itemCount].color = color;
	panel->table[panel->itemCount].param = param;
	
	/* calculate item area */	
	originX = panel->table[panel->itemCount - 1].area.x;
	originY = panel->table[panel->itemCount - 1].area.y;
	radius = panel->itemSize / 2;

	originX += (panel->itemMargin + radius) * 2;
	if ((originX + radius) >= panel->width) {
		originX = panel->itemMargin + radius;
		originY += (panel->itemMargin + radius) * 2;
		if (panel->rows == 1) {
			panel->columns = panel->itemCount + 1;
		}

		panel->rows++;
	}

	k_setCircle(&panel->table[panel->itemCount].area, originX, originY, radius);

	/* draw panel item */
	k_drawPanelItem(panel, panel->itemCount, false);

	/* increase item count */
	panel->itemCount++;

	return true;
}

bool k_removePanelItem(Panel* panel, const char* name) {
	int i;
	int index = -1;
	const Circle* itemArea;
	Rect clearArea;

	/* find remove index by name */
	for (i == 0; i < panel->itemCount; i++) {
		if (k_strcmp(panel->table[i].name, name) == 0) {
			index = i;
			break;
		}
	}

	if (index == -1) {
		return false;
	}

	/* decrease item count */
	panel->itemCount--;

	/* draw panel items */
	for (i = index; i < panel->itemCount; i++) {
		// copy panel item without area.
		k_strcpy(panel->table[i].name, panel->table[i + 1].name);
		panel->table[i].func = panel->table[i + 1].func;
		panel->table[i].color = panel->table[i + 1].color;
		if ((panel->flags & PANEL_FLAGS_COPYWITHOUTPARAM) != PANEL_FLAGS_COPYWITHOUTPARAM) {
			panel->table[i].param = panel->table[i + 1].param;	
		}	

		// draw panel item.
		k_drawPanelItem(panel, i, false);
	}

	/* clear last panel item */
	itemArea = &panel->table[panel->itemCount].area;
	k_setRect(&clearArea, itemArea->x - itemArea->radius, itemArea->y - itemArea->radius, itemArea->x + itemArea->radius, itemArea->y + itemArea->radius);
	k_drawRect(panel->id, clearArea.x1, clearArea.y1, clearArea.x2, clearArea.y2, panel->backgroundColor, true);	
	k_updateScreenByWindowArea(panel->id, &clearArea);

	return true;
}

bool k_processPanelEvent(Panel* panel) {
	Event event;
	MouseEvent* mouseEvent;
	KeyEvent* keyEvent;

	while (true) {
		if (k_recvEventFromWindow(&event, panel->id) == false) {
			return false;
		}

		switch (event.type) {
		case EVENT_MOUSE_MOVE:
			mouseEvent = &event.mouseEvent;
			k_processPanelActivity(panel, k_getPanelItemIndex(panel, mouseEvent->point.x, mouseEvent->point.y));
			break;

		case EVENT_MOUSE_LBUTTONDOWN:
			mouseEvent = &event.mouseEvent;
			k_processPanelFunction(panel, k_getPanelItemIndex(panel, mouseEvent->point.x, mouseEvent->point.y));
			break;

		case EVENT_MOUSE_OUT:
			k_clearPanelActivity(panel);
			break;

		case EVENT_KEY_DOWN:
			keyEvent = &event.keyEvent;


			switch (keyEvent->asciiCode) {
			case KEY_RIGHT:
				if (panel->prevIndex + 1 < panel->itemCount) {
					k_processPanelActivity(panel, panel->prevIndex + 1);
				}
				
				break;

			case KEY_LEFT:
				if (panel->prevIndex == -1) {
					k_processPanelActivity(panel, panel->itemCount - 1);

				} else {
					if (panel->prevIndex - 1 >= 0) {
						k_processPanelActivity(panel, panel->prevIndex - 1);
					}	
				}

				break;

			case KEY_DOWN:
				if (panel->prevIndex == -1) {
					k_processPanelActivity(panel, 0);

				} else {
					if (panel->prevIndex + panel->columns < panel->itemCount) {
						k_processPanelActivity(panel, panel->prevIndex + panel->columns);
					}
				}
				
				break;

			case KEY_UP:
				if (panel->prevIndex == -1) {
					k_processPanelActivity(panel, panel->itemCount - 1);

				} else {
					if (panel->prevIndex - panel->columns >= 0) {
						k_processPanelActivity(panel, panel->prevIndex - panel->columns);
					}
				}

				break;

			case KEY_ENTER:
				k_processPanelFunction(panel, panel->prevIndex);
				break;

			case KEY_ESC:
				if (panel->flags & PANEL_FLAGS_APPPANEL) {
					k_changePanelVisibility(panel, g_systemMenu, SYSMENU_INDEX_APPS, false);

				} else {
					k_changePanelVisibility(panel, null, -1, false);
				}

				break;
			}

			break;
		}
	}

	return true;
}

static void k_processPanelActivity(Panel* panel, int index) {
	if ((index == -1) || (index == panel->prevIndex)) {
		return;
	}

	if (panel->prevIndex != -1) {
		k_drawPanelItem(panel, panel->prevIndex, false);
	}

	k_drawPanelItem(panel, index, true);

	panel->prevIndex = index;
}

static void k_processPanelFunction(Panel* panel, int index) {
	if (index == -1) {
		return;
	}

	if (panel->flags & PANEL_FLAGS_APPPANEL) {
		k_changePanelVisibility(panel, g_systemMenu, SYSMENU_INDEX_APPS, false);

	} else {
		k_changePanelVisibility(panel, null, -1, false);
	}

	if (panel->table[index].func != null) {
		panel->table[index].func(panel->table[index].param);
	}
}

static void k_clearPanelActivity(Panel* panel) {
	if (panel->prevIndex != -1) {
		k_drawPanelItem(panel, panel->prevIndex, false);
		panel->prevIndex = -1;
	}
}

void k_changePanelVisibility(Panel* panel, Menu* menu, int index, bool visible) {
	if (visible == true) {
		if ((menu != null) && (index != -1)) {
			k_drawMenuItem(menu, index, true, menu->textColor, menu->backgroundColor, menu->activeColor);
			menu->prevIndex = index;
		}

		if (panel->prevIndex != -1) {
			k_drawPanelItem(panel, panel->prevIndex, false);
			panel->prevIndex = -1;
		}

		k_moveWindowToTop(panel->id);
		k_showWindow(panel->id, true);
		panel->visible = true;

	} else {
		if ((menu != null) && (index != -1)) {
			k_drawMenuItem(menu, index, false, menu->textColor, menu->backgroundColor, menu->activeColor);
		}

		k_showWindow(panel->id, false);
		panel->visible = false;
	}	
}

void k_togglePanelVisibility(Panel* panel, Menu* menu, int index) {
	if (panel->visible == false) {
		if ((menu != null) && (index != -1)) {
			k_drawMenuItem(menu, index, true, menu->textColor, menu->backgroundColor, menu->activeColor);
			menu->prevIndex = index;
		}

		if (panel->prevIndex != -1) {
			k_drawPanelItem(panel, panel->prevIndex, false);
			panel->prevIndex = -1;
		}

		k_moveWindowToTop(panel->id);
		k_showWindow(panel->id, true);
		panel->visible = true;

	} else {
		if ((menu != null) && (index != -1)) {
			k_drawMenuItem(menu, index, false, menu->textColor, menu->backgroundColor, menu->activeColor);
		}

		k_showWindow(panel->id, false);
		panel->visible = false;
	}
}

int k_getPanelItemIndex(const Panel* panel, int mouseX, int mouseY) {
	int index;
	int itemSize; // item size with margin

	itemSize = panel->itemSize + (panel->itemMargin * 2);
	index = (mouseY / itemSize) * panel->columns + (mouseX / itemSize);
	if ((index < 0) || (index >= panel->itemCount)) {
		return -1;
	}

	if (k_isPointInCircle(&panel->table[index].area, mouseX, mouseY) == false) {
		return -1;
	}

	return index;
}

bool k_drawPanelItem(const Panel* panel, int index, bool active) {
	Window* window;
	Rect area;
	const Circle* itemArea;
	Rect updateArea;
	Color textColor;
	char name1[FS_MAXFILENAMELENGTH] = {'\0', };
	char name2[FS_MAXFILENAMELENGTH] = {'\0', };
	int len1;
	int len2;

	window = k_getWindowWithLock(panel->id);
	if (window == null) {
		return false;
	}

	// set clipping area on window coordinates.
	k_setRect(&area, 0, 0, window->area.x2 - window->area.x1, window->area.y2 - window->area.y1);

	splitPanelItemName(panel->table[index].name, name1, name2);

	itemArea = &panel->table[index].area;
	len1 = k_strlen(name1);
	
	__k_drawCircle(window->buffer, &area, itemArea->x, itemArea->y, itemArea->radius, panel->table[index].color, true);

	if (active == true) {
		textColor = panel->activeColor;
		__k_drawCircle(window->buffer, &area, itemArea->x, itemArea->y, itemArea->radius, panel->activeColor, false);
		__k_drawCircle(window->buffer, &area, itemArea->x, itemArea->y, itemArea->radius - 1, panel->activeColor, false);

	} else {
		textColor = panel->textColor;
	}

	if (name2[0] == '\0') {
		__k_drawText(window->buffer, &area, itemArea->x - ((FONT_DEFAULT_WIDTH * len1) / 2), itemArea->y - (FONT_DEFAULT_HEIGHT / 2), textColor, panel->table[index].color, name1, len1);

	} else {
		len2 = k_strlen(name2);
		__k_drawText(window->buffer, &area, itemArea->x - ((FONT_DEFAULT_WIDTH * len1) / 2), itemArea->y - FONT_DEFAULT_HEIGHT - 1, textColor, panel->table[index].color, name1, len1);
		__k_drawText(window->buffer, &area, itemArea->x - ((FONT_DEFAULT_WIDTH * len2) / 2), itemArea->y + 1, textColor, panel->table[index].color, name2, len2);
	}
		
	k_unlock(&window->mutex);

	k_setRect(&updateArea, itemArea->x - itemArea->radius, itemArea->y - itemArea->radius, itemArea->x + itemArea->radius, itemArea->y + itemArea->radius);
	k_updateScreenByWindowArea(panel->id, &updateArea);
}

static void splitPanelItemName(const char* name, char* name1, char* name2) {
	int i, j;

	for (i = 0; name[i] != '\0'; i++) {
		if ((name[i] == ' ') || (name[i] == '-') || (name[i] == '_') || (name[i] == '.')) {
			break;
		}

		name1[i] = name[i];
	}

	name1[i] = '\0';

	if ((name[i] != '.') && (name[i] != '\0')) {
		j = 0;
		for ( ; name[i] != '\0'; i++) {
			if (name[i] == '.') {
				break;
			}

			name2[j++] = name[i];
		}

		name2[j] = '\0';
	}
}

static Color g_userAppColors[] = {
	APPPANEL_COLOR_ITEM_USER0,
	APPPANEL_COLOR_ITEM_USER1,
	APPPANEL_COLOR_ITEM_USER2,
	APPPANEL_COLOR_ITEM_USER3,
	APPPANEL_COLOR_ITEM_USER4,
	APPPANEL_COLOR_ITEM_USER5,
	APPPANEL_COLOR_ITEM_USER6,
	APPPANEL_COLOR_ITEM_USER7,
	APPPANEL_COLOR_ITEM_USER8,
	APPPANEL_COLOR_ITEM_USER9,
	APPPANEL_COLOR_ITEM_USER10,
	APPPANEL_COLOR_ITEM_USER11,
	APPPANEL_COLOR_ITEM_USER12,
	APPPANEL_COLOR_ITEM_USER13,
	APPPANEL_COLOR_ITEM_USER14,
	APPPANEL_COLOR_ITEM_USER15,
	APPPANEL_COLOR_ITEM_USER16,
	APPPANEL_COLOR_ITEM_USER17,
	APPPANEL_COLOR_ITEM_USER18,
	APPPANEL_COLOR_ITEM_USER19,
	APPPANEL_COLOR_ITEM_USER20,
	APPPANEL_COLOR_ITEM_USER21,
	APPPANEL_COLOR_ITEM_USER22,
	APPPANEL_COLOR_ITEM_USER23
};

Color k_getUserAppColor(void) {
	static int index = 0;

	if (index >= (sizeof(g_userAppColors) / sizeof(Color))) {
		index = 0;
	}

	return g_userAppColors[index++];
}

static void k_funcSystemApp(qword entryPoint) {
	k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, null, 0, entryPoint, TASK_AFFINITY_LB);
}

void k_funcUserApp(qword fileName_) {
	k_executeApp((char*)fileName_, null, TASK_AFFINITY_LB);
}
