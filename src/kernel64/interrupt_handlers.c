#include "interrupt_handlers.h"
#include "pic.h"
#include "keyboard.h"
#include "console.h"
#include "util.h"
#include "task.h"
#include "descriptors.h"
#include "asm_util.h"
#include "hdd.h"
#include "local_apic.h"
#include "mp_config_table.h"
#include "multiprocessor.h"
#include "io_apic.h"

static InterruptManager g_interruptManager = {0, };

void k_initInterruptManager(void) {
	k_memset(&g_interruptManager, 0, sizeof(g_interruptManager));
}

void k_setSymmetricIoMode(bool symmetricIoMode) {
	g_interruptManager.symmetricIoMode = symmetricIoMode;
}

void k_setInterruptLoadBalancing(bool loadBalancing) {
	g_interruptManager.loadBalancing = loadBalancing;
}

void k_increaseInterruptCount(int irq) {
	g_interruptManager.interruptCounts[k_getApicId()][irq]++;
}

void k_sendEoi(int irq) {
	if (g_interruptManager.symmetricIoMode == false) {
		k_sendEoiToPic(irq);
		
	} else {
		k_sendEoiToLocalApic();
	}
}

InterruptManager* k_getInterruptManager(void) {
	return &g_interruptManager;
}

void k_processLoadBalancing(int irq) {
	qword minCount = 0xFFFFFFFFFFFFFFFF;
	int minCountCoreIndex = 0;
	int coreCount;
	int i;
	bool reset = false;
	byte apicId;
	
	apicId = k_getApicId();
	
	if ((g_interruptManager.interruptCounts[apicId][irq] == 0) ||
		((g_interruptManager.interruptCounts[apicId][irq] % INTERRUPT_LOADBALANCINGDIVIDOR) != 0) ||
		(g_interruptManager.loadBalancing == false)) {
		return;
	}
	
	coreCount = k_getProcessorCount();
	for (i = 0; i < coreCount; i++) {
		if (g_interruptManager.interruptCounts[i][irq] < minCount) {
			minCount = g_interruptManager.interruptCounts[i][irq];
			minCountCoreIndex = i;
			
		} else if (g_interruptManager.interruptCounts[i][irq] >= 0xFFFFFFFFFFFFFFFE) {
			reset = true;
		}
	}
	
	k_routeIrqToApic(irq, minCountCoreIndex);
	
	if (reset == true) {
		for (i = 0; i < coreCount; i++) {
			g_interruptManager.interruptCounts[i][irq] = 0;
		}
	}
}

void k_commonExceptionHandler(int vector, qword errorCode) {
	char buffer[5] = {0, };
	
	// set vector number (2 digits integer).
	buffer[0] = '0' + vector / 10;
	buffer[1] = '0' + vector % 10;
	
	k_printStrXy(0, 0, "--------------------------------------------------");
	k_printStrXy(0, 1, "|              Exception Occur                   |");
	k_printStrXy(0, 2, "|           vector:        core:                 |");
	k_printStrXy(20, 2, buffer);
	k_memset(buffer, 0, sizeof(buffer));
	k_sprintf(buffer, "%d", k_getApicId());
	k_printStrXy(33, 2, buffer);
	k_printStrXy(0, 3, "--------------------------------------------------");
	
	while (true);
}

