#include "menu.h"
#include "../core/window.h"
#include "../utils/util.h"

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

		windowId = k_createWindow(x, y, windowWidth, windowHeight, WINDOW_FLAGS_CHILD | WINDOW_FLAGS_MENU | optionalFlags, MENU_TITLE, menu->backgroundColor, null, menu, parentId);
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
	int index;

	index = k_getMenuItemIndex(menu, mouseX, mouseY);
	if ((index == -1) || (index == menu->prevIndex)) {
		return;
	}

	if (menu->prevIndex != -1) {
		k_drawMenuItem(menu, menu->prevIndex, false, menu->textColor, menu->backgroundColor, menu->activeColor);	
		if (menu->table[menu->prevIndex].hasSubMenu == true) {
			k_changeMenuVisibility((Menu*)menu->table[menu->prevIndex].param, false);
		}
	}

	k_drawMenuItem(menu, index, true, menu->textColor, menu->backgroundColor, menu->activeColor);
	if (menu->table[index].hasSubMenu == true) {
		k_changeMenuVisibility((Menu*)menu->table[index].param, true);
	}

	menu->prevIndex = index;
}

static void k_processMenuFunction(Menu* menu, int mouseX, int mouseY) {
	int index;

	index = k_getMenuItemIndex(menu, mouseX, mouseY);
	if (index == -1) {
		return;
	}

	if (menu->table[index].hasSubMenu == true) {
		k_toggleMenuVisibility((Menu*)menu->table[index].param, menu, index);

	} else {
		k_drawMenuItem(menu, index, false, menu->textColor, menu->backgroundColor, menu->activeColor);

		if (menu->parentId != WINDOW_INVALIDID) {
			k_showChildWindows(menu->parentId, false, WINDOW_FLAGS_MENU, false);
			if ((menu->top != null) && (menu->top->prevIndex != -1)) {
				k_drawMenuItem(menu->top, menu->top->prevIndex, false, menu->top->textColor, menu->top->backgroundColor, menu->top->activeColor);
				menu->top->prevIndex = -1;
			}
		}
	}

	if (menu->table[index].func != null) {
		menu->table[index].func(menu->table[index].param);
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

		menu->prevIndex = -1;	
	}
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
