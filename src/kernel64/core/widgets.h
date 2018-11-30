#ifndef __CORE_WIDGETS_H__
#define __CORE_WIDGETS_H__

#include "types.h"
#include "2d_graphics.h"
#include "../fonts/fonts.h"
#include "sync.h"
#include "../utils/list.h"

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

/*** Button Macros ***/
// button flags
#define BUTTON_FLAGS_SHADOW 0x00000001 // 0: not draw shadow, 1: draw shadow

/*** Clock Macros ***/
// clock size
#define CLOCK_MAXWIDTH (FONT_DEFAULT_WIDTH * 11)
#define CLOCK_HEIGHT   FONT_DEFAULT_HEIGHT

// clock format
#define CLOCK_FORMAT_H    1 // hh
#define CLOCK_FORMAT_HA   2 // hh AM
#define CLOCK_FORMAT_HM   3 // hh:mm
#define CLOCK_FORMAT_HMA  4 // hh:mm AM
#define CLOCK_FORMAT_HMS  5 // hh:mm:ss
#define CLOCK_FORMAT_HMSA 6 // hh:mm:ss AM

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
	Rect area;   // window coordinates
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
	struct k_Menu* top;
	dword flags;
} Menu;

/*** Clock Structs ***/
typedef struct k_Clock {
	ListLink link;
	qword windowId;
	Rect area; // window coordinates 
	Color textColor;
	Color backgroundColor;
	byte format;
	char formatStr[12];
} Clock;

typedef struct k_ClockManager {
	Mutex mutex;
	List clockList;
	byte prevHour;
	byte prevMinute;
	byte prevSecond;
} ClockManager;

#pragma pack(pop)

/*** Menu Functions ***/
bool k_createMenu(Menu* menu, int x, int y, int itemHeight, Color* colors, qword parentId, Menu* top, dword flags);
bool k_processMenuEvent(Menu* menu);
static void k_processMenuActivity(Menu* menu, int mouseX, int mouseY); // window coordinates
static void k_processMenuFunction(Menu* menu, int mouseX, int mouseY); // window coordinates
static void k_clearMenuActivity(Menu* menu, int mouseX, int mouseY); // screen coordinates
void k_changeMenuVisibility(Menu* sub, bool visible);
void k_toggleMenuVisibility(Menu* sub, Menu* menu, int index);
int k_getMenuItemIndex(const Menu* menu, int mouseX, int mouseY); // window coordinates
bool k_drawMenuItem(const Menu* menu, int index, bool active, Color textColor, Color backgroundColor, Color activeColor);
void k_drawAllMenuItems(const Menu* menu, Color textColor, Color backgroundColor, Color activeColor);
void k_processTopMenuActivity(qword parentId, int mouseX, int mouseY); // screen coordinates
void k_clearTopMenuActivity(qword parentId, int mouseX, int mouseY); // screen coordinates
int k_getTopMenuItemIndex(qword parentId, int mouseX, int mouseY); // screen coordinates

/*** HansLogo Function ***/
bool k_drawHansLogo(qword windowId, int x, int y, int width, int height, Color brightColor, Color darkColor);

/*** Button Function ***/
bool k_drawButton(qword windowId, const Rect* buttonArea, Color textColor, Color backgroundColor, const char* text, dword flags);

/*** Clock Functions ***/
void k_initClockManager(void);
void k_setClock(Clock* clock, qword windowId, int x, int y, Color textColor, Color backgroundColor, byte format, bool reset);
void k_addClock(Clock* clock);
Clock* k_removeClock(qword clockId);
void k_drawAllClocks();
static void k_drawClock(Clock* clock);

#endif // __CORE_WIDGETS_H__