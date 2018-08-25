#include "types.h"
#include "keyboard.h"
#include "descriptors.h"
#include "asm_util.h"
#include "pic.h"
#include "console.h"
#include "shell.h"
#include "task.h"
#include "pit.h"
#include "util.h"
#include "dynamic_mem.h"
#include "hdd.h"
#include "file_system.h"
#include "serial_Port.h"
#include "multiprocessor.h"
#include "local_apic.h"
#include "vbe.h"
#include "2d_graphics.h"
#include "mp_config_table.h"
#include "mouse.h"
#include "interrupt_handlers.h"
#include "io_apic.h"

void k_mainForAp(void);
bool k_switchToMultiprocessorMode(void);
void k_getRandomXy(int* x, int* y);
Color k_getRandomColor(void);
void k_drawWindowFrame(int x, int y, int width, int height, const char* title);
void k_drawCursor(Color* outMem, Rect* area, int x, int y);
void k_startGraphicModeTest(void);

void k_main(void) {
	// compare BSP flag.
	if (BSPFLAG == BSPFLAG_AP) {
		k_mainForAp();
	}
	
	// set [0:AP] to BSP flag, because booting is finished by BSP.
	BSPFLAG = BSPFLAG_AP;
	
	// initialize console.
	k_initConsole(0, 12);
	
	// print the first message of kernel64 at line 12.
	k_printf("- switch to IA-32e mode......................pass\n");
	k_printf("- start IA-32e mode C kernel.................pass\n");
	k_printf("- initialize console.........................pass\n");
	
	// initialize GDT/TSS and load GDT.
	k_printf("- initialize GDT/TSS and load GDT............");
	k_initGdtAndTss();
	k_loadGdt(GDTR_STARTADDRESS);
	k_printf("pass\n");
	
	// load TSS.
	k_printf("- load TSS...................................");
	k_loadTss(GDT_TSSSEGMENT);
	k_printf("pass\n");
	
	// initialize and load IDT.
	k_printf("- initialize and load IDT....................");
	k_initIdt();
	k_loadIdt(IDTR_STARTADDRESS);
	k_printf("pass\n");
	
	// check total RAM size.
	k_printf("- check total RAM size.......................");
	k_checkTotalRamSize();
	k_printf("pass, %d MB\n", k_getTotalRamSize());
	
	// initialize scheduler.
	k_printf("- initialize scheduler.......................");
	k_initScheduler();
	k_printf("pass\n");
	
	// initialize dynamic memory.
	k_printf("- initialize dynamic memory..................");
	k_initDynamicMem();
	k_printf("pass\n");
	
	// initialize PIT.
    // Timer interrupt occurs once per 1 millisecond periodically.
	k_printf("- initialize PIT (once per 1 ms).............");
	k_initPit(MSTOCOUNT(1), true);
	k_printf("pass\n");
	
	// initialize keyboard.
	k_printf("- initialize keyboard........................");
	if (k_initKeyboard() == true) {
		k_printf("pass\n");
		k_changeKeyboardLed(false, false, false);
		
	} else {
		k_printf("fail\n");
		while (true);
	}
	
	// initialize mouse.
	k_printf("- initialize mouse...........................");
	if (k_initMouse() == true) {
		k_enableMouseInterrupt();
		k_printf("pass\n");

	} else {
		k_printf("fail\n");
		while (true);	
	}

	// initialize PIC and enable interrupt.
	k_printf("- initialize PIC and enable interrupt........");
	k_initPic();
	k_maskPicInterrupt(0);
	k_enableInterrupt();
	k_printf("pass\n");
	
	// initialize file system.
	k_printf("- initialize file system.....................");
	if (k_initFileSystem() == true) {
		k_printf("pass\n");
		
	} else {
		k_printf("fail\n");
		// Do not call infinite loop here,
		// because it could fail when hard disk has never been formatted.
	}
	
	// initialize serial port.
	k_printf("- initialize serial port.....................");
	k_initSerialPort();
	k_printf("pass\n");
	
	// switch to multiprocessor or multi-core processor mode.
	k_printf("- switch to multiprocessor mode..............");
	if (k_switchToMultiprocessorMode() == true) {
		k_printf("pass\n");

	} else {
		k_printf("fail\n");
	}

	// create idle task.
	k_createTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (qword)k_idleTask, k_getApicId());
	
	// check graphic mode flag.
	if (*(byte*)VBE_GRAPHICMODEFLAGADDRESS == 0x00) {
		k_startShell();
		
	} else {
		k_startGraphicModeTest();
	}
}

