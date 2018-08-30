#include "window_manager_task.h"
#include "window.h"
#include "vbe.h"
#include "mouse.h"
#include "task.h"
#include "multiprocessor.h"
#include "util.h"

void k_startWindowManager(void) {
	VbeModeInfoBlock* vbeMode;
	int mouseX, mouseY;
	byte buttonStatus;
	int relativeX, relativeY;
	qword windowId;
	Task* task;
	char title[WINDOW_MAXTITLELENGTH + 1];
	int windowCount = 0;

	// get window manager task.
	task = k_getRunningTask(k_getApicId());

	// initialize GUI.
	k_initGui();

	// move mouse cursor to current mouse position.
	k_getMouseCursorPos(&mouseX, &mouseY);
	k_moveMouseCursor(mouseX, mouseY);

	/**
	  < Window Manager Task >
	  - process to move mouse cursor.
	  - click left mouse button to create a window.
	  - click right mouse button to delete all windows.
	*/

	while (true) {
		// wait for mouse data.
		if (k_getMouseDataFromMouseQueue(&buttonStatus, &relativeX, &relativeY) == false) {
			k_sleep(0);
			continue;
		}

		// calculate current mouse position.
		k_getMouseCursorPos(&mouseX, &mouseY);
		mouseX += relativeX;
		mouseY += relativeY;

		// If left mouse button is clicked, create a window.
		if (buttonStatus & MOUSE_LBUTTONDOWN) {
			// creat a winow at current mouse position.
			// After drawing texts in the window, show the window.
			k_sprintf(title, "Window %d", ++windowCount);
			windowId = k_createWindow(mouseX - 10, mouseY - WINDOW_TITLEBAR_HEIGHT / 2, 400, 200, WINDOW_FLAGS_DRAWFRAME | WINDOW_FLAGS_DRAWTITLEBAR, title);
			k_drawText(windowId, 10, WINDOW_TITLEBAR_HEIGHT + 10, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "This is a real window.");
			k_drawText(windowId, 10, WINDOW_TITLEBAR_HEIGHT + 30, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "No more prototypes.");
			k_showWindow(windowId, true);

		// If right mouse button is clicked, delete all windows.
		} else if (buttonStatus & MOUSE_RBUTTONDOWN) {
			// delete all windows created by window manager task.
			k_deleteAllWindowsByTask(task->link.id);
			windowCount = 0;
		}
		
		// move mouse cursor to current mouse position.
		k_moveMouseCursor(mouseX, mouseY);
	}
}