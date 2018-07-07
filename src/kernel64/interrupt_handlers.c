#include "interrupt_handlers.h"
#include "pic.h"
#include "keyboard.h"
#include "console.h"
#include "util.h"
#include "task.h"
#include "descriptors.h"
#include "asm_util.h"
#include "hdd.h"

void k_commonExceptionHandler(int vectorNumber, qword errorCode) {
	char buffer[3] = {0, };

	// set vector number (2 digits integer).
	buffer[0] = '0' + vectorNumber / 10;
	buffer[1] = '0' + vectorNumber % 10;

	k_printStrXy(0, 0, "==================================================");
	k_printStrXy(0, 1, "               Exception Occur~!!                 ");
	k_printStrXy(0, 2, "                  Vector: [  ]                    ");
	k_printStrXy(27, 2, buffer); // print in the front of "Vector:" string.
	k_printStrXy(0, 3, "==================================================");

	while (true);
}

void k_commonInterruptHandler(int vectorNumber) {
	char buffer[] = "[INT:  , ]";
	static int commonInterruptCount = 0;

	//====================================================================================================
	/* Print Interrupt Message */
	// set vector number (2 digits integer).
	buffer[5] = '0' + vectorNumber / 10;
	buffer[6] = '0' + vectorNumber % 10;

	// set interrupt-occurred count (1 digit integer).
	commonInterruptCount = (commonInterruptCount + 1) % 10;
	buffer[8] = '0' + commonInterruptCount;

	// print in the last position of the first line.
	k_printStrXy(70, 0, buffer);
	//====================================================================================================

	k_sendEoiToPic(vectorNumber - PIC_IRQSTARTVECTOR);
}

void k_keyboardHandler(int vectorNumber) {
	char buffer[] = "[INT:  , ]";
	static int keyboardInterruptCount = 0;
	byte scanCode;

	//====================================================================================================
	/* Print Interrupt Message */
	// set vector number (2 digits integer).
	buffer[5] = '0' + vectorNumber / 10;
	buffer[6] = '0' + vectorNumber % 10;

	// set interrupt-occurred count (1 digit integer).
	keyboardInterruptCount = (keyboardInterruptCount + 1) % 10;
	buffer[8] = '0' + keyboardInterruptCount;

	// print in the first position of the first line.
	k_printStrXy(0, 0, buffer);
	//====================================================================================================

	if (k_isOutputBufferFull() == true) {
		scanCode = k_getKeyboardScanCode();
		k_convertScanCodeAndPutQueue(scanCode);
	}

	k_sendEoiToPic(vectorNumber - PIC_IRQSTARTVECTOR);
}

void k_timerHandler(int vectorNumber) {
	char buffer[] = "[INT:  , ]";
	static int timerInterruptCount = 0;

	//====================================================================================================
	/* Print Interrupt Message */
	// set vector number (2 digits integer).
	buffer[5] = '0' + vectorNumber / 10;
	buffer[6] = '0' + vectorNumber % 10;

	// set interrupt-occurred count (1 digit integer).
	timerInterruptCount = (timerInterruptCount + 1) % 10;
	buffer[8] = '0' + timerInterruptCount;

	// print in the last position of the first line.
	k_printStrXy(70, 0, buffer);
	//====================================================================================================

	k_sendEoiToPic(vectorNumber - PIC_IRQSTARTVECTOR);

	g_tickCount++;

	k_decreaseProcessorTime();

	if (k_isProcessorTimeExpired() == true) {
		k_scheduleInInterrupt();
	}
}

void k_deviceNotAvailableHandler(int vectorNumber) {
	Tcb* fpuTask;        // last FPU-used task
	Tcb* currentTask;    // current task
	qword lastFpuTaskId; // last FPU-used task ID

	//====================================================================================================
	/* Print FPU Exception Message */
	char buffer[] = "[EXC:  , ]";
	static int fpuExceptionCount = 0;

	// set vector number (2 digits number).
	buffer[5] = '0' + vectorNumber / 10;
	buffer[6] = '0' + vectorNumber % 10;

	// set interrupt-occurred count (1 digit integer).
	fpuExceptionCount = (fpuExceptionCount + 1) % 10;
	buffer[8] = '0' + fpuExceptionCount;

	// print in the first position of the first line.
	k_printStrXy(0, 0, buffer);
	//====================================================================================================

	// set CR0.TS=0
	k_clearTs();

	lastFpuTaskId = k_getLastFpuUsedTaskId();
	currentTask = k_getRunningTask();

	// If last FPU-used task is current task, it's not required to save and restore FPU context, and use current FPU context as is.
	if (lastFpuTaskId == currentTask->link.id) {
		return;

	// If last FPU-used task exists, save current FPU context to memory.
	} else if (lastFpuTaskId != TASK_INVALIDID) {
		fpuTask = k_getTcbInTcbPool(GETTCBOFFSET(lastFpuTaskId));
		if ((fpuTask != null) && (fpuTask->link.id == lastFpuTaskId)) {
			k_saveFpuContext(fpuTask->fpuContext);
		}
	}

	// If current task has never used FPU, initialize FPU.
	if (currentTask->fpuUsed == false) {
		k_initFpu();
		currentTask->fpuUsed = true;

	// If current task has used FPU, restore FPU context of current task from memory.
	} else {
		k_loadFpuContext(currentTask->fpuContext);
	}

	// set last FPU-used task to current task.
	k_setLastFpuUsedTaskId(currentTask->link.id);
}

void k_hddHandler(int vectorNumber) {
	char buffer[] = "[INT:  , ]";
	static int hddInterruptCount = 0;

	//====================================================================================================
	/* Print Interrupt Message */
	// set vector number (2 digits integer).
	buffer[5] = '0' + vectorNumber / 10;
	buffer[6] = '0' + vectorNumber % 10;

	// set interrupt-occurred count (1 digit integer).
	hddInterruptCount = (hddInterruptCount + 1) % 10;
	buffer[8] = '0' + hddInterruptCount;

	// print in the second position of the first line.
	k_printStrXy(10, 0, buffer);
	//====================================================================================================

	// process interrupt vector of first PATA port (IRQ 14).
	if ((vectorNumber - PIC_IRQSTARTVECTOR) == 14) {
		// set interrupt flag of first PATA port to true.
		k_setHddInterruptFlag(true, true);

	// process interrupt vector of second PATA port (IRQ 15).
	} else {
		// set interrupt flag of second PATA port to true.
		k_setHddInterruptFlag(false, true);
	}

	k_sendEoiToPic(vectorNumber - PIC_IRQSTARTVECTOR);
}
