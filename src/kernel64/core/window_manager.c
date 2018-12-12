#include "window_manager.h"
#include "window.h"
#include "../utils/util.h"
#include "mouse.h"
#include "keyboard.h"
#include "task.h"
#include "../gui_tasks/system_menu.h"
#include "widgets.h"

/**
  < Screen Update Performance Test >
  - LPS: Loop Per Second
  - Target loop is window manager task loop which has something to do with the screen update performance.
  - This test checks minimum loop count among loop counts during 1 second.
  - The greater loop count means the better performance.
  
  < Test Result >
    This test used Hello World Window.
    Window Count : MIN Loop Count (Windows QEMU) : MIN Loop Count (Mac QEMU)
    - 0   : 315 : 330
    - 1   : 270 : 330
    - 20  : 200 : 290
    - 40  : 110 : 260
    - 90  : 35  : 230
    - 170 : 35  : 200
*/

#if __DEBUG__
volatile qword g_winMgrMinLoopCount = 0xFFFFFFFFFFFFFFFF;
#endif // __DEBUG__

void k_windowManagerTask(void) {
	int mouseX, mouseY;
	bool mouseResult;
	bool keyResult;
	bool windowManagerResult;
	WindowManager* windowManager;
	#if __DEBUG__
	/* Screen Update Performance Test */
	qword lastTickCount;
	qword loopCount;
	qword prevLoopCount;
	#endif // __DEBUG__
	
	// initialize GUI system.
	k_initGuiSystem();
	
	// draw mouse cursor at current mouse position (center in screen).
	k_getMouseCursorPos(&mouseX, &mouseY);
	k_moveMouseCursor(mouseX, mouseY);

	// create system menu task.
	k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_SYSTEM | TASK_FLAGS_THREAD, null, 0, (qword)k_systemMenuTask, TASK_AFFINITY_LB);
	
	// get window manager.
	windowManager = k_getWindowManager();

	// initialize clock manager.
	k_initClockManager();

	#if __DEBUG__
	/* Screen Update Performance Test */
	lastTickCount = k_getTickCount();
	loopCount = 0;
	prevLoopCount = 0;
	#endif // __DEBUG__
	
	/* window manager task loop */
	while (true) {
		#if __DEBUG__
		/* Screen Update Performance Test */
		// print minimum loop count among loop counts during 1 second.
		if (k_getTickCount() - lastTickCount > 1000) {
			lastTickCount = k_getTickCount();
			
			if (loopCount - prevLoopCount < g_winMgrMinLoopCount) {
				g_winMgrMinLoopCount = loopCount - prevLoopCount;
			}
			
			prevLoopCount = loopCount;			
		}
		
		loopCount++;
		#endif // __DEBUG__
		
		// process mouse data.
		mouseResult = k_processMouseData();

		// process key.
		keyResult = k_processKey();

		// process window manager event.
		windowManagerResult = false;
		while (k_processWindowManagerEvent() == true) {
			windowManagerResult = true;
		}
		
		// If window manager event (screen update event) had occured, resize marker might have been cleared,
		// so draw resize marker again.
		if ((windowManagerResult == true) && (windowManager->resizing == true)) {
			k_drawResizeMarker(&windowManager->resizingArea, true);
		}

		// draw all clocks.
		k_drawAllClocks();
		
		// If no data/events have been processed, switch task.
		if ((mouseResult == false) && (keyResult == false) && (windowManagerResult == false)) {
			k_sleep(0);
		}
	}
}

