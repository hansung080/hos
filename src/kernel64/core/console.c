#include <stdarg.h>
#include "console.h"
#include "keyboard.h"
#include "util.h"
#include "task.h"
#include "asm_util.h"

static ConsoleManager g_consoleManager = {0, };
static Char g_screenBuffer[CONSOLE_WIDTH * CONSOLE_HEIGHT];
static Key g_keyBuffer[CONSOLE_KEYQUEUE_MAXCOUNT];

void k_initConsole(int x, int y) {
	k_memset(&g_consoleManager, 0, sizeof(g_consoleManager));
	k_memset(&g_screenBuffer, 0, sizeof(g_screenBuffer));

	if (k_isGraphicMode() == false) {
		g_consoleManager.screenBuffer = (Char*)CONSOLE_VIDEOMEMORYADDRESS;

	} else {
		g_consoleManager.screenBuffer = g_screenBuffer;
		k_initMutex(&g_consoleManager.mutex);
		k_initQueue(&g_consoleManager.keyQueue, g_keyBuffer, CONSOLE_KEYQUEUE_MAXCOUNT, sizeof(Key));
	}

	k_setCursor(x, y);
}

void k_setCursor(int x, int y) {
	int printOffset;
	int i;
	
	printOffset = y * CONSOLE_WIDTH + x;
	
	if (k_isGraphicMode() == false) {
		// send High Cursor Position Register Select Command (0x0E) to CRTC Control Address Register (0x3D4).
		k_outPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR);
		
		// send high byte of cursor to CRTC Control Data Register (0x3D5).
		k_outPortByte(VGA_PORT_DATA, printOffset >> 8);
		
		// send Low Cursor Position Register Select Command (0x0F) to CRTC Control Address Register (0x3D4).
		k_outPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);
		
		// send low byte of cursor to CRTC Control Data Register (0x3D5).
		k_outPortByte(VGA_PORT_DATA, printOffset & 0xFF);

	} else {
		// clear previous cursor.
		for (i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
			if ((g_consoleManager.screenBuffer[i].char_ == CONSOLE_CURSOR_CHARACTER) && (g_consoleManager.screenBuffer[i].attr == CONSOLE_CURSOR_ATTRIBUTE)) {
				g_consoleManager.screenBuffer[i].char_ = ' ';
				g_consoleManager.screenBuffer[i].attr = CONSOLE_TEXT_ATTRIBUTE;
				break;
			}
		}

		// print current cursor.
		g_consoleManager.screenBuffer[printOffset].char_ = CONSOLE_CURSOR_CHARACTER;
		g_consoleManager.screenBuffer[printOffset].attr = CONSOLE_CURSOR_ATTRIBUTE;
	}
	
	g_consoleManager.currentPrintOffset = printOffset;
}

void k_getCursor(int* x, int* y) {
	*x = g_consoleManager.currentPrintOffset % CONSOLE_WIDTH;
	*y = g_consoleManager.currentPrintOffset / CONSOLE_WIDTH;
}

void k_printf(const char* format, ...) {
	va_list ap;
	char buffer[1024];
	int nextPrintOffset;
	
	va_start(ap, format);
	k_vsprintf(buffer, format, ap);
	va_end(ap);
	
	nextPrintOffset = k_printStr(buffer);
	
	k_setCursor(nextPrintOffset % CONSOLE_WIDTH, nextPrintOffset / CONSOLE_WIDTH);
}

int k_printStr(const char* str) {
	Char* screen;
	int i, j;
	int len;
	int printOffset;
		
	screen = g_consoleManager.screenBuffer;

	printOffset = g_consoleManager.currentPrintOffset;
	
	len = k_strlen(str);
	
	for (i = 0; i < len; i++) {
		// process line feed.
		if (str[i] == '\n') {
			// move print offset to the multiple of 80 (the first position of the next line).
			printOffset += (CONSOLE_WIDTH - (printOffset % CONSOLE_WIDTH));
			
		// process tab.
		} else if (str[i] == '\t') {
			// move print offset to the multiple of 8 (the first position of the next tab).
			printOffset += (8 - (printOffset % 8));
			
		// process general character.
		} else {
			screen[printOffset].char_ = str[i];
			screen[printOffset].attr = CONSOLE_TEXT_ATTRIBUTE;
			printOffset++;
		}
		
		// process scroll, if print offset moves out of screen (80*25).
		if (printOffset >= (CONSOLE_WIDTH * CONSOLE_HEIGHT)) {
			// copy whole lines (from the second line to the last line) to a line above.
			k_memcpy(screen, screen + CONSOLE_WIDTH, (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof(Char));

			// put space to the last line.
			for (j = ((CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH); j < (CONSOLE_HEIGHT * CONSOLE_WIDTH); j++) {
				screen[j].char_ = ' ';
				screen[j].attr = CONSOLE_TEXT_ATTRIBUTE;
			}
			
			// move print offset to the first position of the last line.
			printOffset = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
		}
	}
	
	return printOffset;
}

void k_printStrXy(int x, int y, const char* str) {
	Char* screen;
	int i;
	
	screen = g_consoleManager.screenBuffer;	

	screen += (y * CONSOLE_WIDTH) + x;
	
	for (i = 0; str[i] != null; i++) {
		screen[i].char_ = str[i];
		screen[i].attr = CONSOLE_TEXT_ATTRIBUTE;
	}
}

void k_clearScreen(void) {
	Char* screen;
	int i;
	
	screen = g_consoleManager.screenBuffer;	

	for (i = 0; i < (CONSOLE_HEIGHT * CONSOLE_WIDTH); i++) {
		screen[i].char_ = ' ';
		screen[i].attr = CONSOLE_TEXT_ATTRIBUTE;
	}
	
	k_setCursor(0, 0);
}

byte k_getch(void) {
	Key key;
	
	// wait until key will be pressed.
	while (true) {
		if (k_isGraphicMode() == false) {
			// wait until kernel key queue will receive data.
			while (k_getKeyFromKeyQueue(&key) == false) {
				// switch task while waiting in order to reduce the processor load.
				k_schedule();
			}

		} else {
			// wait until console key queue will receive data.
			while (k_getKeyFromConsoleKeyQueue(&key) == false) {
				if (g_consoleManager.exit == true) {
					return 0xFF;
				}

				// switch task while waiting in order to reduce the processor load.
				k_schedule();
			}
		}
		
		// return ASCII code if key queue receives data.
		if (key.flags & KEY_FLAGS_DOWN) {
			return key.asciiCode;
		}
	}
}

ConsoleManager* k_getConsoleManager(void) {
	return &g_consoleManager;
}

bool k_putKeyToConsoleKeyQueue(const Key* key) {
	bool result;

	if (k_isQueueFull(&g_consoleManager.keyQueue) == true) {
		return false;
	}

	k_lock(&g_consoleManager.mutex);

	result = k_putQueue(&g_consoleManager.keyQueue, key);

	k_unlock(&g_consoleManager.mutex);

	return true;
}

bool k_getKeyFromConsoleKeyQueue(Key* key) {
	bool result;

	if (k_isQueueEmpty(&g_consoleManager.keyQueue) == true) {
		return false;
	}

	k_lock(&g_consoleManager.mutex);

	result = k_getQueue(&g_consoleManager.keyQueue, key);

	k_unlock(&g_consoleManager.mutex);

	return true;
}

void k_setShellExitFlag(bool exit) {
	g_consoleManager.exit = exit;
}