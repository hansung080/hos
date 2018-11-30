#include "widgets.h"
#include "window.h"
#include "../utils/util.h"
#include "rtc.h"
#include "../utils/kid.h"

bool k_createMenu(Menu* menu, int x, int y, int itemHeight, Color* colors, qword parentId, Menu* top, dword flags) {
	int i;
	int nameLen;
	int maxNameLen = 0;
	int pixelLen;
	int totalPixelLen = 0;
	int windowWidth;
	int windowHeight;
	dword optionalFlags = 0;
	qword windowId;
	int offsetX = 0;
	int offsetY = 0;

	// (x, y) is screen coordinates without MENU_FLAGS_DRAWONPARENT.
	// (x, y) is parent window coordinates with MENU_FLAGS_DRAWONPARENT.
	if (flags & MENU_FLAGS_DRAWONPARENT) {
		offsetX = x;
		offsetY = y;
	}

	menu->itemHeight = itemHeight;
	menu->itemHeightPadding = (itemHeight - FONT_DEFAULT_HEIGHT) / 2;

	for (i = 0; i < menu->itemCount; i++) {
		nameLen = k_strlen(menu->table[i].name);

		if (flags & MENU_FLAGS_HORIZONTAL) {
			pixelLen = (FONT_DEFAULT_WIDTH * nameLen) + (MENU_ITEMWIDTHPADDING * 2);
			k_setRect(&menu->table[i].area, offsetX + totalPixelLen, offsetY, offsetX + totalPixelLen + pixelLen - 1, offsetY + itemHeight - 1);
			totalPixelLen += pixelLen;

		} else {
			if (nameLen > maxNameLen) {
				maxNameLen = nameLen;
			}
		}
	}

	if (flags & MENU_FLAGS_HORIZONTAL) {
		windowWidth = totalPixelLen;
		windowHeight = itemHeight;

	} else {
		windowWidth = (FONT_DEFAULT_WIDTH * maxNameLen) + (MENU_ITEMWIDTHPADDING * 2);
		windowHeight = itemHeight * menu->itemCount;	
	}

	if (colors == null) {
		menu->textColor = MENU_COLOR_TEXT;
		menu->backgroundColor = MENU_COLOR_BACKGROUND;
		menu->activeColor = MENU_COLOR_ACTIVE;

	} else {
		menu->textColor = colors[0];
		menu->backgroundColor = colors[1];
		menu->activeColor = colors[2];
	}

	if (flags & MENU_FLAGS_DRAWONPARENT) {
		windowId = parentId;

	} else {
		if (flags & MENU_FLAGS_BLOCKING) {
			optionalFlags |= WINDOW_FLAGS_BLOCKING;
		}
		
		if (flags & MENU_FLAGS_VISIBLE) {
			optionalFlags |= WINDOW_FLAGS_VISIBLE;
		}

		windowId = k_createWindow(x, y, windowWidth, windowHeight, WINDOW_FLAGS_CHILD | WINDOW_FLAGS_MENU | optionalFlags, MENU_TITLE, menu->backgroundColor, null, parentId);
		if (windowId == WINDOW_INVALIDID) {
			return false;
		}	
	}
	
	menu->id = windowId;
	menu->visible = false;
	menu->prevIndex = -1;
	menu->parentId = parentId;
	menu->top = top;
	menu->flags = flags;
	
	for (i = 0; i < menu->itemCount; i++) {
		menu->table[i].param = parentId;

		if ((flags & MENU_FLAGS_HORIZONTAL) != MENU_FLAGS_HORIZONTAL) {
			k_setRect(&menu->table[i].area, offsetX, offsetY + itemHeight * i, offsetX + windowWidth - 1, offsetY + itemHeight * (i + 1) - 1);
		}

		k_drawMenuItem(menu, i, false, menu->textColor, menu->backgroundColor, menu->activeColor);
	}

	if (flags & MENU_FLAGS_VISIBLE) {
		k_moveWindowToTop(windowId);
		k_showWindow(windowId, true);
		menu->visible = true;
	}

	return true;
}