void k_mainForAp(void) {
	qword tickCount;
	
	// load GDT.
	k_loadGdt(GDTR_STARTADDRESS);
	
	// load TSS.
	k_loadTss(GDT_TSSSEGMENT + (k_getApicId() * sizeof(GdtEntry16)));
	
	// load IDT.
	k_loadIdt(IDTR_STARTADDRESS);
	
	// initialize scheduler.
	k_initScheduler();
	
	/* support symmetric IO mode. */
	k_enableSoftwareLocalApic();
	k_setInterruptPriority(0);
	k_initLocalVectorTable();
	k_enableInterrupt();
	
	//k_printf("AP (%d) has been activated.\n", k_getApicId());
	
	// run idle task.
	k_idleTask();
}

bool k_switchToMultiprocessorMode(void) {
	MpConfigManager* mpManager;
	bool interruptFlag;
	int i;

	/* start application processors */
	if (k_startupAp() == false) {
		return false;
	}

	/* start symmetric IO mode */
	// If it's PIC mode, disable PIC mode using IMCR Register.
	mpManager = k_getMpConfigManager();
	if (mpManager->picMode == true) {
		k_outPortByte(0x22, 0x70);
		k_outPortByte(0x23, 0x01);
	}

	k_maskPicInterrupt(0xFFFF);
	k_enableGlobalLocalApic();
	k_enableSoftwareLocalApic();
	interruptFlag = k_setInterruptFlag(false);
	k_setInterruptPriority(0);
	k_initLocalVectorTable();
	k_setSymmetricIoMode(true);
	k_initIoRedirectionTable();
	k_setInterruptFlag(interruptFlag);

	/* start interrupt load balancing */
	k_setInterruptLoadBalancing(true);

	/* start task load balancing */	
	for (i = 0; i < MAXPROCESSORCOUNT; i++) {
		k_setTaskLoadBalancing(i, true);
	}

	return true;
}

void k_getRandomXy(int* x, int* y) {
	int random;
	
	random = k_random();
	*x = ABS(random) % 1000;
	
	random = k_random();
	*y = ABS(random) % 700;
}

Color k_getRandomColor(void) {
	int random;
	int r, g, b;
	
	random = k_random();
	r = ABS(random) % 256;
	
	random = k_random();
	g = ABS(random) % 256;
	
	random = k_random();
	b = ABS(random) % 256;
	
	return RGB(r, g, b);
}

