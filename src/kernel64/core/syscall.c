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
#include "app_manager.h"
#include "mp_config_table.h"
#include "multiprocessor.h"
#include "../utils/kid.h"
#include "../gui_tasks/alert.h"
#include "../gui_tasks/confirm.h"

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
		task = k_createTask(PARAM(0), (void*)PARAM(1), PARAM(2), PARAM(3), PARAM(4), (byte)PARAM(5));
		if (task == null) {
			return TASK_INVALIDID;
		}

		return task->link.id;

	case SYSCALL_GETCURRENTTASKID:
		task = k_getRunningTask(k_getApicId());
		if (task == null) {
			return TASK_INVALIDID;
		}

		return task->link.id;

	case SYSCALL_SCHEDULE:
		return (qword)k_schedule();

	case SYSCALL_CHANGETASKPRIORITY:
		return (qword)k_changeTaskPriority(PARAM(0), (byte)PARAM(1));

	case SYSCALL_CHANGETASKAFFINITY:
		return (qword)k_changeTaskAffinity(PARAM(0), (byte)PARAM(1));
		
	case SYSCALL_WAITTASK:
		return (qword)k_waitTask(PARAM(0));

	case SYSCALL_NOTIFYTASK:
		return (qword)k_notifyTask(PARAM(0));

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

	case SYSCALL_WAITGROUP:
		return (qword)k_waitGroup(PARAM(0), (void*)PARAM(1));

	case SYSCALL_NOTIFYONEINWAITGROUP:
		return (qword)k_notifyOneInWaitGroup(PARAM(0));

	case SYSCALL_NOTIFYALLINWAITGROUP:
		return (qword)k_notifyAllInWaitGroup(PARAM(0));

	case SYSCALL_JOINGROUP:
		return (qword)k_joinGroup((qword*)PARAM(0), (int)PARAM(1));

	case SYSCALL_NOTIFYONEINJOINGROUP:
		return (qword)k_notifyOneInJoinGroup(PARAM(0));

	case SYSCALL_NOTIFYALLINJOINGROUP:
		return (qword)k_notifyAllInJoinGroup(PARAM(0));

	case SYSCALL_CREATETHREAD:
		return k_createThread(PARAM(0), PARAM(1), (byte)PARAM(2), PARAM(3));

	/*** Syscall from sync.h ***/
	case SYSCALL_LOCK:
		k_lock((Mutex*)PARAM(0));
		return (qword)true;

	case SYSCALL_UNLOCK:
		k_unlock((Mutex*)PARAM(0));
		return (qword)true;

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

	/*** Syscall from mp_config_table.h ***/
	case SYSCALL_GETPROCESSORCOUNT:
		return (qword)k_getProcessorCount();

	/*** Syscall from multiprocessor.h ***/
	case SYSCALL_GETAPICID:
		return (qword)k_getApicId();

	/*** Syscall from window.h ***/
	case SYSCALL_GETBACKGROUNDWINDOWID:
		return k_getBackgroundWindowId();

	case SYSCALL_GETSCREENAREA:
		k_getScreenArea((Rect*)PARAM(0));
		return (qword)true;

	case SYSCALL_CREATEWINDOW:
		return k_createWindow((int)PARAM(0), (int)PARAM(1), (int)PARAM(2), (int)PARAM(3), (dword)PARAM(4), (char*)PARAM(5), (Color)PARAM(6), (Menu*)PARAM(7), (void*)PARAM(8), PARAM(9));

	case SYSCALL_DELETEWINDOW:
		return (qword)k_deleteWindow(PARAM(0));

	case SYSCALL_ISWINDOWSHOWN:
		return (qword)k_isWindowShown(PARAM(0));

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

	case SYSCALL_ISTOPWINDOW:
		return (qword)k_isTopWindow(PARAM(0));

	case SYSCALL_ISTOPWINDOWWITHCHILD:
		return (qword)k_isTopWindowWithChild(PARAM(0));

	case SYSCALL_MOVEWINDOWTOTOP:
		return (qword)k_moveWindowToTop(PARAM(0));

	case SYSCALL_ISPOINTINTITLEBAR:
		return (qword)k_isPointInTitleBar(PARAM(0), (int)PARAM(1), (int)PARAM(2));

	case SYSCALL_ISPOINTINCLOSEBUTTON:
		return (qword)k_isPointInCloseButton(PARAM(0), (int)PARAM(1), (int)PARAM(2));

	case SYSCALL_ISPOINTINRESIZEBUTTON:
		return (qword)k_isPointInResizeButton(PARAM(0), (int)PARAM(1), (int)PARAM(2));

	case SYSCALL_ISPOINTINTOPMENU:
		return (qword)k_isPointInTopMenu(PARAM(0), (int)PARAM(1), (int)PARAM(2));
		
	case SYSCALL_MOVEWINDOW:
		return (qword)k_moveWindow(PARAM(0), (int)PARAM(1), (int)PARAM(2));

	case SYSCALL_RESIZEWINDOW:
		return (qword)k_resizeWindow(PARAM(0), (int)PARAM(1), (int)PARAM(2), (int)PARAM(3), (int)PARAM(4));

	case SYSCALL_MOVECHILDWINDOWS:
		k_moveChildWindows(PARAM(0), (int)PARAM(1), (int)PARAM(2));
		return (qword)true;

	case SYSCALL_SHOWCHILDWINDOWS:
		k_showChildWindows(PARAM(0), (bool)PARAM(1), (dword)PARAM(2), (bool)PARAM(3));
		return (qword)true;

	case SYSCALL_DELETECHILDWINDOWS:
		k_deleteChildWindows(PARAM(0));
		return (qword)true;

	case SYSCALL_GETNTHCHILDWIDGET:
		return (qword)k_getNthChildWidget(PARAM(0), (int)PARAM(1));

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
		return (qword)k_drawWindowTitleBar(PARAM(0), (bool)PARAM(1));

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

	/*** Syscall from app_manager.h ***/
	case SYSCALL_EXECUTEAPP:
		return k_executeApp((char*)PARAM(0), (char*)PARAM(1), (byte)PARAM(2));

	/*** Syscall from widgets.h ***/
	case SYSCALL_CREATEMENU:
		return (qword)k_createMenu((Menu*)PARAM(0), (int)PARAM(1), (int)PARAM(2), (int)PARAM(3), (Color*)PARAM(4), PARAM(5), (Menu*)PARAM(6), (dword)PARAM(7));

	case SYSCALL_PROCESSMENUEVENT:
		return (qword)k_processMenuEvent((Menu*)PARAM(0));

	case SYSCALL_DRAWHOSLOGO:
		return (qword)k_drawHosLogo(PARAM(0), (int)PARAM(1), (int)PARAM(2), (int)PARAM(3), (int)PARAM(4), (Color)PARAM(5), (Color)PARAM(6));

	case SYSCALL_DRAWBUTTON:
		return (qword)k_drawButton(PARAM(0), (Rect*)PARAM(1), (Color)PARAM(2), (Color)PARAM(3), (char*)PARAM(4), (dword)PARAM(5));

	case SYSCALL_SETCLOCK:
		k_setClock((Clock*)PARAM(0), PARAM(1), (int)PARAM(2), (int)PARAM(3), (Color)PARAM(4), (Color)PARAM(5), (byte)PARAM(6), (bool)PARAM(7));
		return (qword)true;

	case SYSCALL_ADDCLOCK:
		k_addClock((Clock*)PARAM(0));
		return (qword)true;

	case SYSCALL_REMOVECLOCK:
		return (qword)k_removeClock(PARAM(0));

	/*** Syscall from kid.h ***/
	case SYSCALL_ALLOCKID:
		return k_allocKid();

	case SYSCALL_FREEKID:	
		k_freeKid(PARAM(0));
		return (qword)true;

	/*** Syscall from gui_task ***/
	case SYSCALL_ALERT:
		k_alert((char*)PARAM(0));
		return (qword)true;

	case SYSCALL_CONFIRM:
		k_confirm((ConfirmArg*)PARAM(0));
		return (qword)true;

	/*** Syscall - test ***/
	case SYSCALL_TEST:
		k_printf("syscall test...success\n");
		return (qword)true;

	default:
		k_printf("syscall error: invalid syscall number: %d\n", syscallNumber);		
		return (qword)false;
	}	
}