bool k_processMenuEvent(Menu* menu) {
	Event event;
	MouseEvent* mouseEvent;
	Rect menuArea;

	while (true) {
		if (k_recvEventFromWindow(&event, menu->id) == false) {
			return false;
		}

		switch (event.type) {
		case EVENT_MOUSE_MOVE:
			mouseEvent = &event.mouseEvent;
			k_processMenuActivity(menu, mouseEvent->point.x, mouseEvent->point.y);
			break;

		case EVENT_MOUSE_LBUTTONDOWN:
			mouseEvent = &event.mouseEvent;
			k_processMenuFunction(menu, mouseEvent->point.x, mouseEvent->point.y);
			break;

		case EVENT_MOUSE_OUT:
			mouseEvent = &event.mouseEvent;
			k_getWindowArea(menu->id, &menuArea);
			k_clearMenuActivity(menu, menuArea.x1 + mouseEvent->point.x, menuArea.y1 + mouseEvent->point.y);
			break;
		}
	}
		
	return true;
}

static void k_processMenuActivity(Menu* menu, int mouseX, int mouseY) {
	int currentIndex;

	currentIndex = k_getMenuItemIndex(menu, mouseX, mouseY);
	if ((currentIndex == -1) || (currentIndex == menu->prevIndex)) {
		return;
	}

	if (menu->prevIndex != -1) {
		k_drawMenuItem(menu, menu->prevIndex, false, menu->textColor, menu->backgroundColor, menu->activeColor);	
		if (menu->table[menu->prevIndex].hasSubMenu == true) {
			k_changeMenuVisibility((Menu*)menu->table[menu->prevIndex].param, false);
		}
	}

	k_drawMenuItem(menu, currentIndex, true, menu->textColor, menu->backgroundColor, menu->activeColor);
	if (menu->table[currentIndex].hasSubMenu == true) {
		k_changeMenuVisibility((Menu*)menu->table[currentIndex].param, true);
	}

	menu->prevIndex = currentIndex;
}

static void k_processMenuFunction(Menu* menu, int mouseX, int mouseY) {
	int currentIndex;

	currentIndex = k_getMenuItemIndex(menu, mouseX, mouseY);
	if (currentIndex == -1) {
		return;
	}

	if (menu->table[currentIndex].hasSubMenu == true) {
		k_toggleMenuVisibility((Menu*)menu->table[currentIndex].param, menu, currentIndex);

	} else {
		k_drawMenuItem(menu, currentIndex, false, menu->textColor, menu->backgroundColor, menu->activeColor);

		if (menu->parentId != WINDOW_INVALIDID) {
			k_showChildWindows(menu->parentId, false, WINDOW_FLAGS_MENU, false);
			if ((menu->top != null) && (menu->top->prevIndex != -1)) {
				k_drawMenuItem(menu->top, menu->top->prevIndex, false, menu->top->textColor, menu->top->backgroundColor, menu->top->activeColor);
				menu->top->prevIndex = -1;
			}
		}
	}

	if (menu->table[currentIndex].func != null) {
		menu->table[currentIndex].func(menu->table[currentIndex].param);
	}
}

static void k_clearMenuActivity(Menu* menu, int mouseX, int mouseY) {
	Menu* sub;
	Rect subArea;

	if (menu->prevIndex != -1) {
		if (menu->table[menu->prevIndex].hasSubMenu == true) {
			sub = (Menu*)menu->table[menu->prevIndex].param;
			if (sub != null) {
				k_getWindowArea(sub->id, &subArea);
				if (k_isPointInRect(&subArea, mouseX, mouseY) == true) {
					return;
				}
			}
		}

		k_drawMenuItem(menu, menu->prevIndex, false, menu->textColor, menu->backgroundColor, menu->activeColor);
		if (menu->table[menu->prevIndex].hasSubMenu == true) {
			k_changeMenuVisibility(sub, false);
		}
	}

	menu->prevIndex = -1;
}

void k_changeMenuVisibility(Menu* sub, bool visible) {
	if (sub == null) {
		return;
	}

	if ((visible == false) && (sub->prevIndex != -1)) {
		if (sub->table[sub->prevIndex].hasSubMenu == true) {
			k_changeMenuVisibility((Menu*)sub->table[sub->prevIndex].param, false);
		}
	}

	if (visible == true) {
		if (sub->prevIndex != -1) {
			k_drawMenuItem(sub, sub->prevIndex, false, sub->textColor, sub->backgroundColor, sub->activeColor);
			sub->prevIndex = -1;
		}

		k_moveWindowToTop(sub->id);
		k_showWindow(sub->id, true);
		sub->visible = true;

	} else {
		k_showWindow(sub->id, false);
		sub->visible = false;
	}
}