void k_drawWindowFrame(int x, int y, int width, int height, const char* title) {
	VbeModeInfoBlock* vbeMode;
	Color* videoMem;
	Rect screen;	
	char* testStr1 = "This is HansOS window prototype.";
	char* testStr2 = "Coming soon!";

	// get video memory.
	vbeMode = k_getVbeModeInfoBlock();
	videoMem = (Color*)((qword)vbeMode->physicalBaseAddr & 0xFFFFFFFF);

	// set screen.
	screen.x1 = 0;
	screen.y1 = 0;
	screen.x2 = vbeMode->xResolution - 1;
	screen.y2 = vbeMode->yResolution - 1;

	// draw edges of window frame (2 pixes-thick).
	__k_drawRect(videoMem, &screen, x, y, x + width, y + height, RGB(109, 218, 22), false);
	__k_drawRect(videoMem, &screen, x + 1, y + 1, x + width - 1, y + height - 1, RGB(109, 218, 22), false);
	
	// draw a title bar.
	__k_drawRect(videoMem, &screen, x, y + 3, x + width - 1, y + 21, RGB(79, 204, 11), true);
	
	// draw a title.
	__k_drawText(videoMem, &screen, x + 6, y + 3, RGB( 255, 255, 255 ), RGB( 79, 204, 11 ), title);
	
	// draw upper lines of title bar (2 pixels-thick) in order to make it 3-dimensional.
	__k_drawLine(videoMem, &screen, x + 1, y + 1, x + width - 1, y + 1, RGB(183, 249, 171));
	__k_drawLine(videoMem, &screen, x + 1, y + 2, x + width - 1, y + 2, RGB(150, 210, 140));
	
	__k_drawLine(videoMem, &screen, x + 1, y + 2, x + 1, y + 20, RGB(183, 249, 171));
	__k_drawLine(videoMem, &screen, x + 2, y + 2, x + 2, y + 20, RGB(150, 210, 140));
	
	// draw lower lines of title bar.
	__k_drawLine(videoMem, &screen, x + 2, y + 19, x + width - 2, y + 19, RGB(46, 59, 30));
	__k_drawLine(videoMem, &screen, x + 2, y + 20, x + width - 2, y + 20, RGB(46, 59, 30));
	
	// draw a close button in top-right.
	__k_drawRect(videoMem, &screen, x + width - 2 - 18, y + 1, x + width - 2, y + 19, RGB(255, 255, 255), true);
	
	// draw edges of close button (2 pixels-thick) in order to make it 3-dimensional.
	__k_drawRect(videoMem, &screen, x + width - 2, y + 1, x + width - 2, y + 19 - 1, RGB(86, 86, 86), true);
	__k_drawRect(videoMem, &screen, x + width - 2 - 1, y + 1, x + width - 2 - 1, y + 19 - 1, RGB(86, 86, 86), true);
	__k_drawRect(videoMem, &screen, x + width - 2 - 18 + 1, y + 19, x + width - 2, y + 19, RGB(86, 86, 86), true);
	__k_drawRect(videoMem, &screen, x + width - 2 - 18 + 1, y + 19 - 1, x + width - 2, y + 19 - 1, RGB(86, 86, 86), true);
	
	__k_drawLine(videoMem, &screen, x + width - 2 - 18, y + 1, x + width - 2 - 1, y + 1, RGB(229, 229, 229));
	__k_drawLine(videoMem, &screen, x + width - 2 - 18, y + 1 + 1, x + width - 2 - 2, y + 1 + 1, RGB(229, 229, 229));
	__k_drawLine(videoMem, &screen, x + width - 2 - 18, y + 1, x + width - 2 - 18, y + 19, RGB(229, 229, 229));
	__k_drawLine(videoMem, &screen, x + width - 2 - 18 + 1, y + 1, x + width - 2 - 18 + 1, y + 19 - 1, RGB(229, 229, 229));
	
	// draw 'X' mark on close button (3 pixels-thick).
	__k_drawLine(videoMem, &screen, x + width - 2 - 18 + 4, y + 1 + 4, x + width - 2 - 4, y + 19 - 4, RGB(71, 199, 21));
	__k_drawLine(videoMem, &screen, x + width - 2 - 18 + 5, y + 1 + 4, x + width - 2 - 4, y + 19 - 5, RGB(71, 199, 21));
	__k_drawLine(videoMem, &screen, x + width - 2 - 18 + 4, y + 1 + 5, x + width - 2 - 5, y + 19 - 4, RGB(71, 199, 21));
	
	__k_drawLine(videoMem, &screen, x + width - 2 - 18 + 4, y + 19 - 4, x + width - 2 - 4, y + 1 + 4, RGB(71, 199, 21));
	__k_drawLine(videoMem, &screen, x + width - 2 - 18 + 5, y + 19 - 4, x + width - 2 - 4, y + 1 + 5, RGB(71, 199, 21));
	__k_drawLine(videoMem, &screen, x + width - 2 - 18 + 4, y + 19 - 5, x + width - 2 - 5, y + 1 + 4, RGB(71, 199, 21));
	
	// draw the inner part of window frame.
	__k_drawRect(videoMem, &screen, x + 2, y + 21, x + width - 2, y + height - 2, RGB(255, 255, 255), true);
	
	// draw test strings.
	__k_drawText(videoMem, &screen, x + 10, y + 30, RGB( 0, 0, 0 ), RGB( 255, 255, 255 ), testStr1);
	__k_drawText(videoMem, &screen, x + 10, y + 50, RGB( 0, 0, 0 ), RGB( 255, 255, 255 ), testStr2);
}

#define MOUSE_CURSOR_WIDTH  20
#define MOUSE_CURSOR_HEIGHT 20

