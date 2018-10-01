#include "shell.h"
#include "../core/window.h"
#include "../utils/util.h"
#include "../core/console.h"
#include "../core/task.h"
#include "../core/shell.h"
#include "../fonts/fonts.h"
#include "../core/multiprocessor.h"

static Char g_prevScreenBuffer[CONSOLE_WIDTH * CONSOLE_HEIGHT];
volatile qword g_guiShellWindowId = WINDOW_INVALIDID;

void k_guiShellTask(void) {
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
	if (g_guiShellWindowId != WINDOW_INVALIDID) {
		k_moveWindowToTop(g_guiShellWindowId);
		return;
	}

	/* create window */
	k_getScreenArea(&screenArea);

	// The window has 2 pixels-thick free space on left, 2 pixels-thick free space on right,
	// title bar on top, and 2 pixels-thick free space on bottom.
	windowWidth = FONT_DEFAULT_WIDTH * CONSOLE_WIDTH + 4;
	windowHeight = FONT_DEFAULT_HEIGHT * CONSOLE_HEIGHT + WINDOW_TITLEBAR_HEIGHT + 2;

	g_guiShellWindowId = k_createWindow((screenArea.x2 - windowWidth) / 2, (screenArea.y2 - windowHeight) / 2, windowWidth, windowHeight, WINDOW_FLAGS_DEFAULT, "Shell");
	if (g_guiShellWindowId == WINDOW_INVALIDID) {
		return;
	}

	/* create shell task */
	k_setShellExitFlag(false);

	shellTask = k_createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_shellTask, TASK_AFFINITY_LOADBALANCING);
	if (shellTask == null) {
		k_deleteWindow(g_guiShellWindowId);
		return;
	}

	shellTaskId = shellTask->link.id;

	k_memset(g_prevScreenBuffer, 0xFF, sizeof(g_prevScreenBuffer));
	
	/* event processing loop */
	while (true) {
		k_processConsoleScreenBuffer(g_guiShellWindowId);

		if (k_recvEventFromWindow(&event, g_guiShellWindowId) == false) {
			k_sleep(0);
			continue;
		}

		switch (event.type) {
		case EVENT_WINDOW_CLOSE:
			k_setShellExitFlag(true);
			while (k_existTask(shellTaskId) == true) {
				k_sleep(1);
			}

			k_deleteWindow(g_guiShellWindowId);
			g_guiShellWindowId = WINDOW_INVALIDID;

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
				k_drawText(windowId, FONT_DEFAULT_WIDTH * j + 2, FONT_DEFAULT_HEIGHT * i + WINDOW_TITLEBAR_HEIGHT, RGB(0, 255, 0), RGB(0, 0, 0), &screenBuffer->char_, 1);
				k_memcpy(prevScreenBuffer, screenBuffer, sizeof(Char));
				changed = true;
			}

			screenBuffer++;
			prevScreenBuffer++;
		}

		// update current line of console screen if current line has the changes.
		if (changed == true) {
			k_setRect(&lineArea, 2, FONT_DEFAULT_HEIGHT * i + WINDOW_TITLEBAR_HEIGHT, FONT_DEFAULT_WIDTH * CONSOLE_WIDTH + 5, FONT_DEFAULT_HEIGHT * (i + 1) + WINDOW_TITLEBAR_HEIGHT - 1);
			k_updateScreenByWindowArea(windowId, &lineArea);
		}
	}
}