void k_toggleMenuVisibility(Menu* sub, Menu* menu, int index) {
	if (sub == null) {
		return;
	}

	if ((sub->visible == true) && (sub->prevIndex != -1)) {
		if (sub->table[sub->prevIndex].hasSubMenu == true) {
			k_toggleMenuVisibility((Menu*)sub->table[sub->prevIndex].param, sub, sub->prevIndex);
		}
	}

	if (sub->visible == false) {
		if ((menu != null) && (index != -1)) {
			k_drawMenuItem(menu, index, true, menu->textColor, menu->backgroundColor, menu->activeColor);
			menu->prevIndex = index;
		}

		if (sub->prevIndex != -1) {
			k_drawMenuItem(sub, sub->prevIndex, false, sub->textColor, sub->backgroundColor, sub->activeColor);
			sub->prevIndex = -1;
		}

		k_moveWindowToTop(sub->id);
		k_showWindow(sub->id, true);
		sub->visible = true;

	} else {
		if ((menu != null) && (index != -1)) {
			k_drawMenuItem(menu, index, false, menu->textColor, menu->backgroundColor, menu->activeColor);
		}

		k_showWindow(sub->id, false);
		sub->visible = false;
	}
}

int k_getMenuItemIndex(const Menu* menu, int mouseX, int mouseY) {
	int index = -1;
	int i;

	if ((menu->flags & MENU_FLAGS_HORIZONTAL) || (menu->flags & MENU_FLAGS_DRAWONPARENT)) {
		for (i = 0; i < menu->itemCount; i++) {
			if (k_isPointInRect(&menu->table[i].area, mouseX, mouseY) == true) {
				index = i;
				break;
			}
		}

	} else {
		index = mouseY / menu->itemHeight;
	}

	if ((index < 0) || (index >= menu->itemCount)) {
		return -1;
	}

	return index;
}

bool k_drawMenuItem(const Menu* menu, int index, bool active, Color textColor, Color backgroundColor, Color activeColor) {
	Window* window;
	Rect area;
	const Rect* itemArea;

	window = k_getWindowWithLock(menu->id);
	if (window == null) {
		return false;
	}

	// set clipping area on window coordinates.
	k_setRect(&area, 0, 0, window->area.x2 - window->area.x1, window->area.y2 - window->area.y1);

	itemArea = &menu->table[index].area;

	if (active == true) {
		__k_drawRect(window->buffer, &area, itemArea->x1, itemArea->y1, itemArea->x2, itemArea->y2, activeColor, true);
		__k_drawText(window->buffer, &area, itemArea->x1 + MENU_ITEMWIDTHPADDING, itemArea->y1 + menu->itemHeightPadding, textColor, activeColor, menu->table[index].name, k_strlen(menu->table[index].name));

	} else {
		__k_drawRect(window->buffer, &area, itemArea->x1, itemArea->y1, itemArea->x2, itemArea->y2, backgroundColor, true);
		__k_drawText(window->buffer, &area, itemArea->x1 + MENU_ITEMWIDTHPADDING, itemArea->y1 + menu->itemHeightPadding, textColor, backgroundColor, menu->table[index].name, k_strlen(menu->table[index].name));
	}

	k_unlock(&window->mutex);

	k_updateScreenByWindowArea(menu->id, itemArea);

	return true;
}

void k_drawAllMenuItems(const Menu* menu, Color textColor, Color backgroundColor, Color activeColor) {
	int i;

	for (i = 0; i < menu->itemCount; i++) {
		k_drawMenuItem(menu, i, false, textColor, backgroundColor, activeColor);
	}
}

void k_processTopMenuActivity(qword parentId, int mouseX, int mouseY) {
	Window* window;

	window = k_getWindow(parentId);
	if ((window == null) || (window->topMenu == null)) {
		return;
	}

	k_processMenuActivity(window->topMenu, mouseX - window->area.x1, mouseY - window->area.y1);
}

void k_clearTopMenuActivity(qword parentId, int mouseX, int mouseY) {
	Window* window;

	window = k_getWindow(parentId);
	if ((window == null) || (window->topMenu == null)) {
		return;
	}

	k_clearMenuActivity(window->topMenu, mouseX, mouseY);
}

int k_getTopMenuItemIndex(qword parentId, int mouseX, int mouseY) {
	Window* window;

	window = k_getWindow(parentId);
	if ((window == null) || (window->topMenu == null)) {
		return -1;
	}

	return k_getMenuItemIndex(window->topMenu, mouseX - window->area.x1, mouseY - window->area.y1);
}

