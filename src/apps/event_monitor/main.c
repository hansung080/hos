#include <hanslib.h>
#include "defines.h"

int main(const char* args) {
	ArgList argList;
	char option[ARG_MAXLENGTH] = {'\0', };
	dword windowFlags;
	int mouseX, mouseY;
	int windowWidth, windowHeight;
	qword windowId;	
	Event event;
	MouseEvent* mouseEvent;
	WindowEvent* windowEvent;
	KeyEvent* keyEvent;
	UserEvent* userEvent;
	int y;
	char tempBuffer[1024];
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
	if (isGraphicMode() == false) {
		printf("[event monitor error] not graphic mode\n");
		return -1;
	}

	/* print argument string */
	#if 0
	ArgList argList;
	char arg[ARG_MAXLENGTH] = {'\0', };

	initArgs(&argList, args);
	printf("[event monitor debug] args: ");
	while (true) {
		if (getNextArg(&argList, arg) <= 0) {
			break;
		}

		printf("'%s', ", arg);
	}
	printf("\n");
	#endif

	/* get arguments */
	initArgs(&argList, args);

	getNextArg(&argList, option);

	if (strcmp(option, "-nb") == 0) {
		windowFlags = WINDOW_FLAGS_DEFAULT;

	} else {
		windowFlags = WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_BLOCKING;				
	}

	/* create window */
	getMouseCursorPos(&mouseX, &mouseY);
	windowWidth = 500;
	windowHeight = 200;
	sprintf(tempBuffer, "Event Monitor %d", ++windowCount);
	windowId = createWindow(mouseX - 10, mouseY - WINDOW_TITLEBAR_HEIGHT / 2, windowWidth, windowHeight, windowFlags, tempBuffer);
	if (windowId == WINDOW_INVALIDID) {
		return -1;
	}

	/* draw argument string */
	y = WINDOW_TITLEBAR_HEIGHT + 5;
	sprintf(tempBuffer, "# args: '%s'", args);
	drawText(windowId, 20, y, RGB(0, 0, 0), RGB(255, 255, 255), tempBuffer, strlen(tempBuffer));

	/* draw GUI event info area */
	y += 25;
	drawRect(windowId, 10, y + 8, windowWidth - 10, y + 70, RGB(0, 0, 0), false);
	sprintf(tempBuffer, "GUI Event Info (window ID: 0x%q)", windowId);
	drawText(windowId, 20, y, RGB(0, 0, 0), RGB(255, 255, 255), tempBuffer, strlen(tempBuffer));

	/* draw user event send button */
	setRect(&buttonArea, 10, y + 80, windowWidth - 10, windowHeight - 10);
	drawButton(windowId, &buttonArea, WINDOW_COLOR_BACKGROUND, "Send User Event", RGB(0, 0, 0));
	showWindow(windowId, true);

	/* event processing loop */
	while (true) {
		if (recvEventFromWindow(&event, windowId) == false) {
			sleep(0);
			continue;
		}

		// clear previous event info.
		drawRect(windowId, 11, y + 20, windowWidth - 11, y + 69, WINDOW_COLOR_BACKGROUND, true);

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
			sprintf(tempBuffer, "- type: %s", eventStrs[event.type]);	
			drawText(windowId, 20, y + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, strlen(tempBuffer));

			// print mouse data.
			sprintf(tempBuffer, "- data: point: (%d, %d), button status: 0x%x", mouseEvent->point.x, mouseEvent->point.y, mouseEvent->buttonStatus);
			drawText(windowId, 20, y + 40, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, strlen(tempBuffer));

			/* process button event using mouse event */
			if (event.type == EVENT_MOUSE_LBUTTONDOWN) {
				if (isPointInRect(&buttonArea, mouseEvent->point.x, mouseEvent->point.y) == true) {
					// draw button with bright green color in order to express button down.
					drawButton(windowId, &buttonArea, EVENTMONITOR_COLOR_BUTTONACTIVE, "Send User Event", RGB(255, 255, 255));
					updateScreenById(windowId);

					// set user event
					sendEvent.type = EVENT_USER_TESTMESSAGE;
					sendEvent.userEvent.data[0] = windowId;
					sendEvent.userEvent.data[1] = 0x1234;
					sendEvent.userEvent.data[2] = 0x5678;

					// send user event to other windows except itself.
					for (i = 1; i <= windowCount; i++) {
						sprintf(tempBuffer, "Event Monitor %d", i);	
						foundWindowId = findWindowByTitle(tempBuffer);
						if ((foundWindowId != WINDOW_INVALIDID) && (foundWindowId != windowId)) {
							sendEventToWindow(&sendEvent, foundWindowId);
						}
					}
				}

			} else if (event.type == EVENT_MOUSE_LBUTTONUP) {
				// draw button with original color in order to express button up.
				drawButton(windowId, &buttonArea, WINDOW_COLOR_BACKGROUND, "Send User Event", RGB(0, 0, 0));
			}

			break;

		case EVENT_WINDOW_SELECT:
		case EVENT_WINDOW_DESELECT:
		case EVENT_WINDOW_MOVE:
		case EVENT_WINDOW_RESIZE:
		case EVENT_WINDOW_CLOSE:
			windowEvent = &event.windowEvent;

			// print event type.
			sprintf(tempBuffer, "- type: %s", eventStrs[event.type]);	
			drawText(windowId, 20, y + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, strlen(tempBuffer));

			// print window data.
			sprintf(tempBuffer, "- data: area: (%d, %d) (%d, %d)", windowEvent->area.x1, windowEvent->area.y1, windowEvent->area.x2, windowEvent->area.y2);
			drawText(windowId, 20, y + 40, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, strlen(tempBuffer));

			// delete window.
			if (event.type == EVENT_WINDOW_CLOSE) {
				deleteWindow(windowId);
				return 0;
			}

			break;

		case EVENT_KEY_DOWN:
		case EVENT_KEY_UP:
			keyEvent = &event.keyEvent;

			// print event type.
			sprintf(tempBuffer, "- type: %s", eventStrs[event.type]);	
			drawText(windowId, 20, y + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, strlen(tempBuffer));

			// print key data.
			sprintf(tempBuffer, "- data: scan: 0x%x, ASCII: 0x%x (%d, %c), flags: 0x%x", keyEvent->scanCode, keyEvent->asciiCode, keyEvent->asciiCode, keyEvent->asciiCode, keyEvent->flags);
			drawText(windowId, 20, y + 40, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, strlen(tempBuffer));

			break;

		case EVENT_USER_TESTMESSAGE:
			userEvent = &event.userEvent;

			// print event type.
			sprintf(tempBuffer, "- type: USER_TESTMESSAGE");
			drawText(windowId, 20, y + 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, strlen(tempBuffer));

			// print user data.
			sprintf(tempBuffer, "- data: 0x%q, 0x%q, 0x%q", userEvent->data[0], userEvent->data[1], userEvent->data[2]);
			drawText(windowId, 20, y + 40, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, tempBuffer, strlen(tempBuffer));

			break;			

		default:
			break;
		}

		showWindow(windowId, true);
	}

	return 0;
}