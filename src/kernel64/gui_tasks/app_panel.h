#ifndef __GUITASKS_APPPANEL_H__
#define __GUITASKS_APPPANEL_H__

#include "../core/types.h"
#include "../core/window.h"

// app panel item size
#define APPPANEL_ITEMSIZE   80
#define APPPANEL_ITEMMARGIN 22

// app panel item color
#define APPPANEL_COLOR_ITEM_PROTOTYPE     RGB(255, 185, 151) // #ffb997
#define APPPANEL_COLOR_ITEM_EVENTMONITOR  RGB(246, 126, 125) // #f67e7d
#define APPPANEL_COLOR_ITEM_SYSTEMMONITOR RGB(132, 59, 98)   // #843b62
#define APPPANEL_COLOR_ITEM_SHELL         RGB(116, 84, 106)  // #74546a
#define APPPANEL_COLOR_ITEM_IMAGEVIEWER   RGB(29, 120, 116)  // #1d7874
#define APPPANEL_COLOR_ITEM_COLORPICKER   RGB(103, 146, 137) // #679289
#define APPPANEL_COLOR_ITEM_USER0         RGB(244, 192, 149) // #f4c095
#define APPPANEL_COLOR_ITEM_USER1         RGB(140, 47, 57)   // #8c2f39
#define APPPANEL_COLOR_ITEM_USER2         RGB(178, 58, 72)   // #b23a48
#define APPPANEL_COLOR_ITEM_USER3         RGB(104, 176, 171) // #68b0ab
#define APPPANEL_COLOR_ITEM_USER4         RGB(74, 124, 89)   // #4a7c59
#define APPPANEL_COLOR_ITEM_USER5         RGB(250, 162, 117) // #faa275
#define APPPANEL_COLOR_ITEM_USER6         RGB(255, 140, 97)  // #ff8c61
#define APPPANEL_COLOR_ITEM_USER7         RGB(206, 106, 133) // #ce6a85
#define APPPANEL_COLOR_ITEM_USER8         RGB(152, 83, 119)  // #985377
#define APPPANEL_COLOR_ITEM_USER9         RGB(92, 55, 76)    // #5c374c
#define APPPANEL_COLOR_ITEM_USER10        RGB(36, 123, 160)  // #247ba0
#define APPPANEL_COLOR_ITEM_USER11        RGB(254, 147, 140) // #fe938c
#define APPPANEL_COLOR_ITEM_USER12        RGB(156, 175, 183) // #9cafb7
#define APPPANEL_COLOR_ITEM_USER13        RGB(66, 129, 164)  // #4281a4
#define APPPANEL_COLOR_ITEM_USER14        RGB(80, 81, 79)    // #50514f
#define APPPANEL_COLOR_ITEM_USER15        RGB(242, 95, 92)   // #f25f5c
#define APPPANEL_COLOR_ITEM_USER16        RGB(255, 224, 102) // #ffe066
#define APPPANEL_COLOR_ITEM_USER17        RGB(112, 193, 179) // #70c1b3
#define APPPANEL_COLOR_ITEM_USER18        RGB(212, 77, 92)   // #d44d5c
#define APPPANEL_COLOR_ITEM_USER19        RGB(135, 187, 162) // #87bba2
#define APPPANEL_COLOR_ITEM_USER20        RGB(85, 130, 139)  // #55828b
#define APPPANEL_COLOR_ITEM_USER21        RGB(59, 96, 100)   // #3b6064
#define APPPANEL_COLOR_ITEM_USER22        RGB(255, 225, 168) // #ffe1a8
#define APPPANEL_COLOR_ITEM_USER23        RGB(226, 109, 92)  // #e26d5c

/* Panel Macros */
// panel title
#define PANEL_TITLE       "WIDGET_PANEL"

// panel size
#define PANEL_LOGOWIDTH   WINDOW_SYSBACKGROUND_LOGOWIDTH
#define PANEL_LOGOHEIGHT  WINDOW_SYSBACKGROUND_LOGOHEIGHT

// panel color
#define PANEL_COLOR_TEXT       RGB(255, 255, 255)
#define PANEL_COLOR_BACKGROUND RGB(212, 252, 121)
#define PANEL_COLOR_ACTIVE     RGB(222, 98, 98)
#define PANEL_COLOR_LOGOBRIGHT RGB(255, 255, 255)
#define PANEL_COLOR_LOGODARK   RGB(150, 230, 161)

// panel flags
#define PANEL_FLAGS_DRAWLOGO 0x00000001 // 0: not draw logo, 1: draw logo
#define PANEL_FLAGS_BLOCKING 0x00000002 // 0: create non-blocking window, 1: create blocking window
#define PANEL_FLAGS_VISIBLE  0x00000004 // 0: create invisible window, 1: create visible window
#define PANEL_FLAGS_APPPANEL 0x00000008 // 0: not app panel, 1: app panel (temporary flag)

typedef void (*PanelFunc)(qword param);

#pragma pack(push, 1)

/* Panel Structs */
typedef struct k_PanelItem {
	const char* const name1;
	const char* const name2;
	const PanelFunc func;
	const Color color;
	qword param; // Default param is panel ID.
	Circle area; // window coordinates
} PanelItem;

typedef struct k_Panel {
	PanelItem* const table;
	const int itemCount;
	qword id;
	int itemSize; // item size with margin
	int columns;
	int rows;
	bool visible;
	int prevIndex;
	Color textColor;
	Color backgroundColor;
	Color activeColor;
	dword flags;
} Panel;

#pragma pack(pop)

void k_appPanelTask(void);

/* Panel Functions */
bool k_createPanel(Panel* panel, int x, int y, int width, int height, int itemSize, int itemMargin, Color* colors, dword flags);
bool k_processPanelEvent(Panel* panel);
static void k_processPanelActivity(Panel* panel, int index);
static void k_processPanelFunction(Panel* panel, int index);
static void k_clearPanelActivity(Panel* panel);
void k_changePanelVisibility(Panel* panel, Menu* menu, int index, bool visible);
void k_togglePanelVisibility(Panel* panel, Menu* menu, int index);
int k_getPanelItemIndex(const Panel* panel, int mouseX, int mouseY); // window coordinates
bool k_drawPanelItem(const Panel* panel, int index, bool active);

/* System App Panel Function */
static void k_funcSystemApp(qword entryPoint);

extern Panel* g_appPanel;

#endif // __GUITASKS_APPPANEL_H__