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

void k_mainForAp(void);

void k_main(void) {
	// compare BSP flag.
	if (BSP_FLAG == BSP_FLAG_AP) {
		k_mainForAp();
	}
	
	// set [0:AP] to BSP flag, because booting is finished by BSP.
	BSP_FLAG = BSP_FLAG_AP;
	
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
	
	// create idle task and start shell.
	k_createTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (qword)k_idleTask);
	k_startShell();
}

void k_mainForAp(void) {
	qword tickCount;
	
	// load GDT.
	k_loadGdt(GDTR_STARTADDRESS);
	
	// load TSS.
	k_loadTss(GDT_TSSSEGMENT + (k_getApicId() * sizeof(GdtEntry16)));
	
	// load IDT.
	k_loadIdt(IDTR_STARTADDRESS);
	
	/* support symmetric IO mode. */
	k_enableSoftwareLocalApic();
	k_setInterruptPriority(0);
	k_initLocalVectorTable();
	k_enableInterrupt();
	
	k_printf("AP (%d) has been activated.\n", k_getApicId());
	
	// print message once per 1 seconds.
	tickCount = k_getTickCount();
	while (true) {
		if (k_getTickCount() - tickCount > 1000) {
			tickCount = k_getTickCount();
			k_printf("AP (%d) is working.\n", k_getApicId());
		}
	}
}
