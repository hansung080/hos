#include "event_monitor.h"
#include "../core/window.h"
#include "../core/util.h"
#include "../core/console.h"

void k_eventMonitorTask(void) {
	int mouseX, mouseY;
	int windowWidth, windowHeight;
	qword windowId;	
	Event event;
	MouseEvent* mouseEvent;
	WindowEvent* windowEvent;
	KeyEvent* keyEvent;
	UserEvent* userEvent;
	int y;
	char tempBuffer[50];
	static int windowCount = 0;
	Rect buttonArea;
	qword foundWindowId;
	Event sendEvent;
	int i;	
	char* eventStrs[] = {
		"UNKNOWN",
		"MOUSE_MOVE",
		"MOUSE_LBUTTONDOWN",
		"MOUSE_LBUTTONUP",
		"MOUSE_RBUTTONDOWN",
		"MOUSE_RBUTTONUP",
		"MOUSE_MBUTTONDOWN",
		"MOUSE_MBUTTONUP",
		"WINDOW_SELECT",
		"WINDOW_DESELECT",
		"WINDOW_MOVE",
		"WINDOW_RESIZE",
		"WINDOW_CLOSE",
		"KEY_DOWN",
		"KEY_UP"
	};

	/* check graphic mode */
	if (k_isGraphicMode() == false) {
		k_printf("event monitor task error: not graphic mode\n");
		return;
	}

	/* create window */
	k_getMouseCursorPos(&mouseX, &mouseY);
	windowWidth = 500;
	windowHeight = 200;
	k_sprintf(tempBuffer, "Event Monitor %d", ++windowCount);
	windowId = k_createWindow(mouseX - 10, mouseY - WINDOW_TITLEBAR_HEIGHT / 2, windowWidth, windowHeight, WINDOW_FLAGS_DEFAULT, tempBuffer);
	if (windowId == WINDOW_INVALIDID) {
		return;
	}

	/* draw GUI event info area */
	y = WINDOW_TITLEBAR_HEIGHT + 10;
	k_drawRect(windowId, 10, y + 8, windowWidth - 10, y + 70, RGB(0, 0, 0), false);
	k_sprintf(tempBuffer, "GUI Event Info (window ID: 0x%q)", windowId);
	k_drawText(windowId, 20, y, RGB(0, 0, 0), RGB(255, 255, 255), tempBuffer, k_strlen(tempBuffer));

	/* draw user event send button */
	k_setRect(&buttonArea, 10, y + 80, windowWidth - 10, windowHeight - 10);
	k_drawButton(windowId, &buttonArea, WINDOW_COLOR_BACKGROUND, "Send User Event", RGB(0, 0, 0));
	k_showWindow(windowId, true);

	/* event processing loop */
	while (true) {
		if (k_recvEventFromWindow(&event, windowId) == false) {
			k_sleep(0);
			continue;
		}

		// clear previous event info.
		k_drawRect(windowId, 11, y + 20, windowWidth - 11, y + 69, WINDOW_COLOR_BACKGROUND, true);

		switch (event.type) {
		case EVENT_MOUSE_MOVE:
		case EVENT_MOUSE_LBUTTONDOWN:
		case EVENT_MOUSE_LBUTTONUP:
		case EVENT_MOUSE_RBUTTONDOWN:
		case EVENT_MOUSE_RBUTTONUP:
		case EVENT_MOUSE_MBUTTONDOWN:
		case EVENT_MOUSE_MBUTTONUP:
			mouseEvent = &event.mouseEvent;

			// print event type.
			k_sprintf(tempBuffer, "- type: %s", eventStrs[event.type]);	
			k_drawText(windowId, 20, y + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, k_strlen(tempBuffer));

			// print mouse data.
			k_sprintf(tempBuffer, "- data: point: (%d, %d), button status: 0x%x", mouseEvent->point.x, mouseEvent->point.y, mouseEvent->buttonStatus);
			k_drawText(windowId, 20, y + 40, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, k_strlen(tempBuffer));

			/* process button event using mouse event */
			if (event.type == EVENT_MOUSE_LBUTTONDOWN) {
				if (k_isPointInRect(&buttonArea, mouseEvent->point.x, mouseEvent->point.y) == true) {
					// draw button with bright green color in order to express button down.
					k_drawButton(windowId, &buttonArea, EVENTMONITOR_COLOR_BUTTONACTIVE, "Send User Event", RGB(255, 255, 255));
					k_updateScreenById(windowId);

					// set user event
					sendEvent.type = EVENT_USER_TESTMESSAGE;
					sendEvent.userEvent.data[0] = windowId;
					sendEvent.userEvent.data[1] = 0x1234;
					sendEvent.userEvent.data[2] = 0x5678;

					// send user event to other windows except itself.
					for (i = 1; i <= windowCount; i++) {
						k_sprintf(tempBuffer, "Event Monitor %d", i);	
						foundWindowId = k_findWindowByTitle(tempBuffer);
						if ((foundWindowId != WINDOW_INVALIDID) && (foundWindowId != windowId)) {
							k_sendEventToWindow(&sendEvent, foundWindowId);
						}
					}
				}

			} else if (event.type == EVENT_MOUSE_LBUTTONUP) {
				// draw button with original color in order to express button up.
				k_drawButton(windowId, &buttonArea, WINDOW_COLOR_BACKGROUND, "Send User Event", RGB(0, 0, 0));
			}

			break;

		case EVENT_WINDOW_SELECT:
		case EVENT_WINDOW_DESELECT:
		case EVENT_WINDOW_MOVE:
		case EVENT_WINDOW_RESIZE:
		case EVENT_WINDOW_CLOSE:
			windowEvent = &event.windowEvent;

			// print event type.
			k_sprintf(tempBuffer, "- type: %s", eventStrs[event.type]);	
			k_drawText(windowId, 20, y + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, k_strlen(tempBuffer));

			// print window data.
			k_sprintf(tempBuffer, "- data: area: (%d, %d) (%d, %d)", windowEvent->area.x1, windowEvent->area.y1, windowEvent->area.x2, windowEvent->area.y2);
			k_drawText(windowId, 20, y + 40, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, k_strlen(tempBuffer));

			// delete window.
			if (event.type == EVENT_WINDOW_CLOSE) {
				k_deleteWindow(windowId);
				return;
			}

			break;

		case EVENT_KEY_DOWN:
		case EVENT_KEY_UP:
			keyEvent = &event.keyEvent;

			// print event type.
			k_sprintf(tempBuffer, "- type: %s", eventStrs[event.type]);	
			k_drawText(windowId, 20, y + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, k_strlen(tempBuffer));

			// print key data.
			k_sprintf(tempBuffer, "- data: scan: 0x%x, ASCII: 0x%x (%d, %c), flags: 0x%x", keyEvent->scanCode, keyEvent->asciiCode, keyEvent->asciiCode, keyEvent->asciiCode, keyEvent->flags);
			k_drawText(windowId, 20, y + 40, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, k_strlen(tempBuffer));

			break;

		case EVENT_USER_TESTMESSAGE:
			userEvent = &event.userEvent;

			// print event type.
			k_sprintf(tempBuffer, "- type: USER_TESTMESSAGE");
			k_drawText(windowId, 20, y + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, k_strlen(tempBuffer));

			// print user data.
			k_sprintf(tempBuffer, "- data: 0x%q, 0x%q, 0x%q", userEvent->data[0], userEvent->data[1], userEvent->data[2]);
			k_drawText(windowId, 20, y + 40, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, k_strlen(tempBuffer));

			break;			

		default:
			break;
		}

		k_showWindow(windowId, true);
	}
}