bool k_drawHansLogo(qword windowId, int x, int y, int width, int height, Color brightColor, Color darkColor) {
	Window* window;
	Rect area;
	int w1, w2, w3;
	int h1, h2, h3;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// set clipping area on window coordinates.
	k_setRect(&area, 0, 0, window->area.x2 - window->area.x1, window->area.y2 - window->area.y1);

	w1 = width / 3;
	w2 = (width * 2) / 3;
	w3 = width;

	h1 = height / 3;
	h2 = (height * 2) /3;
	h3 = height;

	/* draw dark shadow of mark */
	__k_drawLine(window->buffer, &area, x + w1 - 1, y, x + w1 - 1, y + h1, darkColor);
	__k_drawLine(window->buffer, &area, x + w1, y, x + w1, y + h1, darkColor);

	__k_drawLine(window->buffer, &area, x + w1 - 1, y + h2, x + w1 - 1, y + h3, darkColor);
	__k_drawLine(window->buffer, &area, x + w1, y + h2, x + w1, y + h3, darkColor);	
	
	__k_drawLine(window->buffer, &area, x, y + h3 - 1, x + w1, y + h3 - 1, darkColor);
	__k_drawLine(window->buffer, &area, x, y + h3, x + w1, y + h3, darkColor);

	__k_drawLine(window->buffer, &area, x + w1, y + h2 - 1, x + w2, y + h2 - 1, darkColor);
	__k_drawLine(window->buffer, &area, x + w1, y + h2, x + w2, y + h2, darkColor);

	__k_drawLine(window->buffer, &area, x + w3 - 1, y, x + w3 - 1, y + h3, darkColor);
	__k_drawLine(window->buffer, &area, x + w3, y, x + w3, y + h3, darkColor);

	__k_drawLine(window->buffer, &area, x + w2, y + h3 - 1, x + w3, y + h3 - 1, darkColor);
	__k_drawLine(window->buffer, &area, x + w2, y + h3, x + w3, y + h3, darkColor);

	/* draw bright shadow of mark */
	__k_drawLine(window->buffer, &area, x, y, x + w1, y, brightColor);
	__k_drawLine(window->buffer, &area, x, y + 1, x + w1, y + 1, brightColor);

	__k_drawLine(window->buffer, &area, x, y, x, y + h3, brightColor);
	__k_drawLine(window->buffer, &area, x + 1, y, x + 1, y + h3, brightColor);

	__k_drawLine(window->buffer, &area, x + w1, y + h1, x + w2, y + h1, brightColor);
	__k_drawLine(window->buffer, &area, x + w1, y + h1 + 1, x + w2, y + h1 + 1, brightColor);

	__k_drawLine(window->buffer, &area, x + w2, y, x + w3, y, brightColor);
	__k_drawLine(window->buffer, &area, x + w2, y + 1, x + w3, y + 1, brightColor);

	__k_drawLine(window->buffer, &area, x + w2, y, x + w2, y + h1, brightColor);
	__k_drawLine(window->buffer, &area, x + w2 + 1, y, x + w2 + 1, y + h1, brightColor);

	__k_drawLine(window->buffer, &area, x + w2, y + h2, x + w2, y + h3, brightColor);
	__k_drawLine(window->buffer, &area, x + w2 + 1, y + h2, x + w2 + 1, y + h3, brightColor);

	k_unlock(&window->mutex);

	return true;
}

