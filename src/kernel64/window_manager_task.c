#include "window_manager_task.h"
#include "window.h"
#include "util.h"
#include "mouse.h"
#include "keyboard.h"
#include "task.h" // [Temp] temporary code
#include "gui_tasks.h" // [Temp] temporary code
#include "fonts.h" // Screen Update Performance Test

void k_startWindowManager(void) {
	int mouseX, mouseY;
	bool mouseResult;
	bool keyResult;
	bool windowManagerResult;
	//====================================================================================================
	/* Screen Update Performance Test */
	qword lastTickCount;
	qword loopCount;
	qword prevLoopCount;
	qword minLoopCount;
	qword backgroundWindowId;
	char loopCountBuffer[40];
	Rect loopCountArea;
	//====================================================================================================
	
	// initialize GUI.
	k_initGui();
	
	// draw mouse cursor at current mouse position (center in screen).
	k_getMouseCursorPos(&mouseX, &mouseY);
	k_moveMouseCursor(mouseX, mouseY);
	
	//====================================================================================================
	/* Screen Update Performance Test */
	lastTickCount = k_getTickCount();
	loopCount = 0;
	prevLoopCount = 0;
	minLoopCount = 0xFFFFFFFFFFFFFFFF;
	backgroundWindowId = k_getBackgroundWindowId();
	//====================================================================================================
	
	/* window manager task loop */
	while (true) {
		//====================================================================================================
		/* Screen Update Performance Test */
		// print minimum loop count among loop counts during 1 second.
		if (k_getTickCount() - lastTickCount > 1000) {
			lastTickCount = k_getTickCount();
			
			if (loopCount - prevLoopCount < minLoopCount) {
				minLoopCount = loopCount - prevLoopCount;
			}
			
			prevLoopCount = loopCount;
			
			k_sprintf(loopCountBuffer, "MIN Loop Count: %d", minLoopCount);
			k_drawText(backgroundWindowId, 0, 0, RGB(0, 0, 0), WINDOW_COLOR_SYSTEMBACKGROUND, loopCountBuffer);
			k_setRect(&loopCountArea, 0, 0, FONT_VERAMONO_ENG_WIDTH * k_strlen(loopCountBuffer) - 1, FONT_VERAMONO_ENG_HEIGHT - 1);
			k_redrawWindowByArea(backgroundWindowId, &loopCountArea);
		}
		
		loopCount++;
		//====================================================================================================
		
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
	int i;

	windowManager = k_getWindowManager();

	/* integrate mouse data: integrate only mouse movement data */
	for (i = 0; i < WINDOWMANAGER_DATAINTEGRATIONCOUNT; i++) {
		if (k_getMouseDataFromMouseQueue(&buttonStatus, &relativeX, &relativeY) == false) {
			if (i == 0) {
				return false;

			} else {
				break;	
			}
		}

		k_getMouseCursorPos(&mouseX, &mouseY);

		if (i == 0) {
			prevMouseX = mouseX;
			prevMouseY = mouseY;
		}		

		mouseX += relativeX;
		mouseY += relativeY;

		// move mouse cursor and call k_getMouseCursorPos to get mouse position always inside screen,
		// because k_moveMouseCursor adjusts mouse position when it's outside screen. 
		k_moveMouseCursor(mouseX, mouseY);
		k_getMouseCursorPos(&mouseX, &mouseY);

		changedButtonStatus = windowManager->prevButtonStatus ^ buttonStatus;

		// If mouse button changed, process mouse data immediately,
		// because mouse button data can not be integrated.
		if (changedButtonStatus != 0) {
			break;
		}
	}

	/* process mouse data */

	underMouseWindowId = k_findWindowByPoint(mouseX, mouseY);

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

			// [Temp] This code is for test.
			k_createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_helloWorldGuiTask, TASK_AFFINITY_LOADBALANCING);

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
	Event events[WINDOWMANAGER_DATAINTEGRATIONCOUNT];
	int eventCount;
	ScreenUpdateEvent* screenUpdateEvent;
	ScreenUpdateEvent* nextScreenUpdateEvent;
	Rect area;
	int i, j;

	/* accumulate window manager event */
	for (i = 0; i < WINDOWMANAGER_DATAINTEGRATIONCOUNT; i++) {
		if (k_recvEventFromWindowManager(&events[i]) == false) {
			if (i == 0) {
				return false;

			} else {
				break;
			}
		}

		// save area of screen update by ID event,
		// by converting area from screen coordinates to window coordinates.
		if (events[i].type == EVENT_SCREENUPDATE_BYID) {
			screenUpdateEvent = &events[i].screenUpdateEvent;

			if (k_getWindowArea(screenUpdateEvent->windowId, &area) == false) {
				k_setRect(&screenUpdateEvent->area, 0, 0, 0, 0);

			} else {
				k_setRect(&screenUpdateEvent->area, 0, 0, k_getRectWidth(&area) - 1, k_getRectHeight(&area) - 1);
			}
		}
	}
	
	/* integrate window manager event: integrate screen update event with same window ID and included area. */
	eventCount = i;
	for (i = 0; i < eventCount; i++) {
		if ((events[i].type != EVENT_SCREENUPDATE_BYID) && (events[i].type != EVENT_SCREENUPDATE_BYWINDOWAREA) && (events[i].type != EVENT_SCREENUPDATE_BYSCREENAREA)) {
			continue;
		}

		screenUpdateEvent = &events[i].screenUpdateEvent;

		for (j = i + 1; j < eventCount; j++) {
			if ((events[j].type != EVENT_SCREENUPDATE_BYID) && (events[j].type != EVENT_SCREENUPDATE_BYWINDOWAREA) && (events[j].type != EVENT_SCREENUPDATE_BYSCREENAREA)) {
				continue;
			}

			nextScreenUpdateEvent = &events[j].screenUpdateEvent;

			// - valid window ID: screen update by ID event (window coordinates)
			//                   ,screen update by window area event (window coordinates)
			// - invalid window ID: screen update by screen area event (screen coordinates)
			// Area of valid and same window ID event can be integrated,
			// and area of invalid window ID event can be integrated.
			// It means that area of window coordinates with same window ID can be integrated,
			// and area of screen coordinates can be integrated.
			if (screenUpdateEvent->windowId != nextScreenUpdateEvent->windowId) {
				continue;
			}

			if (k_getOverlappedRect(&screenUpdateEvent->area, &nextScreenUpdateEvent->area, &area) == false) {
				continue;
			}

			// If current event area is included in next event area,
			// select next event area which is larger one to integrate those two areas.
			if (k_memcmp(&screenUpdateEvent->area, &area, sizeof(Rect)) == 0) {
				k_memcpy(&screenUpdateEvent->area, &nextScreenUpdateEvent->area, sizeof(Rect));
				events[j].type = EVENT_UNKNOWN;

			// If next event area is included in current event area,
			// select current event area which is larger one to integrate those two areas.
			} else if (k_memcmp(&nextScreenUpdateEvent->area, &area, sizeof(Rect)) == 0) {
				events[j].type = EVENT_UNKNOWN;
			}
		}
	}

	/* process window manager event */
	for (i = 0; i < eventCount; i++) {
		switch (events[i].type) {
		case EVENT_SCREENUPDATE_BYID:         // window coordinates
		case EVENT_SCREENUPDATE_BYWINDOWAREA: // window coordinates
			screenUpdateEvent = &events[i].screenUpdateEvent;

			if (k_convertRectWindowToScreen(screenUpdateEvent->windowId, &screenUpdateEvent->area, &area) == true) {
				k_redrawWindowByArea(screenUpdateEvent->windowId, &area);
			}

			break;

		case EVENT_SCREENUPDATE_BYSCREENAREA: // screen coordinates
			screenUpdateEvent = &events[i].screenUpdateEvent;

			k_redrawWindowByArea(WINDOW_INVALIDID, &screenUpdateEvent->area);

			break;

		default:
			break;
		}
	}

	return true;
}