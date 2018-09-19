#ifndef __APP_PANEL_H__
#define __APP_PANEL_H__

#include "../core/types.h"
#include "../core/window.h"
#include "../fonts/fonts.h"

// app panel-related macros
#define APPPANEL_TITLE      "SYS_APPPANEL"
#define APPPANEL_HEIGHT     31
#define APPPANEL_CLOCKWIDTH (FONT_VERAMONO_ENG_WIDTH * 8) // clock ex) 09:00 AM

// app list-related macros
#define APPLIST_TITLE      "SYS_APPLIST"
#define APPLIST_ITEMHEIGHT (FONT_VERAMONO_ENG_HEIGHT + 4)

// app panel color
#define APPPANEL_COLOR_OUTERLINE  RGB(109, 218, 22)
#define APPPANEL_COLOR_MIDDLELINE RGB(183, 249, 171)
#define APPPANEL_COLOR_INNERLINE  RGB(150, 210, 140)
#define APPPANEL_COLOR_BACKGROUND RGB(55, 135, 11) // dark green
#define APPPANEL_COLOR_ACTIVE     RGB(79, 204, 11) // bright green

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

void k_appPanelTask(void);
static bool k_createAppPanel(void);
static void k_drawDigitalClock(qword windowId);
static bool k_processAppPanelEvent(void);
static bool k_createAppList(void);
static void k_drawAppItem(int index, bool mouseOver);
static bool k_processAppListEvent(void);
static int k_getMouseOverItemIndex(int mouseY);

#endif // __APP_PANEL_H__