bool k_drawButton(qword windowId, const Rect* buttonArea, Color textColor, Color backgroundColor, const char* text, dword flags) {
	Window* window;
	Rect area;
	int textLen;
	int textX;
	int textY;
	Color brighterColor;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// set clipping area on window coordinates.
	k_setRect(&area, 0, 0, window->area.x2 - window->area.x1, window->area.y2 - window->area.y1);

	// draw a button background.
	__k_drawRect(window->buffer, &area, buttonArea->x1, buttonArea->y1, buttonArea->x2, buttonArea->y2, backgroundColor, true);	

	// get text length.
	textLen = k_strlen(text);

	// draw a text in the center.
	textX = (buttonArea->x1 + k_getRectWidth(buttonArea) / 2) - (textLen * FONT_DEFAULT_WIDTH) / 2;
	textY = (buttonArea->y1 + k_getRectHeight(buttonArea) / 2) - FONT_DEFAULT_HEIGHT / 2;
	__k_drawText(window->buffer, &area, textX, textY, textColor, backgroundColor, text, textLen);
	
	if (flags & BUTTON_FLAGS_SHADOW) {
		brighterColor = k_changeColorBrightness2(backgroundColor, 30, 30, 30);

		// draw top lines (2 pixels-thick) of the button with bright color in order to make it 3-dimensional.
		__k_drawLine(window->buffer, &area, buttonArea->x1, buttonArea->y1, buttonArea->x2, buttonArea->y1, brighterColor);
		__k_drawLine(window->buffer, &area, buttonArea->x1, buttonArea->y1 + 1, buttonArea->x2 - 1, buttonArea->y1 + 1, brighterColor);

		// draw left lines (2 pixels-thick) of the button with bright color in order to make it 3-dimensional.
		__k_drawLine(window->buffer, &area, buttonArea->x1, buttonArea->y1, buttonArea->x1, buttonArea->y2, brighterColor);
		__k_drawLine(window->buffer, &area, buttonArea->x1 + 1, buttonArea->y1, buttonArea->x1 + 1, buttonArea->y2 - 1, brighterColor);
		
		// draw bottom lines (2 pixels-thick) of the button with dark color in order to make it 3-dimensional.
		__k_drawLine(window->buffer, &area, buttonArea->x1 + 1, buttonArea->y2, buttonArea->x2, buttonArea->y2, WINDOW_COLOR_BUTTONDARK);
		__k_drawLine(window->buffer, &area, buttonArea->x1 + 2, buttonArea->y2 - 1, buttonArea->x2, buttonArea->y2 - 1, WINDOW_COLOR_BUTTONDARK);

		// draw right lines (2 pixels-thick) of the button with dark color in order to make it 3-dimensional.
		__k_drawLine(window->buffer, &area, buttonArea->x2, buttonArea->y1 + 1, buttonArea->x2, buttonArea->y2, WINDOW_COLOR_BUTTONDARK);
		__k_drawLine(window->buffer, &area, buttonArea->x2 - 1, buttonArea->y1 + 2, buttonArea->x2 - 1, buttonArea->y2, WINDOW_COLOR_BUTTONDARK);
	}

	k_unlock(&window->mutex);

	return true;
}

static ClockManager g_clockManager;

void k_initClockManager(void) {
	k_initMutex(&g_clockManager.mutex);
	k_initList(&g_clockManager.clockList);
	g_clockManager.prevHour = 0;
	g_clockManager.prevMinute = 0;
	g_clockManager.prevSecond = 0;
}

void k_setClock(Clock* clock, qword windowId, int x, int y, Color textColor, Color backgroundColor, byte format, bool reset) {
	Rect clockArea;

	if ((reset == true) && (clock->link.id != KID_INVALID)) {
		clock->link.id = k_allocKid();	
	}	
	
	clock->windowId = windowId;
	clock->textColor = textColor;
	clock->backgroundColor = backgroundColor;
	clock->format = format;

	switch (format) {
	case CLOCK_FORMAT_H:
		k_strcpy(clock->formatStr, "00");
		k_setRect(&clockArea, x, y, x + FONT_DEFAULT_WIDTH * 2, y + FONT_DEFAULT_HEIGHT);
		k_memcpy(&clock->area, &clockArea, sizeof(Rect));
		break;

	case CLOCK_FORMAT_HA:
		k_strcpy(clock->formatStr, "00 AM");
		k_setRect(&clockArea, x, y, x + FONT_DEFAULT_WIDTH * 5, y + FONT_DEFAULT_HEIGHT);
		k_memcpy(&clock->area, &clockArea, sizeof(Rect));
		break;

	case CLOCK_FORMAT_HM:
		k_strcpy(clock->formatStr, "00:00");
		k_setRect(&clockArea, x, y, x + FONT_DEFAULT_WIDTH * 5, y + FONT_DEFAULT_HEIGHT);
		k_memcpy(&clock->area, &clockArea, sizeof(Rect));
		break;

	case CLOCK_FORMAT_HMA:
		k_strcpy(clock->formatStr, "00:00 AM");
		k_setRect(&clockArea, x, y, x + FONT_DEFAULT_WIDTH * 8, y + FONT_DEFAULT_HEIGHT);
		k_memcpy(&clock->area, &clockArea, sizeof(Rect));
		break;

	case CLOCK_FORMAT_HMS:
		k_strcpy(clock->formatStr, "00:00:00");
		k_setRect(&clockArea, x, y, x + FONT_DEFAULT_WIDTH * 8, y + FONT_DEFAULT_HEIGHT);
		k_memcpy(&clock->area, &clockArea, sizeof(Rect));
		break;

	case CLOCK_FORMAT_HMSA:
	default:
		k_strcpy(clock->formatStr, "00:00:00 AM");
		k_setRect(&clockArea, x, y, x + FONT_DEFAULT_WIDTH * 11, y + FONT_DEFAULT_HEIGHT);
		k_memcpy(&clock->area, &clockArea, sizeof(Rect));
		break;
	}	
}

