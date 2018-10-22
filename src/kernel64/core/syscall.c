#include "syscall.h"
#include "descriptors.h"
#include "asm_util.h"
#include "console.h"
#include "rtc.h"
#include "task.h"
#include "dynamic_mem.h"
#include "hdd.h"
#include "file_system.h"
#include "serial_port.h"
#include "window.h"
#include "../utils/jpeg.h"
#include "../utils/util.h"

/**
  < SYSCALL/SYSRET Initialization Registers >
  1. IA32_STAR MSR (address 0xC0000081, size 64 bits)
     - bit  0 ~ 31 (reserved)
     - bit 32 ~ 47 (offset) : SYSCALL : offset      -> CS
                                        offset + 8  -> SS
     - bit 48 ~ 63 (offset) : SYSRET  : offset      -> CS (If REX.W == 0, protected mode)
                                        offset + 8  -> SS (If REX.W == 0, protected mode)
                                        offset + 16 -> CS (If REX.W == 1, IA-32e mode)
                                        offset + 8  -> SS (If REX.W == 1, IA-32e mode)
     [REF] To set W (bit 3) of REX prefix to 1, use 'o64' keyword in NASM complier. 

  2. IA32_LSTAR MSR (address 0xC0000082, size 64 bits)
     - bit 0 ~ 63 (address) : SYSCALL : RIP (user) -> RCX, address -> RIP (kernel)
                              SYSRET  : RCX -> RIP (user)
  
  3. IA32_FMASK MSR (address 0xC0000084, size 64 bits)
     - bit  0 ~ 31 (mask) : SYSCALL : RFLAGS (user) -> R11, mask -> RFLAGS (kernel) & ~mask
                            SYSRET  : R11 -> RFLAGS (user)
     - bit 32 ~ 63 (reserved)
*/

void k_initSyscall(void) {
	// set IA32_STAR MSR.
	k_writeMsr(0xC0000081, ((GDT_OFFSET_KERNELDATASEGMENT | SELECTOR_RPL3) << 16) | GDT_OFFSET_KERNELCODESEGMENT, 0);

	// set IA32_LSTAR MSR.
	k_writeMsr(0xC0000082, 0, (qword)k_syscallEntryPoint);

	// set IA32_FMASK MSR (no mask).
	k_writeMsr(0xC0000084, 0, 0x00);
}

