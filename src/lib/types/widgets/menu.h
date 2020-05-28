#ifndef __WIDGETS_MENU_H__
#define __WIDGETS_MENU_H__

#include "../types.h"
#include "../fonts.h"
#include "../2d_graphics.h"

/*** Menu Macros ***/
#define MENU_TITLE            "WIDGET_MENU"
#define MENU_ITEMWIDTHPADDING 12

// menu item height
#define MENU_ITEMHEIGHT_NORMAL   (FONT_DEFAULT_HEIGHT + 4)
#define MENU_ITEMHEIGHT_TITLEBAR 21 // WINDOW_TITLEBAR_HEIGHT
#define MENU_ITEMHEIGHT_SYSMENU  24 // WINDOW_SYSMENU_HEIGHT

// menu default color
#define MENU_COLOR_TEXT       RGB(255, 255, 255)
#define MENU_COLOR_BACKGROUND RGB(33, 147, 176)
#define MENU_COLOR_ACTIVE     RGB(222, 98, 98)

// menu flags
#define MENU_FLAGS_HORIZONTAL   0x00000001 // 0: vertical, 1: horizontal
#define MENU_FLAGS_DRAWONPARENT 0x00000002 // 0: create new window, 1: draw menu on parent window
#define MENU_FLAGS_BLOCKING     0x00000004 // 0: create non-blocking window, 1: create blocking window
#define MENU_FLAGS_VISIBLE      0x00000008 // 0: create invisible window, 1: create visible window

typedef void (*MenuFunc)(qword param);

#pragma pack(push, 1)

typedef struct __MenuItem {
	const char* const name;
	const MenuFunc func;
	const bool hasSubMenu;
	qword param; // - Default param is parentId.
	             // - If hasSubMenu == true, param must be the sub menu address.
                 // - If hasSubMenu == false, param can be anything.
	Rect area;   // window coordinates
} MenuItem;

typedef struct __Menu {
	MenuItem* const table;
	const int itemCount;
	qword id;
	bool visible;
	int prevIndex;
	int itemHeight;
	int itemHeightPadding;
	Color textColor;
	Color backgroundColor;
	Color activeColor;
	qword parentId;
	struct k_Menu* top;
	dword flags;
} Menu;

#pragma pack(pop)

#endif // __WIDGETS_MENU_H__