// mouse cursor bitmap (400 bytes)
static byte g_mouseCursorBitmap[MOUSE_CURSOR_WIDTH * MOUSE_CURSOR_HEIGHT] = {
	1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0,
	0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1,
	0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1,
	0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1, 0, 0,
	0, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 1, 1, 0, 0, 0, 0,
	0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 2, 3, 3, 2, 1, 1, 2, 3, 2, 2, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 1, 2, 3, 2, 2, 1, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 0,
	0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0,
	0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0
};

// mouse cursor bitmap color
#define MOUSE_CURSOR_COLOR_OUTERLINE RGB(0, 0, 0)       // black color, 1 in bitmap
#define MOUSE_CURSOR_COLOR_OUTER     RGB(79, 204, 11)   // dark green color, 2 in bitmap
#define MOUSE_CURSOR_COLOR_INNER     RGB(232, 255, 232) // bright color, 3 in bitmap

void k_drawCursor(Color* outMem, Rect* area, int x, int y) {
	int i, j;
	byte* currentPos;

	currentPos = g_mouseCursorBitmap;

	for (i = 0; i < MOUSE_CURSOR_HEIGHT; i++) {
		for (j = 0; j < MOUSE_CURSOR_WIDTH; j++) {
			switch (*currentPos) {
			case 0:
				break;

			case 1:
				__k_drawPixel(outMem, area, x + j, y + i, MOUSE_CURSOR_COLOR_OUTERLINE);
				break;

			case 2:
				__k_drawPixel(outMem, area, x + j, y + i, MOUSE_CURSOR_COLOR_OUTER);
				break;

			case 3:
				__k_drawPixel(outMem, area, x + j, y + i, MOUSE_CURSOR_COLOR_INNER);
				break;
			}

			currentPos++;
		}
	}
}

void k_startGraphicModeTest(void) {
	VbeModeInfoBlock* vbeMode;
	Color* videoMem;
	Rect screen;
	int x, y; // x, y to draw cursor
	byte buttonStatus;
	int relativeX, relativeY;

	// get video memory.
	vbeMode = k_getVbeModeInfoBlock();
	videoMem = (Color*)((qword)vbeMode->physicalBaseAddr & 0xFFFFFFFF);

	// set screen.
	screen.x1 = 0;
	screen.y1 = 0;
	screen.x2 = vbeMode->xResolution - 1;
	screen.y2 = vbeMode->yResolution - 1;

	// set initial mouse cursor position to the center of screen.
	x = vbeMode->xResolution / 2;
	y = vbeMode->yResolution / 2;

	/* print mouse cursor and process mouse movement. */

	// draw background.
	__k_drawRect(videoMem, &screen, screen.x1, screen.y1, screen.x2, screen.y2, RGB(232, 255, 232), true);

	// draw initial mouse cursor.
	k_drawCursor(videoMem, &screen, x, y);

	while (1) {
		// get mouse data from mouse queue.
		if (k_getMouseDataFromMouseQueue(&buttonStatus, &relativeX, &relativeY) == false) {
			k_sleep(0);
			continue;
		}

		// clear previous mouse cursor.
		__k_drawRect(videoMem, &screen, x, y, x + MOUSE_CURSOR_WIDTH, y + MOUSE_CURSOR_HEIGHT, RGB(232, 255, 232), true);

		// move mouse cursor.
		x += relativeX;
		y += relativeY;

		// limit x inside screen.
		if (x < screen.x1) {
			x = screen.x1;

		} else if (x > screen.x2) {
			x = screen.x2;
		}

		// limit y inside screen.
		if (y < screen.y1) {
			y = screen.y1;

		} else if (y > screen.y2) {
			y = screen.y2;
		}

		// process button click.
		if (buttonStatus & MOUSE_LBUTTONDOWN) {
			k_drawWindowFrame(x - 10, y - 10, 400, 200, "Window Prototype");

		} else if (buttonStatus & MOUSE_RBUTTONDOWN) {
			// clear screen.
			__k_drawRect(videoMem, &screen, screen.x1, screen.y1, screen.x2, screen.y2, RGB(232, 255, 232), true);	
		}

		// draw current mouse cursor.
		k_drawCursor(videoMem, &screen, x, y);
	}
}