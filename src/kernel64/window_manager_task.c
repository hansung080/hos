#include "window_manager_task.h"
#include "window.h"
#include "util.h"
#include "mouse.h"
#include "keyboard.h"

void k_startWindowManager(void) {
	int mouseX, mouseY;
	bool mouseResult;
	bool keyResult;
	bool windowManagerResult;

	// initialize GUI.
	k_initGui();

	// draw mouse cursor at current mouse position (center in screen).
	k_getMouseCursorPos(&mouseX, &mouseY);
	k_moveMouseCursor(mouseX, mouseY);

	// window manager task loop
	while (true) {
		// process mouse data.
		mouseResult = k_processMouseData();

		// process key.
		keyResult = k_processKey();

		// process window manager event.
		windowManagerResult = false;
		while (k_processWindowManagerEvent() == true) {
			windowManagerResult = true;
		}

		// If no data/events have been processed, switch task.
		if ((mouseResult == false) && (keyResult == false) && (windowManagerResult == false)) {
			k_sleep(0);
		}
	}
}

bool k_processMouseData(void) {
	qword underMouseWindowId;
	byte buttonStatus;
	int relativeX, relativeY;
	int mouseX, mouseY;
	int prevMouseX, prevMouseY;
	byte changedButtonStatus;
	Rect windowArea;
	WindowManager* windowManager;
	char windowTitle[WINDOW_MAXTITLELENGTH + 1]; // [Temp] temporary code
	static int windowCount = 0; // [Temp] temporary code
	qword windowId; // [Temp] temporary code

	if (k_getMouseDataFromMouseQueue(&buttonStatus, &relativeX, &relativeY) == false) {
		return false;
	}

	windowManager = k_getWindowManager();

	k_getMouseCursorPos(&mouseX, &mouseY);

	prevMouseX = mouseX;
	prevMouseY = mouseY;

	mouseX += relativeX;
	mouseY += relativeY;

	// move mouse cursor and call k_getMouseCursorPos to get mouse position always inside screen,
	// because k_moveMouseCursor adjusts mouse position when it's outside screen. 
	k_moveMouseCursor(mouseX, mouseY);
	k_getMouseCursorPos(&mouseX, &mouseY);

	underMouseWindowId = k_findWindowByPoint(mouseX, mouseY);

	changedButtonStatus = windowManager->prevButtonStatus ^ buttonStatus;

	/* left button changed */
	if (changedButtonStatus & MOUSE_LBUTTONDOWN) {
		/* left button down */
		if (buttonStatus & MOUSE_LBUTTONDOWN) {
			if (underMouseWindowId != windowManager->backgroundWindowId) {
				k_moveWindowToTop(underMouseWindowId);
			}

			if (k_isPointInTitleBar(underMouseWindowId, mouseX, mouseY) == true) {
				if (k_isPointInCloseButton(underMouseWindowId, mouseX, mouseY) == true) {
					k_sendWindowEventToWindow(underMouseWindowId, EVENT_WINDOW_CLOSE);

					// [Temp] This code is for test.
					k_deleteWindow(underMouseWindowId);

				} else {
					windowManager->windowMoving = true;
					windowManager->movingWindowId = underMouseWindowId;
				}

			} else {
				k_sendMouseEventToWindow(underMouseWindowId, EVENT_MOUSE_LBUTTONDOWN, mouseX, mouseY, buttonStatus);
			}

		/* left button up */
		} else {
			if (windowManager->windowMoving == true) {
				windowManager->windowMoving = false;
				windowManager->movingWindowId = WINDOW_INVALIDID;

			} else {
				k_sendMouseEventToWindow(underMouseWindowId, EVENT_MOUSE_LBUTTONUP, mouseX, mouseY, buttonStatus);
			}
		}

	/* right button changed */
	} else if (changedButtonStatus & MOUSE_RBUTTONDOWN) {
		/* right button down */
		if (buttonStatus & MOUSE_RBUTTONDOWN) {
			k_sendMouseEventToWindow(underMouseWindowId, EVENT_MOUSE_RBUTTONDOWN, mouseX, mouseY, buttonStatus);

			/* [Temp] These codes below are for test */
			k_sprintf(windowTitle, "Window %d", ++windowCount);
			windowId = k_createWindow(mouseX - 10, mouseY - WINDOW_TITLEBAR_HEIGHT / 2, 400, 200, WINDOW_FLAGS_DRAWFRAME | WINDOW_FLAGS_DRAWTITLEBAR, windowTitle);
			k_drawText(windowId, 10, WINDOW_TITLEBAR_HEIGHT + 10, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "This is a real window.");
			k_drawText(windowId, 10, WINDOW_TITLEBAR_HEIGHT + 30, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "Now, you can select and move a window.");
			k_showWindow(windowId, true);

		/* right button up */
		} else {
			k_sendMouseEventToWindow(underMouseWindowId, EVENT_MOUSE_RBUTTONUP, mouseX, mouseY, buttonStatus);
		}

	/* middle button changed */
	} else if (changedButtonStatus & MOUSE_MBUTTONDOWN) {
		/* middle button down */
		if (buttonStatus & MOUSE_MBUTTONDOWN) {
			k_sendMouseEventToWindow(underMouseWindowId, EVENT_MOUSE_MBUTTONDOWN, mouseX, mouseY, buttonStatus);

		/* middle button up */
		} else {
			k_sendMouseEventToWindow(underMouseWindowId, EVENT_MOUSE_MBUTTONUP, mouseX, mouseY, buttonStatus);
		}

	/* no buttons changed */
	} else {
		k_sendMouseEventToWindow(underMouseWindowId, EVENT_MOUSE_MOVE, mouseX, mouseY, buttonStatus);
	}

	/* process window move */
	if (windowManager->windowMoving == true) {
		if (k_getWindowArea(windowManager->movingWindowId, &windowArea) == true) {
			k_moveWindow(windowManager->movingWindowId, windowArea.x1 + mouseX - prevMouseX, windowArea.y1 + mouseY - prevMouseY);

		} else {
			windowManager->windowMoving = false;
			windowManager->movingWindowId = WINDOW_INVALIDID;
		}
	}

	windowManager->prevButtonStatus = buttonStatus;

	return true;
}

bool k_processKey(void) {
	Key key;

	if (k_getKeyFromKeyQueue(&key) == false) {
		return false;
	}

	k_sendKeyEventToWindow(k_getTopWindowId(), &key);

	return true;
}

bool k_processWindowManagerEvent(void) {
	Event event;
	ScreenUpdateEvent* screenUpdateEvent;
	qword windowId;
	Rect area;

	if (k_recvEventFromWindowManager(&event) == false) {
		return false;
	}

	screenUpdateEvent = &event.screenUpdateEvent;
	
	switch (event.type) {
	case EVENT_SCREENUPDATE_BYID:
		if (k_getWindowArea(screenUpdateEvent->windowId, &area) == true) {
			k_redrawWindowByArea(&area);
		}

		break;

	case EVENT_SCREENUPDATE_BYWINDOWAREA:
		if (k_convertRectWindowToScreen(screenUpdateEvent->windowId, &screenUpdateEvent->area, &area) == true) {
			k_redrawWindowByArea(&area);
		}

		break;

	case EVENT_SCREENUPDATE_BYSCREENAREA:
		k_redrawWindowByArea(&screenUpdateEvent->area);
		break;

	default:
		break;
	}

	return true;
}