#include <stdarg.h>
#include "console.h"
#include "keyboard.h"
#include "util.h"
#include "task.h"
#include "asm_util.h"

static ConsoleManager g_consoleManager = {0, };

void k_initConsole(int x, int y) {
	k_memset(&g_consoleManager, 0, sizeof(g_consoleManager));
	k_setCursor(x, y);
}

void k_setCursor(int x, int y) {
	int linearValue;
	
	linearValue = y * CONSOLE_WIDTH + x;
	
	// send High Cursor Position Register Select Command (0x0E) to CRTC Control Address Register (0x3D4).
	k_outPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR);
	
	// send high byte of cursor to CRTC Control Data Register (0x3D5).
	k_outPortByte(VGA_PORT_DATA, linearValue >> 8);
	
	// send Low Cursor Position Register Select Command (0x0F) to CRTC Control Address Register (0x3D4).
	k_outPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);
	
	// send low byte of cursor to CRTC Control Data Register (0x3D5).
	k_outPortByte(VGA_PORT_DATA, linearValue & 0xFF);
	
	g_consoleManager.currentPrintOffset = linearValue;
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
	
	nextPrintOffset = k_consolePrintStr(buffer);
	
	k_setCursor(nextPrintOffset % CONSOLE_WIDTH, nextPrintOffset / CONSOLE_WIDTH);
}

int k_consolePrintStr(const char* buffer) {
	Char* screen = (Char*)CONSOLE_VIDEOMEMORYADDRESS;
	int i, j;
	int len;
	int printOffset;
	
	printOffset = g_consoleManager.currentPrintOffset;
	
	len = k_strlen(buffer);
	
	for (i = 0; i < len; i++) {
		// process line feed.
		if (buffer[i] == '\n') {
			// move print offset to the multiple of 80 (the first position of the next line).
			printOffset += (CONSOLE_WIDTH - (printOffset % CONSOLE_WIDTH));
			
		// process tab.
		} else if (buffer[i] == '\t') {
			// move print offset to the multiple of 8 (the first position of the next tab).
			printOffset += (8 - (printOffset % 8));
			
		// process general character.
		} else {
			screen[printOffset].char_ = buffer[i];
			screen[printOffset].attr = CONSOLE_DEFAULTTEXTCOLOR;
			printOffset++;
		}
		
		// process scroll, if print offset moves out of screen (80*25).
		if (printOffset >= (CONSOLE_WIDTH * CONSOLE_HEIGHT)) {
			
			// copy whole lines (from the second line to the last line) to a line above.
			k_memcpy((void*)CONSOLE_VIDEOMEMORYADDRESS
				    ,(void*)(CONSOLE_VIDEOMEMORYADDRESS + (CONSOLE_WIDTH * sizeof(Char)))
				    ,(CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof(Char));
			
			// put space to the last line.
			for (j = ((CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH); j < (CONSOLE_HEIGHT * CONSOLE_WIDTH); j++) {
				screen[j].char_ = ' ';
				screen[j].attr = CONSOLE_DEFAULTTEXTCOLOR;
			}
			
			// move print offset to the first position of the last line.
			printOffset = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
		}
	}
	
	return printOffset;
}

void k_clearScreen(void) {
	Char* screen = (Char*)CONSOLE_VIDEOMEMORYADDRESS;
	int i;
	
	for (i = 0; i < (CONSOLE_HEIGHT * CONSOLE_WIDTH); i++) {
		screen[i].char_ = ' ';
		screen[i].attr = CONSOLE_DEFAULTTEXTCOLOR;
	}
	
	k_setCursor(0, 0);
}

byte k_getch(void) {
	Key key;
	
	// wait until a key is pressed.
	while (true) {
		// wait until key queue receives data.
		while (k_getKeyFromKeyQueue(&key) == false) {
			// switch a task while waiting in order to reduce the processor usage.
			k_schedule();
		}
		
		// return ASCII code, if key queue receives data.
		if (key.flags & KEY_FLAGS_DOWN) {
			return key.asciiCode;
		}
	}
}

void k_printStrXy(int x, int y, const char* str) {
	Char* screen = (Char*)CONSOLE_VIDEOMEMORYADDRESS;
	int i;
	
	screen += (y * CONSOLE_WIDTH) + x;
	
	for (i = 0; str[i] != null; i++) {
		screen[i].char_ = str[i];
		screen[i].attr = CONSOLE_DEFAULTTEXTCOLOR;
	}
}
