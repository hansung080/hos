#ifndef __GUITASKS_SYSTEMMENU_H__
#define __GUITASKS_SYSTEMMENU_H__

#include "../core/types.h"
#include "../fonts/fonts.h"
#include "../core/window.h"

/* System Menu Macros */
#define SYSMENU_TITLE  "SYS_MENU"
#define SYSMENU_HEIGHT WINDOW_SYSMENU_HEIGHT

// system menu color
#define SYSMENU_COLOR_BACKGROUND RGB(33, 147, 176)

#define SYSMENU_INDEX_HANSOS 0
#define SYSMENU_INDEX_APPS   1
#define SYSMENU_INDEX_SHELL  2

void k_systemMenuTask(void);
static bool k_processSystemMenuEvent(qword windowId, Clock* clock, Menu* clockMenu);

/* System Menu Functions */
static void k_funcApps(qword parentId);
static void k_funcShell(qword parentId);

/* HansOS Menu Functions */
static void k_funcAboutHansos(qword parentId);
static void k_funcShutdown(qword parentId);
static void k_funcReboot(qword parentId);

/* Clock Menu Function */
static void k_funcClockHh(qword clock_);
static void k_funcClockHham(qword clock_);
static void k_funcClockHhmm(qword clock_);
static void k_funcClockHhmmam(qword clock_);
static void k_funcClockHhmmss(qword clock_);
static void k_funcClockHhmmssam(qword clock_);

extern Menu* g_systemMenu;

#endif // __GUITASKS_SYSTEMMENU_H__