void k_deviceNotAvailableHandler(int vector) {
	Tcb* fpuTask;        // last FPU-used task
	Tcb* currentTask;    // current task
	qword lastFpuTaskId; // last FPU-used task ID
	byte currentApicId;
	
	//====================================================================================================
	/* Print FPU Exception Message */
	char buffer[] = "[EXC:  , ]";
	static int fpuExceptionCount = 0;
	
	// set vector number (2 digits number).
	buffer[5] = '0' + vector / 10;
	buffer[6] = '0' + vector % 10;
	
	// set interrupt-occurred count (1 digit integer).
	fpuExceptionCount = (fpuExceptionCount + 1) % 10;
	buffer[8] = '0' + fpuExceptionCount;
	
	// print in the first position of the first line.
	k_printStrXy(0, 0, buffer);
	//====================================================================================================
	
	currentApicId = k_getApicId();
	
	// set CR0.TS=0
	k_clearTs();
	
	lastFpuTaskId = k_getLastFpuUsedTaskId(currentApicId);
	currentTask = k_getRunningTask(currentApicId);
	
	// If last FPU-used task is current task, it's not required to save and restore FPU context, and use current FPU context as is.
	if (lastFpuTaskId == currentTask->link.id) {
		return;
		
	// If last FPU-used task exists, save current FPU context to memory.
	} else if (lastFpuTaskId != TASK_INVALIDID) {
		fpuTask = k_getTaskFromTcbPool(GETTCBOFFSET(lastFpuTaskId));
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
	k_setLastFpuUsedTaskId(currentApicId, currentTask->link.id);
}

void k_commonInterruptHandler(int vector) {
	char buffer[] = "[INT:  , ]";
	static int commonInterruptCount = 0;
	int irq;
	
	//====================================================================================================
	/* Print Interrupt Message */
	// set vector number (2 digits integer).
	buffer[5] = '0' + vector / 10;
	buffer[6] = '0' + vector % 10;
	
	// set interrupt-occurred count (1 digit integer).
	commonInterruptCount = (commonInterruptCount + 1) % 10;
	buffer[8] = '0' + commonInterruptCount;
	
	// print in the last position of the first line.
	k_printStrXy(70, 0, buffer);
	//====================================================================================================
	
	irq = vector - PIC_IRQSTARTVECTOR;
	
	k_sendEoi(irq);
	
	k_increaseInterruptCount(irq);
	
	k_processLoadBalancing(irq);
}

void k_timerHandler(int vector) {
	char buffer[] = "[INT:  , ]";
	static int timerInterruptCount = 0;
	int irq;
	byte currentApicId;
	
	//====================================================================================================
	/* Print Interrupt Message */
	// set vector number (2 digits integer).
	buffer[5] = '0' + vector / 10;
	buffer[6] = '0' + vector % 10;
	
	// set interrupt-occurred count (1 digit integer).
	timerInterruptCount = (timerInterruptCount + 1) % 10;
	buffer[8] = '0' + timerInterruptCount;
	
	// print in the last position of the first line.
	k_printStrXy(70, 0, buffer);
	//====================================================================================================
	
	irq = vector - PIC_IRQSTARTVECTOR;
	
	k_sendEoi(irq);
	
	k_increaseInterruptCount(irq);
	
	currentApicId = k_getApicId();
	
	if (currentApicId == APICID_BSP) {
		g_tickCount++;
	}
	
	k_decreaseProcessorTime(currentApicId);
	
	if (k_isProcessorTimeExpired(currentApicId) == true) {
		k_scheduleInInterrupt();
	}
}

void k_keyboardHandler(int vector) {
	char buffer[] = "[INT:  , ]";
	static int keyboardInterruptCount = 0;
	byte scanCode;
	int irq;
	
	//====================================================================================================
	/* Print Interrupt Message */
	// set vector number (2 digits integer).
	buffer[5] = '0' + vector / 10;
	buffer[6] = '0' + vector % 10;
	
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
	
	irq = vector - PIC_IRQSTARTVECTOR;
	
	k_sendEoi(irq);
	
	k_increaseInterruptCount(irq);
	
	k_processLoadBalancing(irq);
}

void k_hddHandler(int vector) {
	char buffer[] = "[INT:  , ]";
	static int hddInterruptCount = 0;
	int irq;
	
	//====================================================================================================
	/* Print Interrupt Message */
	// set vector number (2 digits integer).
	buffer[5] = '0' + vector / 10;
	buffer[6] = '0' + vector % 10;
	
	// set interrupt-occurred count (1 digit integer).
	hddInterruptCount = (hddInterruptCount + 1) % 10;
	buffer[8] = '0' + hddInterruptCount;
	
	// print in the second position of the first line.
	k_printStrXy(10, 0, buffer);
	//====================================================================================================
	
	irq = vector - PIC_IRQSTARTVECTOR;
	
	// process interrupt vector of first PATA port (IRQ 14).
	if (irq == IRQ_HDD1) {
		// set interrupt flag of first PATA port to true.
		k_setHddInterruptFlag(true, true);
		
	// process interrupt vector of second PATA port (IRQ 15).
	} else {
		// set interrupt flag of second PATA port to true.
		k_setHddInterruptFlag(false, true);
	}
	
	k_sendEoi(irq);
	
	k_increaseInterruptCount(irq);
	
	k_processLoadBalancing(irq);
}
