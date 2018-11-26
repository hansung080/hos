#ifndef __CORE_WIDGETS_H__
#define __CORE_WIDGETS_H__

#include "types.h"
#include "2d_graphics.h"
#include "../fonts/fonts.h"

/*** Menu Macros ***/
#define MENU_TITLE            "WIDGET_MENU"
#define MENU_ITEMWIDTHPADDING 12

// menu item height
#define MENU_ITEMHEIGHT_NORMAL   (FONT_DEFAULT_HEIGHT + 4)
#define MENU_ITEMHEIGHT_TITLEBAR 21 // WINDOW_TITLEBAR_HEIGHT
#define MENU_ITEMHEIGHT_SYSMENU  31 // WINDOW_SYSMENU_HEIGHT

// menu default color
#define MENU_COLOR_TEXT       RGB(255, 255, 255)
#define MENU_COLOR_BACKGROUND RGB(33, 147, 176)
#define MENU_COLOR_ACTIVE     RGB(222, 98, 98)

// menu flags
#define MENU_FLAGS_HORIZONTAL   0x00000001 // 0: vertical, 1: horizontal
#define MENU_FLAGS_VISIBLE      0x00000002 // 0: start with invisible, 1: start with visible
#define MENU_FLAGS_DRAWONPARENT 0x00000003 // 0: create new window, 1: draw menu on parent window
#define MENU_FLAGS_BLOCKING     0x00000004 // 0: create non-blocking window, 1: create blocking window

typedef void (*MenuFunc)(qword param);

#pragma pack(push, 1)

/*** Menu Structs ***/
typedef struct k_MenuItem {
	const char* const name;
	const MenuFunc func;
	const bool hasSubMenu;
	qword param; // - Default param is parentId.
	             // - If hasSubMenu == true, param must be the sub menu address.
                 // - If hasSubMenu == false, param can be anything.
	Rect area; // window coordinates
} MenuItem;

typedef struct k_Menu {
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
	dword flags;
} Menu;

#pragma pack(pop)

/*** Menu Functions ***/
bool k_createMenu(Menu* menu, int x, int y, int itemHeight, Color* colors, qword parentId, dword flags);
bool k_processMenuEvent(Menu* menu);
static void k_processMenuActivity(Menu* menu, int mouseX, int mouseY); // window coordinates
static void k_processMenuFunction(Menu* menu, int mouseX, int mouseY); // window coordinates
static void k_clearMenuActivity(Menu* menu, int mouseX, int mouseY); // screen coordinates
void k_changeMenuVisibility(Menu* sub, bool visible);
void k_toggleMenuVisibility(Menu* sub, Menu* menu, int index);
int k_getMenuItemIndex(const Menu* menu, int mouseX, int mouseY); // window coordinates
void k_drawMenuItem(const Menu* menu, int index, bool active, Color textColor, Color backgroundColor, Color activeColor);
void k_drawAllMenuItems(const Menu* menu, Color textColor, Color backgroundColor, Color activeColor);
void k_processTopMenuActivity(qword parentId, int mouseX, int mouseY); // screen coordinates
void k_clearTopMenuActivity(qword parentId, int mouseX, int mouseY); // screen coordinates
int k_getTopMenuItemIndex(qword parentId, int mouseX, int mouseY); // screen coordinates

#endif // __CORE_WIDGETS_H__