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

void k_mainForAp(void);
void k_getRandomXy(int* x, int* y);
Color k_getRandomColor(void);
void k_drawWindowFrame(int x, int y, int width, int height, const char* title);
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
	
	// create idle task.
	k_createTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (qword)k_idleTask, k_getApicId());
	
	// check graphic mode flag
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
	
	k_printf("AP (%d) has been activated.\n", k_getApicId());
	
	// run idle task.
	k_idleTask();
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
	//char* testStr1 = "This is HansOS window prototype.";
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

void k_startGraphicModeTest(void) {
	VbeModeInfoBlock* vbeMode;
	Color* videoMem;
	Rect screen;
	int x1, y1, x2, y2;
	Color color1, color2;
	int i;
	char* strs[] = {"Points", "Lines", "Rectangles", "Circles", "HansOS"};
	
	// get video memory.
	vbeMode = k_getVbeModeInfoBlock();
	videoMem = (Color*)((qword)vbeMode->physicalBaseAddr & 0xFFFFFFFF);

	// set screen.
	screen.x1 = 0;
	screen.y1 = 0;
	screen.x2 = vbeMode->xResolution - 1;
	screen.y2 = vbeMode->yResolution - 1;

	/* draw point, line, rectangle, circle, text in the simple way. */
	
	// draw text of 'Points' with white text color and black background color at (0, 0).
	__k_drawText(videoMem, &screen, 0, 0, RGB(255, 255, 255), RGB(0, 0, 0), strs[0]);
	
	// draw 2 points with white color at (1, 20) and (2, 20).
	__k_drawPixel(videoMem, &screen, 1, 20, RGB(255, 255, 255));
	__k_drawPixel(videoMem, &screen, 2, 20, RGB(255, 255, 255));
	
	// draw text of 'Lines' with red text color and black background color at (0, 25).
	__k_drawText(videoMem, &screen, 0, 25, RGB(255, 0, 0), RGB(0, 0, 0), strs[1]);
	
	// draw 5 lines with red color from (20, 50) to (1000, 50), (1000, 100), (1000, 150), (1000, 200), (1000, 250).
	__k_drawLine(videoMem, &screen, 20, 50, 1000, 50, RGB(255, 0, 0));
	__k_drawLine(videoMem, &screen, 20, 50, 1000, 100, RGB(255, 0, 0));
	__k_drawLine(videoMem, &screen, 20, 50, 1000, 150, RGB(255, 0, 0));
	__k_drawLine(videoMem, &screen, 20, 50, 1000, 200, RGB(255, 0, 0));
	__k_drawLine(videoMem, &screen, 20, 50, 1000, 250, RGB(255, 0, 0));
	
	// draw text of 'Rectangles' with green text color and black background color at (0, 180).
	__k_drawText(videoMem, &screen, 0, 180, RGB(0, 255, 0), RGB(0, 0, 0), strs[2]);
	
	// draw 4 rectangles with green color and 50, 100, 150, 200 length starting from (20, 200).
	__k_drawRect(videoMem, &screen, 20, 200, 70, 250, RGB(0, 255, 0), false);
	__k_drawRect(videoMem, &screen, 120, 200, 220, 300, RGB(0, 255, 0), true);
	__k_drawRect(videoMem, &screen, 270, 200, 420, 350, RGB(0, 255, 0), false);
	__k_drawRect(videoMem, &screen, 470, 200, 670, 400, RGB(0, 255, 0), true);
	
	// draw text of 'Circles' with blue text color and black background color at (0, 550).
	__k_drawText(videoMem, &screen, 0, 550, RGB(0, 0, 255), RGB(0, 0, 0), strs[3]);
	
	// draw 4 circles with blue color and 25, 50, 75, 100 radius starting from (45, 600).
	__k_drawCircle(videoMem, &screen, 45, 600, 25, RGB(0, 0, 255), false);
	__k_drawCircle(videoMem, &screen, 170, 600, 50, RGB(0, 0, 255), true);
	__k_drawCircle(videoMem, &screen, 345, 600, 75, RGB(0, 0, 255), false);
	__k_drawCircle(videoMem, &screen, 570, 600, 100, RGB(0, 0, 255), true);
	
	// wait until any key will be input.
	k_getch();
	
	/* draw point, line, rectangle, circle, text in the random way. */
	
	// loop until 'q' key will be input.
	do {
		// draw background in order to clear screen.
		__k_drawRect(videoMem, &screen, 0, 0, 1024, 768, RGB(0, 0, 0), true);
		
		// draw a random point.
		k_getRandomXy(&x1, &y1);
		color1 = k_getRandomColor();
		__k_drawPixel(videoMem, &screen, x1, y1, color1);
		
		// draw a random line.
		k_getRandomXy(&x1, &y1);
		k_getRandomXy(&x2, &y2);
		color1 = k_getRandomColor();
		__k_drawLine(videoMem, &screen, x1, y1, x2, y2, color1);
		
		// draw a random rectangle.
		k_getRandomXy(&x1, &y1);
		k_getRandomXy(&x2, &y2);
		color1 = k_getRandomColor();
		__k_drawRect(videoMem, &screen, x1, y1, x2, y2, color1, k_random() % 2);
		
		// draw a random circle.
		k_getRandomXy(&x1, &y1);
		color1 = k_getRandomColor();
		__k_drawCircle(videoMem, &screen, x1, y1, ABS((k_random() % 50) + 1), color1, k_random() % 2);
		
		// draw a random text of 'HansOS'.
		k_getRandomXy(&x1, &y1);
		color1 = k_getRandomColor();
		color2 = k_getRandomColor();
		__k_drawText(videoMem, &screen, x1, y1, color1, color2, strs[4]);
		
	} while (k_getch() != 'q');
	
	/* draw window prototype. */
	while (true) {
		// draw background.
		__k_drawRect(videoMem, &screen, 0, 0, 1024, 768, RGB(232, 255, 232), true);
		
		// draw 3 window frames.
		for (i = 0; i < 3; i++) {
			k_getRandomXy(&x1, &y1);
			k_drawWindowFrame(x1, y1, 400, 200, "Window Prototype");
		}
		
		// wait until any key will be input.
		k_getch();
	}
}