static bool k_processMouseData(void) {
	qword underMouseId;
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
	for (i = 0; i < WINMGR_DATAINTEGRATIONCOUNT; i++) {
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
	underMouseId = k_findWindowByPoint(mouseX, mouseY);

	/* left button changed */
	if (changedButtonStatus & MOUSE_LBUTTONDOWN) {
		/* left button down */
		if (buttonStatus & MOUSE_LBUTTONDOWN) {
			if (underMouseId != windowManager->backgroundId) {
				k_moveWindowToTop(underMouseId);
			}

			if (k_isPointInTitleBar(underMouseId, mouseX, mouseY) == true) {
				if (k_isPointInCloseButton(underMouseId, mouseX, mouseY) == true) {
					k_sendWindowEventToWindow(underMouseId, EVENT_WINDOW_CLOSE);

				} else if (k_isPointInResizeButton(underMouseId, mouseX, mouseY) == true) {
					windowManager->resizing = true;
					windowManager->resizingId = underMouseId;
					k_getWindowArea(underMouseId, &windowManager->resizingArea);
					k_drawResizeMarker(&windowManager->resizingArea, true);

				} else if (k_isPointInTopMenu(underMouseId, mouseX, mouseY) == true) {
					k_sendMenuEventToWindow(underMouseId, EVENT_TOPMENU_CLICK, mouseX, mouseY);

				} else {
					windowManager->moving = true;
					windowManager->movingId = underMouseId;
				}

			} else {
				k_sendMouseEventToWindow(underMouseId, EVENT_MOUSE_LBUTTONDOWN, mouseX, mouseY, buttonStatus);
			}

		/* left button up */
		} else {
			if (windowManager->moving == true) {
				windowManager->moving = false;
				windowManager->movingId = WINDOW_INVALIDID;

			} else if (windowManager->resizing == true) {
				k_resizeWindow(windowManager->resizingId
							  ,windowManager->resizingArea.x1
							  ,windowManager->resizingArea.y1
							  ,k_getRectWidth(&windowManager->resizingArea)
							  ,k_getRectHeight(&windowManager->resizingArea));

				k_drawResizeMarker(&windowManager->resizingArea, false);

				windowManager->resizing = false;
				windowManager->resizingId = WINDOW_INVALIDID;

			} else {
				k_sendMouseEventToWindow(underMouseId, EVENT_MOUSE_LBUTTONUP, mouseX, mouseY, buttonStatus);
			}
		}

	/* right button changed */
	} else if (changedButtonStatus & MOUSE_RBUTTONDOWN) {
		/* right button down */
		if (buttonStatus & MOUSE_RBUTTONDOWN) {
			k_sendMouseEventToWindow(underMouseId, EVENT_MOUSE_RBUTTONDOWN, mouseX, mouseY, buttonStatus);

		/* right button up */
		} else {
			k_sendMouseEventToWindow(underMouseId, EVENT_MOUSE_RBUTTONUP, mouseX, mouseY, buttonStatus);
		}

	/* middle button changed */
	} else if (changedButtonStatus & MOUSE_MBUTTONDOWN) {
		/* middle button down */
		if (buttonStatus & MOUSE_MBUTTONDOWN) {
			k_sendMouseEventToWindow(underMouseId, EVENT_MOUSE_MBUTTONDOWN, mouseX, mouseY, buttonStatus);

		/* middle button up */
		} else {
			k_sendMouseEventToWindow(underMouseId, EVENT_MOUSE_MBUTTONUP, mouseX, mouseY, buttonStatus);
		}

	/* no buttons changed */
	} else {
		k_sendMouseEventToWindow(underMouseId, EVENT_MOUSE_MOVE, mouseX, mouseY, buttonStatus);
				
		if (k_isPointInCloseButton(underMouseId, mouseX, mouseY) == true) {
			if (windowManager->overCloseId == WINDOW_INVALIDID) {
				windowManager->overCloseId = underMouseId;
				k_updateCloseButton(underMouseId, true);
			}

		} else {
			if (windowManager->overCloseId != WINDOW_INVALIDID) {
				k_updateCloseButton(windowManager->overCloseId, false);
				windowManager->overCloseId = WINDOW_INVALIDID;				
			}
		}

		if (k_isPointInResizeButton(underMouseId, mouseX, mouseY) == true) {
			if (windowManager->overResizeId == WINDOW_INVALIDID) {
				windowManager->overResizeId = underMouseId;
				k_updateResizeButton(underMouseId, true);			
			}

		} else {
			if (windowManager->overResizeId != WINDOW_INVALIDID && windowManager->resizing == false) {
				k_updateResizeButton(windowManager->overResizeId, false);
				windowManager->overResizeId = WINDOW_INVALIDID;			
			}
		}

		if (k_isPointInTopMenu(underMouseId, mouseX, mouseY) == true) {
			if ((windowManager->moving == false) && (windowManager->resizing == false)) {
				windowManager->overMenuId = underMouseId;
				k_processTopMenuActivity(underMouseId, mouseX, mouseY);			
			}

		} else {
			if (windowManager->overMenuId != WINDOW_INVALIDID) {
				k_clearTopMenuActivity(windowManager->overMenuId, mouseX, mouseY);
				windowManager->overMenuId = WINDOW_INVALIDID;
			}
		}
	}

	/* process window moving */
	if (windowManager->moving == true) {
		if (k_getWindowArea(windowManager->movingId, &windowArea) == true) {
			k_moveWindow(windowManager->movingId, windowArea.x1 + mouseX - prevMouseX, windowArea.y1 + mouseY - prevMouseY);

		} else {
			windowManager->moving = false;
			windowManager->movingId = WINDOW_INVALIDID;
		}

	/* process window resizing */
	} else if (windowManager->resizing == true) {
		k_drawResizeMarker(&windowManager->resizingArea, false);

		windowManager->resizingArea.x2 += mouseX - prevMouseX;
		windowManager->resizingArea.y1 += mouseY - prevMouseY;

		if ((windowManager->resizingArea.x2 < windowManager->resizingArea.x1) || (k_getRectWidth(&windowManager->resizingArea) < WINDOW_MINWIDTH)) {
			windowManager->resizingArea.x2 = windowManager->resizingArea.x1 + WINDOW_MINWIDTH - 1;	
		}

		if ((windowManager->resizingArea.y2 < windowManager->resizingArea.y1) || (k_getRectHeight(&windowManager->resizingArea) < WINDOW_MINHEIGHT)) {
			windowManager->resizingArea.y1 = windowManager->resizingArea.y2 - WINDOW_MINHEIGHT + 1;	
		}

		k_drawResizeMarker(&windowManager->resizingArea, true);
	}
	
	/* send mouse out event */	
	if ((windowManager->prevUnderMouseId != WINDOW_INVALIDID) && (windowManager->prevUnderMouseId != underMouseId)) {
		k_sendMouseEventToWindow(windowManager->prevUnderMouseId, EVENT_MOUSE_OUT, mouseX, mouseY, buttonStatus);	
	}

	windowManager->prevButtonStatus = buttonStatus;
	windowManager->prevUnderMouseId = underMouseId;

	return true;
}

static bool k_processKey(void) {
	Key key;

	if (k_getKeyFromKeyQueue(&key) == false) {
		return false;
	}

	k_sendKeyEventToWindow(k_getTopWindowId(), &key);

	return true;
}

static bool k_processWindowManagerEvent(void) {
	Event events[WINMGR_DATAINTEGRATIONCOUNT];
	int eventCount;
	ScreenUpdateEvent* screenUpdateEvent;
	ScreenUpdateEvent* nextScreenUpdateEvent;
	Rect area;
	int i, j;

	/* accumulate window manager event */
	for (i = 0; i < WINMGR_DATAINTEGRATIONCOUNT; i++) {
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