qword k_processSyscall(qword syscallNumber, const ParamTable* paramTable) {
	Task* task;

	switch (syscallNumber) {
	/*** Syscall from console.h ***/
	case SYSCALL_SETCURSOR:
		k_setCursor((int)PARAM(0), (int)PARAM(1));
		return (qword)true;

	case SYSCALL_GETCURSOR:
		k_getCursor((int*)PARAM(0), (int*)PARAM(1));
		return (qword)true;

	case SYSCALL_PRINTSTR:
		return (qword)k_printStr((char*)PARAM(0));

	case SYSCALL_CLEARSCREEN:
		k_clearScreen();
		return (qword)true;

	case SYSCALL_GETCH:
		return (qword)k_getch();

	/*** Syscall from rtc.h ***/
	case SYSCALL_READRTCTIME:
		k_readRtcTime((byte*)PARAM(0), (byte*)PARAM(1), (byte*)PARAM(2));
		return (qword)true;

	case SYSCALL_READRTCDATE:
		k_readRtcDate((word*)PARAM(0), (byte*)PARAM(1), (byte*)PARAM(2), (byte*)PARAM(3));
		return (qword)true;

	/*** Syscall from task.h ***/
	case SYSCALL_CREATETASK:
		task = k_createTask(PARAM(0), (void*)PARAM(1), PARAM(2), PARAM(3), (byte)PARAM(4));
		if (task == null) {
			return TASK_INVALIDID;
		}

		return task->link.id;

	case SYSCALL_SCHEDULE:
		return (qword)k_schedule();

	case SYSCALL_CHANGETASKPRIORITY:
		return (qword)k_changeTaskPriority(PARAM(0), (byte)PARAM(1));

	case SYSCALL_ENDTASK:
		return (qword)k_endTask(PARAM(0));

	case SYSCALL_EXIT:
		k_exitTask();
		return (qword)true;

	case SYSCALL_GETTASKCOUNT:
		return (qword)k_getTaskCount((byte)PARAM(0));

	case SYSCALL_EXISTTASK:
		return (qword)k_existTask(PARAM(0));

	case SYSCALL_GETPROCESSORLOAD:
		return k_getProcessorLoad((byte)PARAM(0));

	case SYSCALL_CHANGETASKAFFINITY:
		return (qword)k_changeTaskAffinity(PARAM(0), (byte)PARAM(1));

	/*** Syscall from dynamic_mem.h ***/
	case SYSCALL_MALLOC:
		return (qword)k_allocMem(PARAM(0));

	case SYSCALL_FREE:
		return (qword)k_freeMem((void*)PARAM(0));

	/*** Syscall from hdd.h ***/
	case SYSCALL_READHDDSECTOR:
		return (qword)k_readHddSector((bool)PARAM(0), (bool)PARAM(1), (dword)PARAM(2), (int)PARAM(3), (char*)PARAM(4));

	case SYSCALL_WRITEHDDSECTOR:
		return (qword)k_writeHddSector((bool)PARAM(0), (bool)PARAM(1), (dword)PARAM(2), (int)PARAM(3), (char*)PARAM(4));

	/*** Syscall from file_system.h ***/
	case SYSCALL_FOPEN:
		return (qword)fopen((char*)PARAM(0), (char*)PARAM(1));

	case SYSCALL_FREAD:
		return (qword)fread((void*)PARAM(0), (dword)PARAM(1), (dword)PARAM(2), (File*)PARAM(3));

	case SYSCALL_FWRITE:
		return (qword)fwrite((void*)PARAM(0), (dword)PARAM(1), (dword)PARAM(2), (File*)PARAM(3));

	case SYSCALL_FSEEK:
		return (qword)fseek((File*)PARAM(0), (int)PARAM(1), (int)PARAM(2));

	case SYSCALL_FCLOSE:
		return (qword)fclose((File*)PARAM(0));

	case SYSCALL_REMOVE:
		return (qword)remove((char*)PARAM(0));

	case SYSCALL_OPENDIR:
		return (qword)opendir((char*)PARAM(0));

	case SYSCALL_READDIR:
		return (qword)readdir((Dir*)PARAM(0));

	case SYSCALL_REWINDDIR:
		rewinddir((Dir*)PARAM(0));
		return (qword)true;

	case SYSCALL_CLOSEDIR:
		return (qword)closedir((Dir*)PARAM(0));

	case SYSCALL_ISFOPEN:
		return (qword)isfopen((dirent*)PARAM(0));

	/*** Syscall from serial_port.h ***/
	case SYSCALL_SENDSERIALDATA:
		k_sendSerialData((byte*)PARAM(0), (int)PARAM(1));
		return (qword)true;

	case SYSCALL_RECVSERIALDATA:
		return (qword)k_recvSerialData((byte*)PARAM(0), (int)PARAM(1));

	case SYSCALL_CLEARSERIALFIFO:
		k_clearSerialFifo();
		return (qword)true;

	/*** Syscall from window.h ***/
	case SYSCALL_GETBACKGROUNDWINDOWID:
		return k_getBackgroundWindowId();

	case SYSCALL_GETSCREENAREA:
		k_getScreenArea((Rect*)PARAM(0));
		return (qword)true;

	case SYSCALL_CREATEWINDOW:
		return k_createWindow((int)PARAM(0), (int)PARAM(1), (int)PARAM(2), (int)PARAM(3), (dword)PARAM(4), (char*)PARAM(5));

	case SYSCALL_DELETEWINDOW:
		return (qword)k_deleteWindow(PARAM(0));

	case SYSCALL_SHOWWINDOW:
		return (qword)k_showWindow(PARAM(0), (bool)PARAM(1));

	case SYSCALL_FINDWINDOWBYPOINT:
		return k_findWindowByPoint((int)PARAM(0), (int)PARAM(1));

	case SYSCALL_FINDWINDOWBYTITLE:
		return k_findWindowByTitle((char*)PARAM(0));

	case SYSCALL_EXISTWINDOW:
		return (qword)k_existWindow(PARAM(0));

	case SYSCALL_GETTOPWINDOWID:
		return k_getTopWindowId();

	case SYSCALL_MOVEWINDOWTOTOP:
		return (qword)k_moveWindowToTop(PARAM(0));

	case SYSCALL_ISPOINTINTITLEBAR:
		return (qword)k_isPointInTitleBar(PARAM(0), (int)PARAM(1), (int)PARAM(2));

	case SYSCALL_ISPOINTINCLOSEBUTTON:
		return (qword)k_isPointInCloseButton(PARAM(0), (int)PARAM(1), (int)PARAM(2));

	case SYSCALL_MOVEWINDOW:
		return (qword)k_moveWindow(PARAM(0), (int)PARAM(1), (int)PARAM(2));

	case SYSCALL_RESIZEWINDOW:
		return (qword)k_resizeWindow(PARAM(0), (int)PARAM(1), (int)PARAM(2), (int)PARAM(3), (int)PARAM(4));

	case SYSCALL_GETWINDOWAREA:
		return (qword)k_getWindowArea(PARAM(0), (Rect*)PARAM(1));

	case SYSCALL_SENDEVENTTOWINDOW:
		return (qword)k_sendEventToWindow((Event*)PARAM(0), PARAM(1));

	case SYSCALL_RECVEVENTFROMWINDOW:
		return (qword)k_recvEventFromWindow((Event*)PARAM(0), PARAM(1));

	case SYSCALL_UPDATESCREENBYID:
		return (qword)k_updateScreenById(PARAM(0));

	case SYSCALL_UPDATESCREENBYWINDOWAREA:
		return (qword)k_updateScreenByWindowArea(PARAM(0), (Rect*)PARAM(1));

	case SYSCALL_UPDATESCREENBYSCREENAREA:
		return (qword)k_updateScreenByScreenArea((Rect*)PARAM(0));

	case SYSCALL_DRAWWINDOWBACKGROUND:
		return (qword)k_drawWindowBackground(PARAM(0));

	case SYSCALL_DRAWWINDOWFRAME:
		return (qword)k_drawWindowFrame(PARAM(0));

	case SYSCALL_DRAWWINDOWTITLEBAR:
		return (qword)k_drawWindowTitleBar(PARAM(0), (char*)PARAM(1), (bool)PARAM(2));

	case SYSCALL_DRAWBUTTON:
		return (qword)k_drawButton(PARAM(0), (Rect*)PARAM(1), (Color)PARAM(2), (char*)PARAM(3), (Color)PARAM(4));

	case SYSCALL_DRAWPIXEL:
		return (qword)k_drawPixel(PARAM(0), (int)PARAM(1), (int)PARAM(2), (Color)PARAM(3));

	case SYSCALL_DRAWLINE:
		return (qword)k_drawLine(PARAM(0), (int)PARAM(1), (int)PARAM(2), (int)PARAM(3), (int)PARAM(4), (Color)PARAM(5));

	case SYSCALL_DRAWRECT:
		return (qword)k_drawRect(PARAM(0), (int)PARAM(1), (int)PARAM(2), (int)PARAM(3), (int)PARAM(4), (Color)PARAM(5), (bool)PARAM(6));

	case SYSCALL_DRAWCIRCLE:
		return (qword)k_drawCircle(PARAM(0), (int)PARAM(1), (int)PARAM(2), (int)PARAM(3), (Color)PARAM(4), (bool)PARAM(5));

	case SYSCALL_DRAWTEXT:
		return (qword)k_drawText(PARAM(0), (int)PARAM(1), (int)PARAM(2), (Color)PARAM(3), (Color)PARAM(4), (char*)PARAM(5), (int)PARAM(6));

	case SYSCALL_BITBLT:
		return (qword)k_bitblt(PARAM(0), (int)PARAM(1), (int)PARAM(2), (Color*)PARAM(3), (int)PARAM(4), (int)PARAM(5));

	case SYSCALL_MOVEMOUSECURSOR:
		k_moveMouseCursor((int)PARAM(0), (int)PARAM(1));
		return (qword)true;

	case SYSCALL_GETMOUSECURSORPOS:
		k_getMouseCursorPos((int*)PARAM(0), (int*)PARAM(1));
		return (qword)true;

	/*** Syscall from jpeg.h ***/
	case SYSCALL_INITJPEG:
		return (qword)k_initJpeg((Jpeg*)PARAM(0), (byte*)PARAM(1), (dword)PARAM(2));

	case SYSCALL_DECODEJPEG:
		return (qword)k_decodeJpeg((Jpeg*)PARAM(0), (Color*)PARAM(1));

	/*** Syscall from util.h ***/
	case SYSCALL_GETTOTALRAMSIZE:
		return k_getTotalRamSize();

	case SYSCALL_GETTICKCOUNT:
		return k_getTickCount();

	case SYSCALL_SLEEP:
		k_sleep(PARAM(0));
		return (qword)true;

	case SYSCALL_ISGRAPHICMODE:
		return (qword)k_isGraphicMode();

	/*** Syscall - test ***/
	case SYSCALL_TEST:
		k_printf("syscall test...success\n");
		return true;

	default:
		k_printf("syscall error: invalid syscall number: %d\n", syscallNumber);		
		return false;
	}	
}
