#ifndef __GUITASKS_SYSTEMMENU_H__
#define __GUITASKS_SYSTEMMENU_H__

#include "../core/types.h"
#include "../core/window.h"
#include "../fonts/fonts.h"

#if 0
/* System Menu Macros */
#define SYSMENU_TITLE      "SYS_MENU"
#define SYSMENU_HEIGHT     WINDOW_SYSMENU_HEIGHT
#define SYSMENU_CLOCKWIDTH (FONT_DEFAULT_WIDTH * 8) // clock ex) 09:00 AM

// system menu color
#define SYSMENU_COLOR_INNERLINE  RGB(33, 147, 176)
#define SYSMENU_COLOR_BACKGROUND RGB(33, 147, 176)
#define SYSMENU_COLOR_ACTIVE     RGB(222, 98, 98)

void k_systemMenuTask(void);
static bool k_createSystemMenu(void);
static void k_drawDigitalClock(qword windowId);
static bool k_processSystemMenuEvent(void);
#endif

// app panel-related macros
#define APPPANEL_TITLE      "SYS_APPPANEL"
#define APPPANEL_HEIGHT     WINDOW_SYSMENU_HEIGHT
#define APPPANEL_CLOCKWIDTH (FONT_DEFAULT_WIDTH * 8) // clock ex) 09:00 AM

// app list-related macros
#define APPLIST_TITLE      "SYS_APPLIST"
#define APPLIST_ITEMHEIGHT (FONT_DEFAULT_HEIGHT + 4)

// app panel color
#define APPPANEL_COLOR_OUTERLINE  RGB(33, 147, 176)
#define APPPANEL_COLOR_MIDDLELINE RGB(33, 147, 176)
#define APPPANEL_COLOR_INNERLINE  RGB(33, 147, 176)
#define APPPANEL_COLOR_BACKGROUND RGB(33, 147, 176)
#define APPPANEL_COLOR_ACTIVE     RGB(222, 98, 98)

#pragma pack(push, 1)

typedef struct k_AppEntry {
	char* name;       // app name
	void* entryPoint; // task entry point
} AppEntry;

typedef struct k_AppPanelManager {
	qword appPanelId;       // app panel ID (window ID)
	qword appListId;        // app list ID (window ID)
	Rect buttonArea;        // app list button (toggle button) area 
	int appListWidth;       // app list width
	int prevMouseOverIndex; // previous mouse over index in app list
	bool appListVisible;    // app list visible flag
} AppPanelManager;

#pragma pack(pop)

void k_systemMenuTask(void);
static bool k_createAppPanel(void);
static void k_drawDigitalClock(qword windowId);
static bool k_processAppPanelEvent(void);
static bool k_createAppList(void);
static void k_drawAppItem(int index, bool mouseOver);
static bool k_processAppListEvent(void);
static int k_getMouseOverItemIndex(int mouseY);

#endif // __GUITASKS_SYSTEMMENU_H__