void k_addClock(Clock* clock) {
	k_lock(&g_clockManager.mutex);

	k_addListToTail(&g_clockManager.clockList, clock);

	k_unlock(&g_clockManager.mutex);
}

Clock* k_removeClock(qword clockId) {
	Clock* clock;

	k_lock(&g_clockManager.mutex);

	clock = k_removeListById(&g_clockManager.clockList, clockId);

	k_unlock(&g_clockManager.mutex);

	return clock;
}

void k_drawAllClocks() {
	Clock* clock;

	k_lock(&g_clockManager.mutex);

	clock = k_getHeadFromList(&g_clockManager.clockList);
	while (clock != null) {
		k_drawClock(clock);
		clock = k_getNextFromList(&g_clockManager.clockList, clock);
	}

	k_unlock(&g_clockManager.mutex);
}

static void k_drawClock(Clock* clock) {
	byte hour, minute, second;
	bool pm = false;

	k_readRtcTime(&hour, &minute, &second);

	if ((g_clockManager.prevHour == hour) && (g_clockManager.prevMinute == minute) && (g_clockManager.prevSecond == second)) {
		return;
	}

	g_clockManager.prevHour = hour;
	g_clockManager.prevMinute = minute;
	g_clockManager.prevSecond = second;

	if (hour >= 12) {
		pm = true;
		if (hour > 12) {
			hour -= 12;
		}
	}

	switch (clock->format) {
	case CLOCK_FORMAT_H:
		clock->formatStr[0] = hour / 10 + '0';
		clock->formatStr[1] = hour % 10 + '0';
		break;

	case CLOCK_FORMAT_HA:
		clock->formatStr[0] = hour / 10 + '0';
		clock->formatStr[1] = hour % 10 + '0';
		if (pm == true) {
			clock->formatStr[3] = 'P';
		}
		break;

	case CLOCK_FORMAT_HM:
		clock->formatStr[0] = hour / 10 + '0';
		clock->formatStr[1] = hour % 10 + '0';
		clock->formatStr[3] = minute / 10 + '0';
		clock->formatStr[4] = minute % 10 + '0';
		break;

	case CLOCK_FORMAT_HMA:
		clock->formatStr[0] = hour / 10 + '0';
		clock->formatStr[1] = hour % 10 + '0';
		clock->formatStr[3] = minute / 10 + '0';
		clock->formatStr[4] = minute % 10 + '0';
		if (pm == true) {
			clock->formatStr[6] = 'P';
		}
		break;

	case CLOCK_FORMAT_HMS:
		clock->formatStr[0] = hour / 10 + '0';
		clock->formatStr[1] = hour % 10 + '0';
		clock->formatStr[3] = minute / 10 + '0';
		clock->formatStr[4] = minute % 10 + '0';
		clock->formatStr[6] = second / 10 + '0';
		clock->formatStr[7] = second % 10 + '0';
		break;

	case CLOCK_FORMAT_HMSA:
	default:
		clock->formatStr[0] = hour / 10 + '0';
		clock->formatStr[1] = hour % 10 + '0';
		clock->formatStr[3] = minute / 10 + '0';
		clock->formatStr[4] = minute % 10 + '0';
		clock->formatStr[6] = second / 10 + '0';
		clock->formatStr[7] = second % 10 + '0';
		if (pm == true) {
			clock->formatStr[9] = 'P';
		}
		break;
	}

	k_drawText(clock->windowId, clock->area.x1, clock->area.y1, clock->textColor, clock->backgroundColor, clock->formatStr, k_strlen(clock->formatStr));
	k_updateScreenByWindowArea(clock->windowId, &clock->area);
}
