#include "shell.h"
#include "../core/window.h"
#include "../core/util.h"
#include "../core/console.h"
#include "../core/task.h"
#include "../core/shell.h"
#include "../fonts/fonts.h"

static Char g_prevScreenBuffer[CONSOLE_WIDTH * CONSOLE_HEIGHT];

void k_guiShellTask(void) {
	static qword windowId = WINDOW_INVALIDID;
	Rect screenArea;
	int windowWidth;
	int windowHeight;
	Task* shellTask;
	qword shellTaskId;	
	Event event;
	KeyEvent* keyEvent;
	Key key;

	/* check graphic mode */
	if (k_isGraphicMode() == false) {
		k_printf("GUI shell task error: not graphic mode\n");
		return;
	}

	// GUI shell window had better exist only one,
	// even though user executes shell app many times.
	if (windowId != WINDOW_INVALIDID) {
		k_moveWindowToTop(windowId);
		return;
	}

	/* create window */
	k_getScreenArea(&screenArea);

	// The window has 2 pixels-thick free space on left, 2 pixels-thick free space on right,
	// title bar on top, and 2 pixels-thick free space on bottom.
	windowWidth = FONT_VERAMONO_ENG_WIDTH * CONSOLE_WIDTH + 4;
	windowHeight = FONT_VERAMONO_ENG_HEIGHT * CONSOLE_HEIGHT + WINDOW_TITLEBAR_HEIGHT + 2;

	windowId = k_createWindow((screenArea.x2 - windowWidth) / 2, (screenArea.y2 - windowHeight) / 2, windowWidth, windowHeight, WINDOW_FLAGS_DEFAULT, "Shell");
	if (windowId == WINDOW_INVALIDID) {
		return;
	}

	/* create shell task */
	k_setShellExitFlag(false);
	shellTask = k_createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_shellTask, TASK_AFFINITY_LOADBALANCING);
	if (shellTask == null) {
		k_deleteWindow(windowId);
		return;
	}

	shellTaskId = shellTask->link.id;

	k_memset(g_prevScreenBuffer, 0xFF, sizeof(g_prevScreenBuffer));

	/* event processing loop */
	while (true) {
		k_processConsoleScreenBuffer(windowId);

		if (k_recvEventFromWindow(&event, windowId) == false) {
			k_sleep(0);
			continue;
		}

		switch (event.type) {
		case EVENT_WINDOW_CLOSE:
			k_setShellExitFlag(true);
			while (k_existTask(shellTaskId) == true) {
				k_sleep(1);
			}

			k_deleteWindow(windowId);
			windowId = WINDOW_INVALIDID;

			return;

		case EVENT_KEY_DOWN:
		case EVENT_KEY_UP:
			keyEvent = &event.keyEvent;

			key.scanCode = keyEvent->scanCode;
			key.asciiCode = keyEvent->asciiCode;
			key.flags = keyEvent->flags;

			k_putKeyToConsoleKeyQueue(&key);

			break;

		default:
			break;
		}
	}
}

static void k_processConsoleScreenBuffer(qword windowId) {
	ConsoleManager* consoleManager;
	Char* screenBuffer;
	Char* prevScreenBuffer;
	static qword lastTickCount = 0;
	bool fullRedraw;
	bool changed;
	Rect lineArea;
	int i, j;

	consoleManager = k_getConsoleManager();
	screenBuffer = consoleManager->screenBuffer;
	prevScreenBuffer = g_prevScreenBuffer;

	// full-redraw console screen every 5 seconds.
	if (k_getTickCount() - lastTickCount > 5000) {
		lastTickCount = k_getTickCount();
		fullRedraw = true;

	} else {
		fullRedraw = false;
	}

	for (i = 0; i < CONSOLE_HEIGHT; i++) {
		changed = false;

		for (j = 0; j < CONSOLE_WIDTH; j++) {
			if ((screenBuffer->char_ != prevScreenBuffer->char_) || (fullRedraw == true)) {
				k_drawText(windowId, FONT_VERAMONO_ENG_WIDTH * j + 2, FONT_VERAMONO_ENG_HEIGHT * i + WINDOW_TITLEBAR_HEIGHT, RGB(0, 255, 0), RGB(0, 0, 0), &screenBuffer->char_);
				k_memcpy(prevScreenBuffer, screenBuffer, sizeof(Char));
				changed = true;
			}

			screenBuffer++;
			prevScreenBuffer++;
		}

		// update current line of console screen if current line has the changes.
		if (changed == true) {
			k_setRect(&lineArea, 2, FONT_VERAMONO_ENG_HEIGHT * i + WINDOW_TITLEBAR_HEIGHT, FONT_VERAMONO_ENG_WIDTH * CONSOLE_WIDTH + 5, FONT_VERAMONO_ENG_HEIGHT * (i + 1) + WINDOW_TITLEBAR_HEIGHT - 1);
			k_updateScreenByWindowArea(windowId, &lineArea);
		}
	}
}