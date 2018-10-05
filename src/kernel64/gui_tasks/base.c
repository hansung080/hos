#include "base.h"
#include "../core/window.h"
#include "../utils/util.h"
#include "../core/console.h"

#if __DEBUG__
void k_baseTask(void) {
	int mouseX, mouseY;
	int windowWidth, windowHeight;
	qword windowId;	
	Event event;
	MouseEvent* mouseEvent;
	WindowEvent* windowEvent;
	KeyEvent* keyEvent;

	/* check graphic mode */
	if (k_isGraphicMode() == false) {
		k_printf("[base error] not graphic mode\n");
		return;
	}

	/* create window */
	k_getMouseCursorPos(&mouseX, &mouseY);
	windowWidth = 500;
	windowHeight = 200;
	windowId = k_createWindow(mouseX - 10, mouseY - WINDOW_TITLEBAR_HEIGHT / 2, windowWidth, windowHeight, WINDOW_FLAGS_DEFAULT, "Base");
	if (windowId == WINDOW_INVALIDID) {
		return;
	}

	/* event processing loop */
	while (true) {
		if (k_recvEventFromWindow(&event, windowId) == false) {
			k_sleep(0);
			continue;
		}

		switch (event.type) {
		case EVENT_MOUSE_MOVE:
		case EVENT_MOUSE_LBUTTONDOWN:
		case EVENT_MOUSE_LBUTTONUP:
		case EVENT_MOUSE_RBUTTONDOWN:
		case EVENT_MOUSE_RBUTTONUP:
		case EVENT_MOUSE_MBUTTONDOWN:
		case EVENT_MOUSE_MBUTTONUP:
			mouseEvent = &event.mouseEvent;
			break;

		case EVENT_WINDOW_SELECT:
		case EVENT_WINDOW_DESELECT:
		case EVENT_WINDOW_MOVE:
		case EVENT_WINDOW_RESIZE:
		case EVENT_WINDOW_CLOSE:
			windowEvent = &event.windowEvent;

			if (event.type == EVENT_WINDOW_CLOSE) {
				k_deleteWindow(windowId);
				return;
			}

			break;

		case EVENT_KEY_DOWN:
		case EVENT_KEY_UP:
			keyEvent = &event.keyEvent;
			break;

		default:
			break;
		}
	}
}
#endif // __DEBUG__