#include "shell.h"
#include "console.h"
#include "keyboard.h"
#include "../utils/util.h"
#include "pit.h"
#include "rtc.h"
#include "asm_util.h"
#include "task.h"
#include "sync.h"
#include "dynamic_mem.h"
#include "hdd.h"
#include "file_system.h"
#include "serial_port.h"
#include "mp_config_table.h"
#include "multiprocessor.h"
#include "pic.h"
#include "local_apic.h"
#include "io_apic.h"
#include "interrupt_handlers.h"
#include "vbe.h"
#include "window.h"
#include "../gui_tasks/shell.h"
#include "window_manager.h"
#include "syscall.h"
#include "app_manager.h"
#include "../utils/queue.h"

static ShellCommandEntry g_commandTable[] = {
		{"help", "show help", k_help},
		{"clear", "clear screen", k_clear},
		{"ram", "show total RAM size", k_showTotalRamSize},
		{"reboot", "reboot system", k_reboot},
		{"shutdown", "shut down system", k_shutdown},
		{"cpus", "measure CPU speed", k_measureCpuSpeed},
		{"date", "show current date and time", k_showDateAndTime},
		{"chpr" ,"change task priority, usage) chpr <taskId> <priority>", k_changePriority},
		{"ts", "show task status, usage) ts <option>", k_showTaskStatus},
		{"ps", "show task status, usage) ps <option>", k_showTaskStatus},
		{"kill", "kill task, usage) kill <taskId>", k_killTask},
		{"cpul", "show CPU load", k_showCpuLoad},
		{"matrix", "show Matrix", k_showMatrix},
		{"dmem", "show dynamic memory info", k_showDynamicMemInfo},
		{"hdd", "show HDD info", k_showHddInfo},
		{"format", "format HDD", k_format},
		{"mount", "mount HDD", k_mount},
		{"fs", "show file system info", k_showFileSystemInfo},
		{"ls", "show directory", k_showRootDir},
		{"ll", "show directory", k_showRootDir},
		{"create", "create file, usage) create <file>", k_createFileInRootDir},
		{"delete", "delete file, usage) delete <file>", k_deleteFileInRootDir},
		{"write", "write file, usage) write <file>", k_writeDataToFile},
		{"read", "read file, usage) read <file>", k_readDataFromFile},
		{"flush", "flush file system cache", k_flushCache},
		{"download", "download file using serial port, usage) download <file>", k_downloadFile},
		{"mpconf", "show MP configuration table info", k_showMpConfigTable},
		{"irqmap", "show IRQ to INTIN Map", k_showIrqToIntinMap},
		{"intcnt", "show interrupt count by core * IRQ, usage) intcnt <irq>", k_showInterruptCounts},
		{"chaf" ,"change task affinity, usage) chaf <taskId> <affinity>", k_changeAffinity},
		{"vbe", "show VBE mode info", k_showVbeModeInfo},
		{"run", "run application (.elf), usage) run <app> <arg1> <arg2> ...", k_runApp},
		{"install", "install application (.elf), usage) install <app>", k_install},
		{"uninstall", "uninstall application (.elf), usage) uninstall <app>", k_uninstall},
		{"exit", "exit shell", k_exitShell},
		#if __DEBUG__
		{"teststod", "test string to decimal/hex conversion, usage) teststod <decimal> <hex> ...", k_testStrToDecimalHex},
		{"timer", "set timer, usage) timer <ms> <periodic>", k_setTimer},
		{"wait", "wait, usage) wait <ms>", k_waitUsingPit},
		{"tsc", "read time stamp counter", k_readTimeStampCounter},
		{"testtask", "test task, usage) testtask <type> <count>", k_createTestTask},
		{"testmutex", "test mutex", k_testMutex},
		{"testthread", "test thread", k_testThread},
		{"testpi", "test Pi calculation", k_testPi},
		{"testdmem", "test dynamic memory, usage) testdmem <type>", k_testDynamicMem},
		{"writes", "write HDD sector, usage) writes <lba> <count>", k_writeSector},
		{"reads", "read HDD sector, usage) reads <lba> <count>", k_readSector},
		{"testfile", "test file IO", k_testFileIo},
		{"testperf", "test file IO performance", k_testPerformance},
		{"stap", "start application processor", k_startAp},
		{"stsim", "start symmetric IO mode", k_startSymmetricIoMode},
		{"stilb", "start interrupt load balancing", k_startInterruptLoadBalancing},
		{"sttlb", "start task load balancing", k_startTaskLoadBalancing},
		{"stmp", "start multiprocessor or multi-core processor mode", k_startMultiprocessorMode},
		{"testsup", "test screen update performance, usage) testsup <option>", k_testScreenUpdatePerformance},
		{"testsc", "test system call", k_testSyscall},
		{"testwait", "test wait task, usage) testwait <option> <taskId>", k_testWaitTask},
		#endif // __DEBUG__
};

void k_shellTask(void) {
	char commandBuffer[SHELL_MAXCOMMANDBUFFERCOUNT] = {'\0', };
	int commandBufferIndex = 0;
	byte key;
	int x, y;
	ConsoleManager* consoleManager;

	consoleManager = k_getConsoleManager();
	
	k_printf(SHELL_PROMPTMESSAGE);
	
	/* shell task loop */
	while (consoleManager->exit == false) {
		// wait until key will be received.
		key = k_getch();

		if (consoleManager->exit == true) {
			break;
		}
		
		/* process backspace key */
		if (key == KEY_BACKSPACE) {
			if (commandBufferIndex > 0) {
				k_getCursor(&x, &y);
				k_printStrXy(x - 1, y, " ");
				k_setCursor(x - 1, y);
				commandBufferIndex--;
			}
			
		/* process enter key */
		} else if (key == KEY_ENTER) {
			k_printf("\n");
			
			// execute command.
			if (commandBufferIndex > 0) {
				commandBuffer[commandBufferIndex] = '\0';
				k_executeCommand(commandBuffer);
			}
			
			// initialize screen and command buffer.
			k_printf(SHELL_PROMPTMESSAGE);
			k_memset(commandBuffer, '\0', sizeof(commandBuffer));
			commandBufferIndex = 0;
			
		/* ignore shift, caps lock, num lock, scroll lock key */
		} else if (key == KEY_LSHIFT || key == KEY_RSHIFT || key == KEY_CAPSLOCK || key == KEY_NUMLOCK || key == KEY_SCROLLLOCK) {
			;
			
		/* process other keys */
		} else if (key < 128) {
			if (key == KEY_TAB) {
				key = ' ';
			}
			
			if (commandBufferIndex < SHELL_MAXCOMMANDBUFFERCOUNT) {
				commandBuffer[commandBufferIndex++] = key;
				k_printf("%c", key);
			}
		}
	}
}

void k_executeCommand(const char* commandBuffer) {
	char trimmedBuffer[SHELL_MAXCOMMANDBUFFERCOUNT] = {'\0', };
	int i, j = 0, spaceIndex;
	int commandLen;
	int count;
	
	for (i = 0; commandBuffer[i] != '\0'; i++) {
		if (commandBuffer[i] == ' ' && (j == 0 || trimmedBuffer[j - 1] == ' ')) {
			continue;
		}
		
		trimmedBuffer[j++] = commandBuffer[i];
	}
	
	if (trimmedBuffer[j - 1] == ' ') {
		trimmedBuffer[j - 1] = '\0';
		j--;

	} else {
		trimmedBuffer[j] = '\0';
	}

	// If trimmed buffer length (j) <= 0, return.
	if (j <= 0) {
		return;
	}
	
	// get space index (command length).
	for (spaceIndex = 0; spaceIndex < j; spaceIndex++) {
		if (trimmedBuffer[spaceIndex] == ' ') {
			break;
		}
	}
	
	count = sizeof(g_commandTable) / sizeof(ShellCommandEntry);
	
	for (i = 0; i < count; i++) {
		commandLen = k_strlen(g_commandTable[i].command);
		
		// call a command function, if length and content of the command are right.
		if ((commandLen == spaceIndex) && (k_memcmp(g_commandTable[i].command, trimmedBuffer, spaceIndex) == 0)) {
			g_commandTable[i].func(trimmedBuffer + spaceIndex + 1); // call a command function, and pass parameters.
			break;
		}
	}
	
	// print error if the command doesn't exist in the command table.
	if (i >= count) {
		char command[spaceIndex + 1];
		k_memset(command, 0, spaceIndex + 1);
		k_memcpy(command, trimmedBuffer, spaceIndex);
		k_printf("%s not found\n", command);
	}
}

void k_initParam(ParamList* list, const char* paramBuffer) {
	list->buffer = paramBuffer;
	list->len = k_strlen(paramBuffer);
	list->currentIndex = 0;
}

int k_getNextParam(ParamList* list, char* param) {
	int spaceIndex;
	int len;
	
	if (list->currentIndex >= list->len) {
		return 0;
	}
	
	// get space index (parameter length).
	for (spaceIndex = list->currentIndex; spaceIndex < list->len; spaceIndex++) {
		if (list->buffer[spaceIndex] == ' ') {
			break;
		}
	}
	
	// copy parameter and update current index.
	k_memcpy(param, list->buffer + list->currentIndex, spaceIndex);
	len = spaceIndex - list->currentIndex;
	param[len] = '\0';
	list->currentIndex += len + 1;
	
	if (len >= SHELL_MAXPARAMETERLENGTH) {
		k_printf("too long parameter length: Parameter length must be less than %d\n", SHELL_MAXPARAMETERLENGTH - 1);
		return SHELL_ERROR_TOOLONGPARAMETERLENGTH;
	}
	
	// return current parameter length.
	return len;
}

static void k_help(const char* paramBuffer) {
	int i;
	int count;
	int x, y;
	int len, maxCommandLen = 0;
	
	k_printf("*** hOS Shell Help ***\n");
	
	count = sizeof(g_commandTable) / sizeof(ShellCommandEntry);
	
	// get the length of the longest command.
	for (i = 0; i < count; i++) {
		len = k_strlen(g_commandTable[i].command);
		if(len > maxCommandLen){
			maxCommandLen = len;
		}
	}
	
	// print help.
	for (i = 0; i < count; i++) {
		k_printf("- %s", g_commandTable[i].command);
		k_getCursor(&x, &y);
		k_setCursor(maxCommandLen, y);
		k_printf(" : %s\n", g_commandTable[i].desc);
		
		// ask a user to print more items, every after 15 items are printed.
		if ((i != 0) && ((i % 15) == 0)) {
			
			k_printf("Press any key to continue...('q' is quit): ");
			
			if (k_getch() == 'q') {
				k_printf("\n");
				break;
			}
			
			k_printf("\n");
		}
	}
}

static void k_clear(const char* paramBuffer) {
	k_clearScreen();
	k_setCursor(0, 1); // move the cursor to the second line, because the interrupt is printed in the first line.
}

static void k_showTotalRamSize(const char* paramBuffer) {
	k_printf("total RAM size: %d MB\n", k_getTotalRamSize());
}

static void k_reboot(const char* paramBuffer) {
	k_printf("flush file system cache.\n");
	if (k_flushFileSystemCache() == false) {
		k_printf("reboot error: file system cache flushing failure\n");
		return;
	}

	// reboot the system using Keyboard Controller.
	k_printf("reboot hOS.\n");
	k_rebootSystem();
}

static void k_shutdown(const char* paramBuffer) {
	k_printf("flush file system cache.\n");
	if (k_flushFileSystemCache() == false) {
		k_printf("shutdown error: file system cache flushing failure\n");
		return;
	}

	// shutdown the system.
	k_printf("shut down hOS.\n");
	k_shutdownSystem();
}

static void k_measureCpuSpeed(const char* paramBuffer) {
	int i;
	qword lastTsc, totalTsc = 0;
	
	// measure process speed indirectly using the changes of Time Stamp Counter for 10 seconds.
	k_disableInterrupt();
	for (i = 0; i < 200; i++) {
		lastTsc = k_readTsc();
		k_waitUsingDirectPit(MSTOCOUNT(50));
		totalTsc += (k_readTsc() - lastTsc);
	}
	
	// restore timer.
	k_initPit(MSTOCOUNT(1), true);
	k_enableInterrupt();
	
	k_printf("CPU speed: %d MHz\n", totalTsc / 10 / 1000 / 1000);
}

static void k_showDateAndTime(const char* paramBuffer) {
	word year;
	byte month, dayOfMonth, dayOfWeek;
	byte hour, minute, second;
	
	// read current time and date from RTC controller.
	k_readRtcTime(&hour, &minute, &second);
	k_readRtcDate(&year, &month, &dayOfMonth, &dayOfWeek);
	
	// print current date and time.
	k_printf(" %d-%d-%d %d:%d:%d (%s)\n", year, month, dayOfMonth, hour, minute, second, k_convertDayOfWeekToStr(dayOfWeek));
}

static void k_changePriority(const char* paramBuffer) {
	ParamList list;
	char param[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	qword taskId;
	byte priority;
	Task* task;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: taskId
	if (k_getNextParam(&list, param) <= 0) {
		k_printf("Usage) chpr <taskId> <priority>\n");
		k_printf("  - taskId: 8 bytes hexadecimal number\n");
		k_printf("  - priority: 0 ~ 4 (highest ~ lowest)\n");
		k_printf("  - example: chpr 0x300000002 0\n");
		return;
	}
	
	if (k_memcmp(param, "0x", 2) == 0) {
		taskId = k_atol16(param + 2);
		
	} else {
		taskId = k_atol10(param);
	}
	
	k_memset(param, '\0', SHELL_MAXPARAMETERLENGTH);
	
	// get No.2 parameter: priority
	if (k_getNextParam(&list, param) <= 0) {
		k_printf("Usage) chpr <taskId> <priority>\n");
		k_printf("  - taskId: 8 bytes hexadecimal number\n");
		k_printf("  - priority: 0 ~ 4 (highest ~ lowest)\n");
		k_printf("  - example: chpr 0x300000002 0\n");
		return;
	}
	
	priority = k_atol10(param);
	
	if (priority < 0 || priority > 4) {
		k_printf("invalid priority: %d, Priority must be 0 ~ 4.\n", priority);
		return;
	}
	
	task = k_getTaskFromPool(GETTASKOFFSET(taskId));

	if (task->link.id != taskId) {
		k_printf("task priority changing failure: Task does not exist.\n");

	} else if ((task->link.id >> 32) == 0) {
		k_printf("task priority changing failure: Task has not been allocated.\n");

	} else if ((task->flags & TASK_FLAGS_SYSTEM) == TASK_FLAGS_SYSTEM) {
		k_printf("task priority changing failure: System task can not be killed.\n");

	} else {
		if (k_changeTaskPriority(task->link.id, priority) == true) {
			k_printf("task priority changing success: 0x%q, %d\n", task->link.id, priority);
			
		} else {
			k_printf("task priority changing failure: 0x%q, %d\n", task->link.id, priority);
		}
	}
}

static void k_showTaskStatus(const char* paramBuffer) {
	ParamList list;
	char option[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	int optionLen;
	int i;
	Task* task;
	int count = 0;
	int totalTaskCount = 0;
	char buffer[20];
	int remainLen;
	int coreCount;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: option
	optionLen = k_getNextParam(&list, option);
	if (optionLen != 0) {
		if ((optionLen < 0) || ((k_equalStr(option, "-a") == false) && (k_equalStr(option, "-c") == false) && (k_equalStr(option, "-t") == false))) {
			k_printf("Usage) ts <option>\n");
			k_printf("  - option: -a (all info)\n");
			k_printf("  - option: -c (core info)\n");
			k_printf("  - option: -t (task info, default)\n"); // 'ts -t' is same as 'ts'.
			k_printf("  - example: ts\n");
			k_printf("  - example: ts -a\n");
			return;
		}
	}
	
	coreCount = k_getProcessorCount();
	for (i = 0; i < coreCount; i++) {
		totalTaskCount += k_getTaskCount(i);
	}
	
	k_printf("*** Task Status (%d) ***\n", totalTaskCount);
	
	/* print core info */
	if ((k_equalStr(option, "-a") == true) || (k_equalStr(option, "-c") == true)) {
		for (i = 0; i < coreCount; i++) {
			if ((i != 0) && ((i % 4) == 0)) {
				k_printf("\n");
			}
			
			k_sprintf(buffer, "core %d: %d", i, k_getTaskCount(i));
			k_printf(buffer);
			
			// put spaces to remain cells out of 19 cells.
			remainLen = 19 - k_strlen(buffer);
			k_memset(buffer, ' ', remainLen);
			buffer[remainLen] = '\0';
			k_printf(buffer);
		}
		
		if (k_equalStr(option, "-a") == true) {
			// ask a user to print more info.
			k_printf("\nPress any key to continue...('q' is quit): ");
			if (k_getch() == 'q') {
				k_printf("\n");
				return;
			}
			
			k_printf("\n");
			
		} else {
			k_printf("\n");
			return;
		}
	}
	
	/* print task info */
	k_printf("No  TID  PPID  Child  Flags  Priority  MemAddr  MemSize  Affinity  Core\n");
	
	for (i = 0; i < TASK_MAXCOUNT; i++) {
		task = k_getTaskFromPool(i);
		
		// check if high 32 bits of task ID (task allocation count) != 0.
		if ((task->link.id >> 32) != 0) {
			
			// ask a user to print more items, every after 10 items are printed.
			if ((count != 0) && ((count % 10) == 0)) {
				k_printf("Press any key to continue...('q' is quit): ");
				if (k_getch() == 'q') {
					k_printf("\n");
					break;
				}
				
				k_printf("\n");
			}
			
			k_printf("%d> 0x%q  0x%q  %d  %s%s%s%s%s%s%s%s  %d  0x%q  0x%q  %d  %d\n"
					,1 + count++
					,task->link.id
					,task->parentProcessId
					,k_getListCount(&(task->childThreadList))
					,(task->flags & TASK_FLAGS_WAIT) ? "W" : ""
					,(task->flags & TASK_FLAGS_END) ? "E" : ""
					,(task->flags & TASK_FLAGS_SYSTEM) ? "S" : ""
					,(task->flags & TASK_FLAGS_PROCESS) ? "P" : ""
					,(task->flags & TASK_FLAGS_THREAD) ? "T" : ""
					,(task->flags & TASK_FLAGS_IDLE) ? "I" : ""
					,(task->flags & TASK_FLAGS_GUI) ? "G" : ""
					,(task->flags & TASK_FLAGS_USER) ? "U" : ""
					,GETTASKPRIORITY(task->flags)
					,task->memAddr
					,task->memSize
					,task->affinity
					,task->apicId);
		}
	}
}

static void k_killTask(const char* paramBuffer) {
	ParamList list;
	char taskId_[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	qword taskId;
	Task* task;
	qword shellTaskId;
	qword guiShellTaskId;
	int i;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: taskId
	if (k_getNextParam(&list, taskId_) <= 0) {
		k_printf("Usage) kill <taskId>\n");
		k_printf("  - taskId: 8 bytes hexadecimal number\n");
		k_printf("  - example: kill 0x300000002\n");
		k_printf("  - example: kill all\n");
		k_printf("  - example: kill gui\n");
		k_printf("  - example: kill nogui\n");
		return;
	}
	
	shellTaskId = k_getRunningTask(k_getApicId())->link.id;
	guiShellTaskId = k_getWindow(g_guiShellId)->taskId;

	/* kill all tasks (except system tasks) */
	if (k_equalStr(taskId_, "all") == true) {
		for (i = 0; i < TASK_MAXCOUNT; i++) {
			task = k_getTaskFromPool(i);
			
			if (((task->link.id >> 32) != 0) && 
				((task->flags & TASK_FLAGS_SYSTEM) != TASK_FLAGS_SYSTEM) && 
				(task->link.id != shellTaskId) && 
				(task->link.id != guiShellTaskId)) {

				if (k_endTask(task->link.id) == true) {
					k_printf("Task (0x%q) has been killed.\n", task->link.id);
					
				} else {
					k_printf("task killing failure: 0x%q\n", task->link.id);
				}
			}
		}

	/* kill GUI tasks (except system tasks) */
	} else if (k_equalStr(taskId_, "gui") == true) {
		for (i = 0; i < TASK_MAXCOUNT; i++) {
			task = k_getTaskFromPool(i);

			if (((task->link.id >> 32) != 0) && 
				((task->flags & TASK_FLAGS_SYSTEM) != TASK_FLAGS_SYSTEM) && 
				((task->flags & TASK_FLAGS_GUI) == TASK_FLAGS_GUI) && 
				(task->link.id != shellTaskId) && 
				(task->link.id != guiShellTaskId)) {

				if (k_endTask(task->link.id) == true) {
					k_printf("Task (0x%q) has been killed.\n", task->link.id);

				} else {
					k_printf("task killing failure: 0x%q\n", task->link.id);
				}
			}
		}

	/* kill non-GUI tasks (except system tasks) */
	} else if (k_equalStr(taskId_, "nogui") == true) {
		for (i = 0; i < TASK_MAXCOUNT; i++) {
			task = k_getTaskFromPool(i);

			if (((task->link.id >> 32) != 0) && 
				((task->flags & TASK_FLAGS_SYSTEM) != TASK_FLAGS_SYSTEM) && 
				((task->flags & TASK_FLAGS_GUI) != TASK_FLAGS_GUI) && 
				(task->link.id != shellTaskId) && 
				(task->link.id != guiShellTaskId)) {

				if (k_endTask(task->link.id) == true) {
					k_printf("Task (0x%q) has been killed.\n", task->link.id);

				} else {
					k_printf("task killing failure: 0x%q\n", task->link.id);
				}
			}
		}

	/* kill a task with task ID (except system tasks) */
	} else {
		if (k_memcmp(taskId_, "0x", 2) == 0) {
			taskId = k_atol16(taskId_ + 2);
			
		} else {
			taskId = k_atol10(taskId_);
		}
				
		task = k_getTaskFromPool(GETTASKOFFSET(taskId));

		if (task->link.id != taskId) {
			k_printf("task killing failure: Task does not exist.\n");

		} else if ((task->link.id >> 32) == 0) {
			k_printf("task killing failure: Task has not been allocated.\n");

		} else if ((task->flags & TASK_FLAGS_SYSTEM) == TASK_FLAGS_SYSTEM) {
			k_printf("task killing failure: System task can not be killed.\n");

		} else if (task->link.id == shellTaskId) {
			k_printf("task killing failure: Shell task can not be killed.\n");

		} else if (task->link.id == guiShellTaskId) {
			k_printf("task killing failure: GUI shell task can not be killed.\n");

		} else {
			if (k_endTask(task->link.id) == true) {
				k_printf("Task (0x%q) has been killed.\n", task->link.id);

			} else {
				k_printf("task killing failure: 0x%q\n", task->link.id);
			}
		}
	}
}

static void k_showCpuLoad(const char* paramBuffer) {
	int i;
	char buffer[50];
	int remainLen;
	
	k_printf("*** CPU Load by Core ***\n");
	
	for (i = 0; i < k_getProcessorCount(); i++) {
		if ((i != 0) && ((i % 4) == 0)) {
			k_printf("\n");
		}
		
		k_sprintf(buffer, "core %d: %d %%", i, k_getProcessorLoad(i));
		k_printf("%s", buffer); // Using format "%s" is essential if buffer has '%' and '%' does not mean data type in buffer.
		
		// put spaces to remain cells out of 19 cells.
		remainLen = 19 - k_strlen(buffer);
		k_memset(buffer, ' ', remainLen);
		buffer[remainLen] = '\0';
		k_printf(buffer);
	}
	
	k_printf("\n");
}

static void k_showMatrix(const char* paramBuffer) {
	Task* process;
	
#define TASK_AFFINITY_LB 0xFF // load balancing (no affinity)
	process = k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_PROCESS, (void*)0xE00000, 0xE00000, (qword)k_matrixProcess, 0, TASK_AFFINITY_LB);
	if (process != null) {
		k_printf("Matrix process creation success: 0x%q\n", process->link.id);
		
		// wait until process exits.
		while ((process->link.id >> 32) != 0) {
			k_sleep(100);
		}
		
	} else {
		k_printf("Matrix process creation faiure\n");
	}
}

static void k_matrixProcess(void) {
	int i;
	
	for (i = 0; i < 300; i++) {
		if (k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_charDropThread, 0, TASK_AFFINITY_LB) == null) {
			break;
		}
		
		k_sleep(k_rand() % 5 + 5);
	}
	
	k_printf("%d threads have been created.\n", i);
	
	// exit process after key is received.
	k_getch();
}

static void k_charDropThread(void) {
	int x;
	int i;
	char text[2] = {0, };
	
	x = k_rand() % CONSOLE_WIDTH;
	
	while (true) {
		k_sleep(k_rand() % 20);
		
		if ((k_rand() % 20) < 15) {
			text[0] = ' ';
			for (i = 0; i < CONSOLE_HEIGHT - 1; i++) {
				k_printStrXy(x, i, text);
				k_sleep(50);
			}
			
		} else {
			for (i = 0; i < CONSOLE_HEIGHT - 1; i++) {
				text[0] = (i + k_rand()) % 128;
				k_printStrXy(x, i, text);
				k_sleep(50);
			}
		}
	}
}

static void k_showDynamicMemInfo(const char* paramBuffer) {
	qword startAddr, totalSize, metaSize, usedSize;
	qword endAddredss;
	qword totalRamSize;
	
	k_getDynamicMemInfo(&startAddr, &totalSize, &metaSize, &usedSize);
	endAddredss = startAddr + totalSize;
	totalRamSize = k_getTotalRamSize();
	
	k_printf("*** Dynamic Memory Info ***\n");
	k_printf("- start address  : 0x%q bytes (%d MB)\n", startAddr, startAddr / 1024 / 1024);
	k_printf("- end address    : 0x%q bytes (%d MB)\n", endAddredss, endAddredss / 1024 / 1024);
	k_printf("- total size     : 0x%q bytes (%d MB)\n", totalSize, totalSize / 1024 / 1024);
	k_printf("- meta size      : 0x%q bytes (%d KB)\n", metaSize, metaSize / 1024);
	k_printf("- used size      : 0x%q bytes (%d KB)\n", usedSize, usedSize / 1024);
	k_printf("- total RAM size : 0x%q bytes (%d MB)\n", totalRamSize * 1024 * 1024, totalRamSize);
}

static void k_showHddInfo(const char* paramBuffer) {
	HddInfo hddInfo;
	char buffer[100];
	
	// read hard disk info.
	if (k_getHddInfo(&hddInfo) == false) {
		k_printf("HDD info reading failure");
		return;
	}
	
	k_printf("*** Primary Master HDD Info ***\n");
	
	// print model number.
	k_memcpy(buffer, hddInfo.modelNumber, sizeof(hddInfo.modelNumber));
	buffer[sizeof(hddInfo.modelNumber) - 1] = '\0';
	k_printf("- model number   : %s\n", buffer);
	
	// print serial number.
	k_memcpy(buffer, hddInfo.serialNumber, sizeof(hddInfo.serialNumber));
	buffer[sizeof(hddInfo.serialNumber) - 1] = '\0';
	k_printf("- serial number  : %s\n", buffer);
	
	// print cylinder count, head count, sector count per cylinder.
	k_printf("- cylinder count : %d\n", hddInfo.numberOfCylinder);
	k_printf("- head count     : %d\n", hddInfo.numberOfHead);
	k_printf("- sector count   : %d\n", hddInfo.numberOfSectorPerCylinder);
	
	// print total sector count.
	k_printf("- total sectors  : %d sectors (%d MB)\n", hddInfo.totalSectors, hddInfo.totalSectors / 2 / 1024);
}

static void k_format(const char* paramBuffer) {
	if (k_formatHdd() == false) {
		k_printf("HDD format failure\n");
		return;
	}
	
	k_printf("HDD format success\n");
}

static void k_mount(const char* paramBuffer) {
	if (k_mountHdd() == false) {
		k_printf("HDD mount failure\n");
		return;
	}
	
	k_printf("HDD mount success\n");
}

static void k_showFileSystemInfo(const char* paramBuffer) {
	FileSystemManager manager;
	
	k_getFileSystemInfo(&manager);
	
	k_printf("*** File System Info ***\n");
	k_printf("- mounted                          : %s\n",         (manager.mounted == true) ? "true" : "false");
	k_printf("- MBR sector count                 : %d sectors\n",  (manager.mounted == true) ? 1 : 0);
	k_printf("- reserved sector count            : %d sectors\n",  manager.reservedSectorCount);
	k_printf("- cluster link area start address  : %d sectors\n",  manager.clusterLinkAreaStartAddr);
	k_printf("- cluster link area size           : %d sectors\n",  manager.clusterLinkAreaSize);
	k_printf("- data area start address          : %d sectors\n",  manager.dataAreaStartAddr);
	k_printf("- total cluster count              : %d clusters\n", manager.totalClusterCount);
	k_printf("- cache enable                     : %s\n",         (manager.cacheEnabled == true) ? "true" : "false");
}

static void k_showRootDir(const char* paramBuffer) {
	Dir* dir;
	int i, count, totalCount;
	dirent* entry;
	char buffer[76]; // make buffer size (76 bytes) fit to a line of text video memory (screen).
	char tempValue[50];
	dword totalByte;
	dword usedClusterCount;
	FileSystemManager manager;
	
	// get file system info.
	k_getFileSystemInfo(&manager);
	
	// open root directory. (ignore directory name(/), because only root directory exists.)
	dir = opendir("/");
	if (dir == null) {
		k_printf("root directory opening failure\n");
		return;
	}
	
	// get total file count, total file size, used cluster count in root directory.
	totalCount = 0;
	totalByte = 0;
	usedClusterCount = 0;
	while (true) {
		// read root directory.
		entry = readdir(dir);
		if (entry == null) {
			break;
		}
		
		// get total file count, total file size.
		totalCount++;
		totalByte += entry->fileSize;
		
		// get used cluster count
		if (entry->fileSize == 0) {
			// allocate minimum 1 cluster even for 0-sized file.
			usedClusterCount++;
			
		} else {
			// align file size with cluster-level (rounding up), get used cluster count.
			usedClusterCount += ((entry->fileSize + (FS_CLUSTERSIZE - 1)) / FS_CLUSTERSIZE);
		}
	}
	
	// loop for printing file list.
	rewinddir(dir);
	count = 0;
	while (true) {
		// read root directory.
		entry = readdir(dir);
		if (entry == null) {
			break;
		}
		
		// initialize buffer as spaces.
		k_memset(buffer, ' ', sizeof(buffer) - 1);
		buffer[sizeof(buffer)-1] = '\0';
		
		// set file name to buffer.
		k_memcpy(buffer, entry->d_name, k_strlen(entry->d_name));
		
		// set file size to buffer.
		k_sprintf(tempValue, "%d bytes", entry->fileSize);
		k_memcpy(buffer + 30, tempValue, k_strlen(tempValue));
		
		// set start cluster index to buffer.
		k_sprintf(tempValue, "0x%x clusters", entry->startClusterIndex);
		k_memcpy(buffer + 55, tempValue, k_strlen(tempValue));
		
		// print file list.
		k_printf("    %s\n", buffer);
		
		// ask a user to print more items, every after 15 items are printed.
		if ((count != 0) && ((count % 15) == 0)) {
			
			k_printf("Press any key to continue...('q' is quit): ");
			
			if (k_getch() == 'q') {
				k_printf("\n");
				break;
			}
			
			k_printf("\n");
		}
		
		count++;
	}
	
	// print total file count, total file size, free space of hard disk.
	k_printf("\t\ttotal file count : %d\n", totalCount);
	k_printf("\t\ttotal file size  : %d bytes (%d clusters)\n", totalByte, usedClusterCount);
	k_printf("\t\tfree space       : %d KB (%d clusters)\n", (manager.totalClusterCount - usedClusterCount) * FS_CLUSTERSIZE / 1024, manager.totalClusterCount - usedClusterCount);
	
	// close root directory.
	closedir(dir);
}

static void k_createFileInRootDir(const char* paramBuffer) {
	ParamList list;
	char fileName[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	int len;
	File* file;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: file
	if ((len = k_getNextParam(&list, fileName)) <= 0) {
		k_printf("Usage) create <file>\n");
		k_printf("  - file: file name\n");
		k_printf("  - example: create a.txt\n");
		return;
	}
	
	fileName[len] = '\0';
	
	if (len > (FS_MAXFILENAMELENGTH - 1)) {
		k_printf("file creation failure: too long file name\n");
		return;
	}
	
	// open file.
	file = fopen(fileName, "w");
	if (file == null) {
		k_printf("file creation failure: file opening failure\n");
		return;
	}
	
	// close file.
	fclose(file);
}

static void k_deleteFileInRootDir(const char* paramBuffer) {
	ParamList list;
	char fileName[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	int len;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: file
	if ((len = k_getNextParam(&list, fileName)) <= 0) {
		k_printf("Usage) delete <file>\n");
		k_printf("  - file: file name\n");
		k_printf("  - example: delete a.txt\n");
		return;
	}
	
	fileName[len] = '\0';
	
	if (len > (FS_MAXFILENAMELENGTH - 1)) {
		k_printf("file deletion failure: too long file name\n");
		return;
	}
	
	// remove file.
	if (remove(fileName) != 0) {
		k_printf("file deletion failure: file removing failure\n");
		return;
	}
}

static void k_writeDataToFile(const char* paramBuffer) {
	ParamList list;
	char fileName[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	int len;
	File* file;
	int enterCount;
	byte key;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: file
	if ((len = k_getNextParam(&list, fileName)) <= 0) {
		k_printf("Usage) write <file>\n");
		k_printf("  - file: file name\n");
		k_printf("  - example: write a.txt\n");
		return;
	}
	
	fileName[len] = '\0';
	
	if (len > (FS_MAXFILENAMELENGTH - 1)) {
		k_printf("file writing failure: too long file name\n");
		return;
	}
	
	// open file.
	file = fopen(fileName, "w");
	if (file == null) {
		k_printf("file writing failure: file opening failure\n");
		return;
	}
	
	// loop for writing file.
	enterCount = 0;
	while (true) {
		key = k_getch();
		
		// press enter key 3 times continuously to finish writing file.
		if (key == KEY_ENTER) {
			enterCount++;
			if (enterCount >= 3) {
				break;
			}
			
		} else {
			enterCount = 0;
		}
		
		k_printf("%c", key);
		
		// write file.
		if (fwrite(&key, 1, 1, file) != 1) {
			k_printf("file writing failure: file writing failure");
			break;
		}
	}
	
	// close file.
	fclose(file);
}

static void k_readDataFromFile(const char* paramBuffer) {
	ParamList list;
	char fileName[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	int len;
	File* file;
	int enterCount;
	byte key;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: file
	if ((len = k_getNextParam(&list, fileName)) <= 0) {
		k_printf("Usage) read <file>\n");
		k_printf("  - file:  file name\n");
		k_printf("  - example: read a.txt\n");
		return;
	}
	
	fileName[len] = '\0';
	
	if (len > (FS_MAXFILENAMELENGTH - 1)) {
		k_printf("file reading failure: too long file name\n");
		return;
	}
	
	// open file.
	file = fopen(fileName, "r");
	if (file == null) {
		k_printf("file reading failure: file opening failure\n");
		return;
	}
	
	// loop for reading file.
	enterCount = 0;
	while (true) {
		// read file.
		if (fread(&key, 1, 1, file) != 1) {
			break;
		}
		
		k_printf("%c", key);
		
		// increase count if enter key is pressed.
		if (key == KEY_ENTER) {
			enterCount++;
			
			// ask a user to print more lines, every after 15 lines are printed.
			if ((enterCount != 0) && ((enterCount % 15) == 0)) {
				
				k_printf("Press any key to continue...('q' is quit): ");
				
				if (k_getch() == 'q') {
					k_printf("\n");
					break;
				}
				
				k_printf("\n");
				enterCount = 0;
			}
		}
	}
	
	// close file.
	fclose(file);
}

static void k_flushCache(const char* paramBuffer) {
	qword tickCount;
	
	tickCount = k_getTickCount();
	
	k_printf("flush file system cache...");
	if (k_flushFileSystemCache() == true) {
		k_printf("success\n");
		
	} else {
		k_printf("failure\n");
	}
	
	k_printf("flush time: %d ms\n", k_getTickCount() - tickCount);
}

static void k_downloadFile(const char* paramBuffer) {
	ParamList list;
	char fileName[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	int fileNameLen;
	dword dataLen;
	File* file;
	dword receivedSize;
	dword tempSize;
	byte dataBuffer[SERIAL_FIFOMAXSIZE];
	qword lastReceivedTickCount;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: file
	if ((fileNameLen = k_getNextParam(&list, fileName)) <= 0) {
		k_printf("Usage) download <file>\n");
		k_printf("  - file: file name\n");
		k_printf("  - example: download a.txt\n");
		return;
	}
	
	fileName[fileNameLen] = '\0';
	
	if (fileNameLen > (FS_MAXFILENAMELENGTH - 1)) {
		k_printf("file downloading failure: too long file name\n");
		return;
	}
	
	// clear send/receive FIFO.
	k_clearSerialFifo();
	
	//----------------------------------------------------------------------------------------------------
	// receive data length (4 bytes), and send ACK
	//----------------------------------------------------------------------------------------------------
	k_printf("wait for data length...");
	receivedSize = 0;
	lastReceivedTickCount = k_getTickCount();
	
	// loop for receiving data length.
	while (receivedSize < 4) {
		// receive data for left bytes.
		tempSize = k_recvSerialData(((byte*)&dataLen) + receivedSize, 4 - receivedSize);
		receivedSize += tempSize;
		
		// wait for a while, if data is not ready to receive.
		if (tempSize == 0) {
			k_sleep(0);
			
			// return if the wait time exceeds 30 seconds.
			if ((k_getTickCount() - lastReceivedTickCount) > 30000) {
				k_printf("time out\n");
				return;
			}
			
		} else {
			lastReceivedTickCount = k_getTickCount();
		}
	}
	
	k_printf("%d bytes\n", dataLen);
	
	// received data length successfully, so send ACK.
	k_sendSerialData("A", 1);
	
	//----------------------------------------------------------------------------------------------------
	// save data from serial port to file.
	//----------------------------------------------------------------------------------------------------
	
	// create file.
	file = fopen(fileName, "w");
	if (file == null) {
		k_printf("%s opening failure\n", fileName);
		return;
	}
	
	k_printf("receive data...");
	receivedSize = 0;
	lastReceivedTickCount = k_getTickCount();
	
	// loop for receiving data.
	while (receivedSize < dataLen) {
		// receive data.
		tempSize = k_recvSerialData(dataBuffer, SERIAL_FIFOMAXSIZE);
		receivedSize += tempSize;
		
		// If the receive data exists, send ACK and write file.
		if (tempSize != 0) {
			
			// send ACK every when it reaches FIFO max size, and when receiving completes.
			if (((receivedSize % SERIAL_FIFOMAXSIZE) == 0) || (receivedSize == dataLen)) {
				k_sendSerialData("A", 1);
			}
			
			// write file.
			if (fwrite(dataBuffer, 1, tempSize, file) !=  tempSize) {
				k_printf("%s writing failure\n", fileName);
				break;
			}
			
			lastReceivedTickCount = k_getTickCount();
			
		// If the receive data doesn't exist, wait for a while.
		} else {
			k_sleep(0);
			
			// break if the wait time exceeds 10 seconds.
			if ((k_getTickCount() - lastReceivedTickCount) > 10000) {
				k_printf("time out\n");
				break;
			}
		}
	}
	
	k_printf("success\n");
	
	//----------------------------------------------------------------------------------------------------
	// check if data is received successfully, and close file, and flush file system cache
	//----------------------------------------------------------------------------------------------------
	
	// check if data is received successfully
	if (receivedSize != dataLen) {
		k_printf("data length and received size are different: data lengh: %d bytes, received size: %d bytes\n", dataLen, receivedSize);
		
	} else {
		k_printf("receiving complete: received size: %d bytes\n", receivedSize);
	}
	
	// close file, and flush file system cache
	fclose(file);
	k_flushFileSystemCache();
}

static void k_showMpConfigTable(const char* paramBuffer) {
	k_printMpConfigTable();
}

static void k_showIrqToIntinMap(const char* paramBuffer) {
	k_printIrqToIntinMap();
}

static void k_showInterruptCounts(const char* paramBuffer) {
	ParamList list;
	char param[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	int irq;
	int irqLen;
	int irqStart = 0;
	int irqEnd = INTERRUPT_MAXVECTORCOUNT;
	InterruptManager* interruptManager;
	int i, j;
	int coreCount;
	char buffer[20];
	int remainLen;
	int lineCount;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: irq
	irqLen = k_getNextParam(&list, param);
	if (irqLen != 0) {
		irq = k_atol10(param);
		
		if ((irqLen < 0) || (irq < 0) || (irq >= INTERRUPT_MAXVECTORCOUNT)) {
			k_printf("Usage) intcnt <irq>\n");
			k_printf("  - irq: 0 (timer)\n");
			k_printf("  - irq: 1 (PS/2 keyboard)\n");
			k_printf("  - irq: 2 (slave PIC)\n");
			k_printf("  - irq: 3 (serial port 2)\n");
			k_printf("  - irq: 4 (serial port 1)\n");
			k_printf("  - irq: 5 (parallel port 2)\n");
			k_printf("  - irq: 6 (floppy disk)\n");
			k_printf("  - irq: 7 (parallel port 1)\n");
			k_printf("  - irq: 8 (RTC)\n");
			k_printf("  - irq: 9 (reserved)\n");
			k_printf("  - irq: 10 (not used 1)\n");
			k_printf("  - irq: 11 (not used 2)\n");
			k_printf("  - irq: 12 (PS/2 mouse)\n");
			k_printf("  - irq: 13 (coprocessor)\n");
			k_printf("  - irq: 14 (hard disk 1)\n");
			k_printf("  - irq: 15 (hard disk 2)\n");
			k_printf("  - example: intcnt\n");
			k_printf("  - example: intcnt 0\n");
			return;
		}
		
		irqStart = irq;
		irqEnd = irq + 1;
	}
	
	coreCount = k_getProcessorCount();
	
	if (irqLen == 0) {
		k_printf("*** Interrupt Count by Core * IRQ ***\n");
		k_printf("===============================================================================\n");
		
		/* print header */
		// print 4 cores in a line, and allocate 15 cells in a core.
		for (i = 0; i < coreCount; i++) {
			if (i == 0) {
				k_printf("IRQ No\t\t");
				
			} else if ((i % 4) == 0) {
				k_printf("\n      \t\t");
			}
			
			k_sprintf(buffer, "core %d", i);
			k_printf(buffer);
			
			// put spaces to remain cells out of 15 cells.
			remainLen = 15 - k_strlen(buffer);
			k_memset(buffer, ' ', remainLen);
			buffer[remainLen] = '\0';
			k_printf(buffer);
		}
		
		k_printf("\n");
	}
	
	/* print content (interrupt count) */
	k_memset(buffer, 0, sizeof(buffer));
	lineCount = 0;
	interruptManager = k_getInterruptManager();
	for (i = irqStart; i < irqEnd; i++) {
		// print 4 cores in a line, and allocate 15 cells in a core.
		for (j = 0; j < coreCount; j++) {
			if (j == 0) {
				// ask a user to print more lines, every after more than 10 lines are printed.
				if ((lineCount != 0) && (lineCount > 10)) {
					k_printf("Press any key to continue...('q' is quit): ");
					if (k_getch() == 'q') {
						k_printf("\n");
						return;
					}
					
					lineCount = 0;
					k_printf("\n");
				}
				
				k_printf("-------------------------------------------------------------------------------\n");
				k_printf("IRQ %d\t\t", i);
				lineCount += 2;
				
			} else if ((j % 4) == 0) {
				k_printf("\n      \t\t");
				lineCount++;
			}
			
			k_sprintf(buffer, "0x%q", interruptManager->interruptCounts[j][i]);
			k_printf(buffer);
			
			// put spaces to remain cells out of 15 cells.
			remainLen = 15 - k_strlen(buffer);
			k_memset(buffer, ' ', remainLen);
			buffer[remainLen] = '\0';
			k_printf(buffer);
		}
		
		k_printf("\n");
	}
	
	if (irqLen == 0) {
		k_printf("===============================================================================\n");
		
	} else {
		k_printf("-------------------------------------------------------------------------------\n");
	}
}

static void k_changeAffinity(const char* paramBuffer) {
	ParamList list;
	char param[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	qword taskId;
	byte affinity;
	const byte maxAffinity = k_getProcessorCount() - 1;
	Task* task;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: taskId
	if (k_getNextParam(&list, param) <= 0) {
		k_printf("Usage) chaf <taskId> <affinity>\n");
		k_printf("  - taskId: 8 bytes hexadecimal number\n");
		k_printf("  - affinity: 0 ~ %d (core index), 255 (load balancing)\n", maxAffinity);
		k_printf("  - example: chaf 0x300000002 0\n");
		return;
	}
	
	if (k_memcmp(param, "0x", 2) == 0) {
		taskId = k_atol16(param + 2);
		
	} else {
		taskId = k_atol10(param);
	}
	
	k_memset(param, '\0', SHELL_MAXPARAMETERLENGTH);
	
	// get No.2 parameter: affinity
	if (k_getNextParam(&list, param) <= 0) {
		k_printf("Usage) chaf <taskId> <affinity>\n");
		k_printf("  - taskId: 8 bytes hexadecimal number\n");
		k_printf("  - affinity: 0 ~ %d (core index)\n", maxAffinity);
		k_printf("  - example: chaf 0x300000002 0\n");
		return;
	}
	
	if (k_memcmp(param, "0x", 2) == 0) {
		affinity = k_atol16(param + 2);
		
	} else {
		affinity = k_atol10(param);
	}
	
	if (affinity < 0 || affinity > maxAffinity) {
		k_printf("invalid affinity: %d, Affinity must be 0 ~ %d.\n", affinity, maxAffinity);
		return;
	}
	
	task = k_getTaskFromPool(GETTASKOFFSET(taskId));

	if (task->link.id != taskId) {
		k_printf("task affinity changing failure: Task does not exist.\n");

	} else if ((task->link.id >> 32) == 0) {
		k_printf("task affinity changing failure: Task has not been allocated.\n");

	} else if ((task->flags & TASK_FLAGS_SYSTEM) == TASK_FLAGS_SYSTEM) {
		k_printf("task affinity changing failure: System task can not be killed.\n");

	} else {
		if (k_changeTaskAffinity(task->link.id, affinity) == true) {
			k_printf("task affinity changing success: 0x%q, %d\n", task->link.id, affinity);
			
		} else {
			k_printf("task affinity changing failure: 0x%q, %d\n", task->link.id, affinity);
		}	
	}
}

static void k_showVbeModeInfo(const char* paramBuffer) {
	VbeModeInfoBlock* vbeMode;
	
	vbeMode = k_getVbeModeInfoBlock();
	
	k_printf("*** VBE Mode Info ***\n");
	k_printf("- window granularity          : 0x%x\n", vbeMode->winGranularity);
	k_printf("- x resolution                : %d pixels\n", vbeMode->xResolution);
	k_printf("- y resolution                : %d pixels\n", vbeMode->yResolution);
	k_printf("- bits per pixel              : %d bits\n", vbeMode->bitsPerPixel);
	k_printf("- red field position          : bit %d, mask size: %d bits\n", vbeMode->redFieldPos, vbeMode->redMaskSize);
	k_printf("- green field position        : bit %d, mask size: %d bits\n", vbeMode->greenFieldPos, vbeMode->greenMaskSize);
	k_printf("- blue field position         : bit %d, mask size: %d bits\n", vbeMode->blueFieldPos, vbeMode->blueMaskSize);
	k_printf("- physical base address       : 0x%x bytes\n", vbeMode->physicalBaseAddr);
	k_printf("- linear red field position   : bit %d, mask size: %d bits\n", vbeMode->linearRedFieldPos, vbeMode->linearRedMaskSize);
	k_printf("- linear green field position : bit %d, mask size: %d bits\n", vbeMode->linearGreenFieldPos, vbeMode->linearGreenMaskSize);
	k_printf("- linear blue field position  : bit %d, mask size: %d bits\n", vbeMode->linearBlueFieldPos, vbeMode->linearBlueMaskSize);
}

static void k_runApp(const char* paramBuffer) {
	ParamList list;
	char fileName[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	char args[APPMGR_MAXARGSLENGTH + 1] = {'\0', };
	int fileNameLen;
	int argsLen;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: app
	fileNameLen = k_getNextParam(&list, fileName);

	if (fileNameLen <= 0) {
		k_printf("Usage) run <app> <arg1> <arg2> ...\n");
		k_printf("  - app: application name (.elf can be omitted)\n");
		k_printf("  - args: argument string (max %d)\n", APPMGR_MAXARGSLENGTH);
		k_printf("  - example: run a.elf arg1 arg2 ...\n");
		return;
	}

	// get No.2 ~ N parameter: args
	argsLen = k_strlen(paramBuffer) - (fileNameLen + 1);
	k_memcpy(args, paramBuffer + fileNameLen + 1, argsLen);
	args[argsLen] = '\0';

	k_addFileExtension(fileName, "elf");

	k_executeApp(fileName, args, TASK_AFFINITY_LB);
}

static void k_install(const char* paramBuffer) {
	ParamList list;
	char fileName[SHELL_MAXPARAMETERLENGTH] = {'\0', };

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: app
	if (k_getNextParam(&list, fileName) <= 0) {
		k_printf("Usage) install <app>\n");
		k_printf("  - app: application name (.elf can be omitted)\n");
		k_printf("  - example: install a.elf\n");
		return;
	}

	k_addFileExtension(fileName, "elf");

	if (k_installApp(fileName) == true) {
		k_printf("%s installation success\n", fileName);

	} else {
		k_printf("%s installation failure\n", fileName);
	}
}

static void k_uninstall(const char* paramBuffer) {
	ParamList list;
	char fileName[SHELL_MAXPARAMETERLENGTH] = {'\0', };

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: app
	if (k_getNextParam(&list, fileName) <= 0) {
		k_printf("Usage) uninstall <app>\n");
		k_printf("  - app: application name (.elf can be omitted)\n");
		k_printf("  - example: uninstall a.elf\n");
		return;
	}

	k_addFileExtension(fileName, "elf");

	if (k_uninstallApp(fileName) == true) {
		k_printf("%s uninstallation success\n", fileName);

	} else {
		k_printf("%s uninstallation failure\n", fileName);
	}	
}

static void k_exitShell(const char* paramBuffer) {
	if (k_isGraphicMode() == false) {
		k_printf("shell exit failure: This command does not work in the text mode.\n");
		return;
	}

	k_sendWindowEventToWindow(g_guiShellId, EVENT_WINDOW_CLOSE);
}

#if __DEBUG__
static void k_testStrToDecimalHex(const char* paramBuffer) {
	ParamList list;
	char param[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	int len;
	int count = 0;
	long value;
	bool first = true;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	while (true) {
		// get parameter: decimal or hex
		len = k_getNextParam(&list, param);
		if (len < 0) {
			k_printf("Usage) teststod <decimal> <hex> ...\n");
			k_printf("  - decimal: decimal number\n");
			k_printf("  - hex: hexadecimal number\n");
			k_printf("  - example: teststod 19 0x1F 256\n");
			return;
		}
		
		if (first == true) {
			first = false;
			if (len == 0) {
				k_printf("Usage) teststod <decimal> <hex> ...\n");
				k_printf("  - decimal: decimal number\n");
				k_printf("  - hex: hexadecimal number\n");
				k_printf("  - example: teststod 19 0x1F 256\n");
				return;
			}
		}
		
		if (len == 0) {
			break;
		}
		
		k_printf("- param %d: '%s', len: %d, ", count + 1, param, len);
		
		// if parameter is a hexadecimal number.
		if (k_memcmp(param, "0x", 2) == 0) {
			value = k_atol16(param + 2);
			k_printf("hex: 0x%q\n", value); // add <0x> to the printed number.
			
		// if parameter is a decimal number.
		} else {
			value = k_atol10(param);
			k_printf("decimal: %d\n", value);
		}
		
		count++;
	}
}

static void k_setTimer(const char* paramBuffer) {
	ParamList list;
	char param[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	long millisecond;
	bool periodic;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: ms
	if (k_getNextParam(&list, param) <= 0) {
		k_printf("Usage) timer <ms> <periodic>\n");
		k_printf("  - ms: millisecond\n");
		k_printf("  - periodic: 0 (once)\n");
		k_printf("  - periodic: 1 (periodic)\n");
		k_printf("  - default: timer 1 1\n");
		return;
	}
	
	millisecond = k_atol10(param);
	
	k_memset(param, '\0', SHELL_MAXPARAMETERLENGTH);
	
	// get No.2 parameter: periodic
	if (k_getNextParam(&list, param) <= 0) {
		k_printf("Usage) timer <ms> <periodic>\n");
		k_printf("  - ms: millisecond\n");
		k_printf("  - periodic: 0 (once)\n");
		k_printf("  - periodic: 1 (periodic)\n");
		k_printf("  - default: timer 1 1\n");
		return;
	}
	
	periodic = k_atol10(param);
	
	// initialize PIT.
	k_initPit(MSTOCOUNT(millisecond), periodic);
	
	k_printf("set timer: %d ms, %d (%s)\n", millisecond, periodic, (periodic == true) ? "periodic" : "once");
}

static void k_waitUsingPit(const char* paramBuffer) {
	ParamList list;
	char param[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	int len;
	long millisecond;
	int i;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: ms
	if (k_getNextParam(&list, param) <= 0) {
		k_printf("Usage) wait <ms>\n");
		k_printf("  - ms: millisecond\n");
		k_printf("  - example: wait 1\n");
		return;
	}
	
	millisecond = k_atol10(param);
	
	k_printf("wait for %d ms...\n", millisecond);
	
	// disable interrupt, and measure time directly using PIT controller.
	k_disableInterrupt();
	for (i = 0; i < (millisecond / 30); i++) {
		k_waitUsingDirectPit(MSTOCOUNT(30));
	}
	k_waitUsingDirectPit(MSTOCOUNT(millisecond % 30));
	k_enableInterrupt();
	
	k_printf("wait complete\n");
	
	// restore timer.
	k_initPit(MSTOCOUNT(1), true);
}

static void k_readTimeStampCounter(const char* paramBuffer) {
	qword tsc;
	
	tsc = k_readTsc();
	
	k_printf("time stamp counter: %q\n", tsc);
}

static void k_createTestTask(const char* paramBuffer) {
	ParamList list;
	char param[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	long type, count;
	int i;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: type
	if (k_getNextParam(&list, param) <= 0) {
		k_printf("Usage) testtask <type> <count>\n");
		k_printf("  - type: 1 (border character)\n");
		k_printf("  - type: 2 (rotating pinwheel)\n");
		k_printf("  - type: 3 (core checker)\n");
		k_printf("  - count: 0 ~ 1022 (task count)\n");
		k_printf("  - example: testtask 1 1022\n");
		return;
	}
	
	type = k_atol10(param);
	
	k_memset(param, '\0', SHELL_MAXPARAMETERLENGTH);
	
	// get No.2 parameter: count
	if (k_getNextParam(&list, param) <= 0) {
		k_printf("Usage) testtask <type> <count>\n");
		k_printf("  - type: 1 (border character)\n");
		k_printf("  - type: 2 (rotating pinwheel)\n");
		k_printf("  - type: 3 (core checker)\n");
		k_printf("  - count: 0 ~ 1022 (task count)\n");
		k_printf("  - example: testtask 1 1022\n");
		return;
	}
	
	count = k_atol10(param);
	
	switch (type) {
	case 1: // test-task-1: border character
		for (i = 0; i < count; i++) {
			if (k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_testTask1, 0, TASK_AFFINITY_LB) == null) {
				break;
			}
			
			//k_schedule();
		}
		
		k_printf("created task-1 count: %d\n", i);
		break;
		
	case 2: // test-task-2: rotating pinwheel
		for (i = 0; i < count; i++) {
			if (k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_testTask2, 0, TASK_AFFINITY_LB) == null) {
				break;
			}
			
			//k_schedule();
		}
		
		k_printf("created task-2 count: %d\n", i);
		break;
		
	case 3: // test-task-3: core checker
		for (i = 0; i < count; i++) {
			if (k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_testTask3, 0, TASK_AFFINITY_LB) == null) {
				break;
			}
			
			k_schedule();
		}
		
		k_printf("created task-3 count: %d\n", i);
		break;
		
	default:
		k_printf("invalid type: %d, Type must be 1, 2, 3.\n", type);
		return;
	}
}

// test-task-1: print character moving around the border of screen.
static void k_testTask1(void) {
	byte data;
	int i = 0, x = 0, y = 0, margin, j;
	Char* screen;
	Task* runningTask;
	
	screen = k_getConsoleManager()->screenBuffer;

	// use the serial number of task ID as screen offset.
	runningTask = k_getRunningTask(k_getApicId());
	margin = (runningTask->link.id & 0xFFFFFFFF) % 10;
	
	for (j = 0; j < 20000; j++) {
		switch (i) {
		case 0:
			x++;
			if (x >= (CONSOLE_WIDTH - margin)) {
				i = 1;
			}
			break;
			
		case 1:
			y++;
			if (y >= (CONSOLE_HEIGHT - margin)) {
				i = 2;
			}
			break;
			
		case 2:
			x--;
			if (x < margin) {
				i = 3;
			}
			break;
			
		case 3:
			y--;
			if (y < margin) {
				i = 0;
			}
			break;
		}
		
		screen[y * CONSOLE_WIDTH + x].char_ = data;
		screen[y * CONSOLE_WIDTH + x].attr = data & 0x0F;
		data++;
		
		// It's commented out, because it has been upgraded from Round Robin Scheduler to Multilevel Queue Scheduler.
		//k_schedule();
	}
	
	// It's commented out, because the return address has been pushed to stack.
	//k_exitTask();
}

// test-task-2: print rotating pinwheel in the position corresponding to the serial number of task ID.
static void k_testTask2(void) {
	int i = 0, offset;
	Char* screen;
	Task* runningTask;
	char data[4] = {'-', '\\', '|', '/'};
	
	screen = k_getConsoleManager()->screenBuffer;

	// use the offset of running task ID as screen offset.
	runningTask = k_getRunningTask(k_getApicId());
	offset = (runningTask->link.id & 0xFFFFFFFF) * 2;
	offset = (CONSOLE_WIDTH * CONSOLE_HEIGHT) - (offset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));
	
	// infinite loop
	while (true) {
		// print rotating pinwheel.
		screen[offset].char_ = data[i % 4];
		screen[offset].attr = (offset % 15) + 1;
		i++;
		
		// It's commented out, because it has been upgraded from Round Robin Scheduler to Multilevel Queue Scheduler.
		//k_schedule();
	}
}

// test-task-3: print task ID and core ID whenever the task moves to another core.
static void k_testTask3(void) {
	qword taskId;
	Task* runningTask;
	byte lastApicId;
	qword lastTick;
	
	// get test-task-3 ID.
	runningTask = k_getRunningTask(k_getApicId());
	taskId = runningTask->link.id;
	
	k_printf("task-3 (0x%q) started on core %d\n", taskId, k_getApicId());
	
	lastApicId = k_getApicId();
	
	while (true) {
		// If the task has been moved to another core, print task ID and core ID.
		if (lastApicId != k_getApicId()) {
			k_printf("task-3 (0x%q) moved from core %d to core %d\n", taskId, lastApicId, k_getApicId());
			lastApicId = k_getApicId();
		}
		
		k_schedule();
	}
}

// for mutex test.
static Mutex g_testMutex;
static volatile qword g_testAdder;

static void k_testMutex(const char* paramBuffer) {
	int i;
	
	g_testAdder = 1;
	
	// initialize mutex
	k_initMutex(&g_testMutex);
	
	// create 3 tasks for mutex test.
	for (i = 0; i < 3; i++) {
		k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_numberPrintTask, 0, k_getApicId());
	}
	
	k_printf("wait for the mutex test until %d tasks end.\n", i);
	k_getch();
}

static void k_numberPrintTask(void) {
	int i, j;
	qword tickCount;
	
	// wait for 50 milliseconds in order to prevent the mutex test messages from duplicating with the console shell messages.
	tickCount = k_getTickCount();
	while ((k_getTickCount() - tickCount) < 50) {
		k_schedule();
	}
	
	// print mutex test number.
	for (i = 0; i < 5; i++) {
		k_lock(&g_testMutex);
		
		k_printf("mutex test: task ID: 0x%q, value: %d\n", k_getRunningTask(k_getApicId())->link.id, g_testAdder);
		g_testAdder++;
		
		k_unlock(&g_testMutex);
		
		// add this code to increase processor usage.
		for (j = 0; j < 30000; j++);
	}
	
	// wait for 1000 milliseconds which is enough time for all tasks to complete printing numbers
	// in order to prevent the mutex test messages from duplicating with the console shell messages.
	tickCount = k_getTickCount();
	while ((k_getTickCount() - tickCount) < 1000) {
		k_schedule();
	}
	
	// It's commented out, because the return address has been pushed to stack.
	//k_exitTask();
}

static void k_testThread(const char* paramBuffer) {
	Task* process;
	
	// create 1 process and 3 threads.
	process = k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_PROCESS, (void*)0xEEEEEEEE, 0x1000, (qword)k_threadCreationTask, 0, TASK_AFFINITY_LB);
	
	if (process != null) {
		k_printf("thread test success: 1 process (0x%q) and 3 threads have been created.\n", process->link.id);
		
	} else {
		k_printf("thread test failure: process creation failure\n");
	}
}

static void k_threadCreationTask(void) {
	int i;
	
	for (i = 0; i < 3; i++) {
		k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_testTask2, 0, TASK_AFFINITY_LB);
	}
	
	while (true) {
		k_sleep(1);
	}
}

static void k_testPi(const char* paramBuffer) {
	double result;
	int i;
	
	// print Pi after calculating it.
	k_printf("Pi: ");
	result = (double)355 / 113;
	//k_printf("%d.%d%d\n", (qword)result, ((qword)(result * 10) % 10), ((qword)(result * 100) % 10));
	k_printf("%f\n", result);
	
	// create 100 tasks for calculating float numbers (rotating pinwheels).
	for (i = 0; i < 100; i++) {
		k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_fpuTestTask, 0, TASK_AFFINITY_LB);
	}
}

static void k_fpuTestTask(void) {
	double value1;
	double value2;
	Task* runningTask;
	qword count = 0;
	qword randomValue;
	int i;
	int offset;
	char data[4] = {'-', '\\', '|', '/'};
	Char* screen;

	screen = k_getConsoleManager()->screenBuffer;
	
	// use the offset of current tast ID as screen offset.
	runningTask = k_getRunningTask(k_getApicId());
	offset = (runningTask->link.id & 0xFFFFFFFF) * 2;
	offset = (CONSOLE_WIDTH * CONSOLE_HEIGHT) - (offset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));
	
	// infinite loop
	while (true) {
		value1 = 1;
		value2 = 1;
		
		// calculate 2 times for test.
		for (i = 0; i < 10; i++) {
			randomValue = k_rand();
			value1 *= (double)randomValue;
			value2 *= (double)randomValue;
			
			k_sleep(1);
			
			randomValue = k_rand();
			value1 /= (double)randomValue;
			value2 /= (double)randomValue;
		}
		
		// If FPU operation has problems, exit a task with a error message.
		if (value1 != value2) {
			k_printf("FPU operation failure: values are not same, %f != %f\n", value1, value2);
			break;
		}
		
		// If FPU operation has no problems, print rotating pinwheel.
		screen[offset].char_ = data[count % 4];
		screen[offset].attr = (offset % 15) + 1;
		count++;
	}
}

static void k_testDynamicMem(const char* paramBuffer) {
	ParamList list;
	char param[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	long type;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: type
	if (k_getNextParam(&list, param) <= 0) {
		k_printf("Usage) testdmem <type>\n");
		k_printf("  - type: 1 (sequential allocation)\n");
		k_printf("  - type: 2 (random allocation)\n");
		k_printf("  - example: testdmem 1\n");
		return;
	}
	
	type = k_atol10(param);
	
	switch (type) {
	case 1: // sequential allocation
		k_testSeqAlloc();
		break;
		
	case 2: // random allocation
		k_testRandomAlloc();
		break;
		
	default:
		k_printf("invalid type: %d, Type must be 1, 2.", type);
		return;
	}
}

static void k_testSeqAlloc(void) {
	DynamicMemManager* manager;
	long i, j, k;
	qword* buffer;
	
	k_printf("*** Dynamic Memory Sequential Allocation Test ***\n");
	
	manager = k_getDynamicMemManager();
	
	for (i = 0; i < manager->maxLevelCount; i++) {
		
		k_printf("start block list (%d) test.\n", i);
		
		// allocate and compare every size of blocks.
		k_printf("allocate and compare memory...\n");
		
		for (j = 0; j < (manager->smallestBlockCount >> i); j++) {
			buffer = (qword*)k_allocMem(DMEM_MINSIZE << i);
			if (buffer == null) {
				k_printf("test failure: memory allocation failure\n");
				return;
			}
			
			// put value to allocated memory.
			for (k = 0; k < ((DMEM_MINSIZE << i) / 8); k++) {
				buffer[k] = k;
			}
			
			// compare
			for (k = 0; k < ((DMEM_MINSIZE << i) / 8); k++) {
				if (buffer[k] != k) {
					k_printf("test failure: memory comparison failure\n");
					return;
				}
			}
		}
		
		// free all blocks.
		k_printf("free memory...\n");
		for (j = 0; j < (manager->smallestBlockCount >> i); j++) {
			if (k_freeMem((void*)(manager->startAddr + ((DMEM_MINSIZE << i) * j))) == false) {
				k_printf("test failure: memory freeing failure\n");
				return;
			}
		}
	}
	
	k_printf("test success\n");
}

static void k_testRandomAlloc(void) {
	int i;
	
	k_printf("*** Dynamic Memory Random Allocation Test ***\n");
	
	for (i = 0; i < 1000; i++) {
		k_createTask(TASK_PRIORITY_LOWEST | TASK_FLAGS_THREAD, null, 0, (qword)k_randomAllocTask, 0, TASK_AFFINITY_LB);
	}
}

static void k_randomAllocTask(void) {
	Task* task;
	qword memSize;
	char buffer[200];
	byte* allocBuffer;
	int i, j;
	int y;
	
	task = k_getRunningTask(k_getApicId());
	y = (task->link.id) % 15 + 9;
	
	for (j = 0; j < 10; j++) {
		// allocate 1KB ~ 32MB size of memory.
		do {
			memSize = ((k_rand() % (32 * 1024)) + 1) * 1024;
			allocBuffer = (byte*)k_allocMem(memSize);
			
			// If memory allocation fails, wait for a while, because other tasks could be using memory.
			if (allocBuffer == 0) {
				k_sleep(1);
			}
			
		} while (allocBuffer == 0);
		
		k_sprintf(buffer, "| address (0x%q), size (0x%q) allocation success", allocBuffer, memSize);
		k_printStrXy(20, y, buffer);
		k_sleep(200);
		
		// divide buffer half, put the same random data to both of them.
		k_sprintf(buffer, "| address (0x%q), size (0x%q) write data...", allocBuffer, memSize);
		k_printStrXy(20, y, buffer);
		
		for (i = 0; i < (memSize / 2); i++) {
			allocBuffer[i] = k_rand() & 0xFF;
			allocBuffer[i+(memSize/2)] = allocBuffer[i];
		}
		
		k_sleep(200);
		
		// verify data.
		k_sprintf(buffer, "| address (0x%q), size (0x%q) verify data...", allocBuffer, memSize);
		k_printStrXy(20, y, buffer);
		
		for (i = 0; i < (memSize / 2); i++) {
			if (allocBuffer[i] != allocBuffer[i+(memSize/2)]) {
				k_printf("test failure: data verification failure: task ID: 0x%q\n", task->link.id);
				k_exitTask();
			}
		}
		
		k_freeMem(allocBuffer);
		k_sleep(200);
	}
	
	k_printf("test success\n");
	
	k_exitTask();
}

static void k_writeSector(const char* paramBuffer) {
	ParamList list;
	char param[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	dword lba;
	int sectorCount;
	char* buffer;
	int i, j;
	byte data;
	bool exit = false;
	static dword writeCount = 0;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: lba
	if (k_getNextParam(&list, param) <= 0) {
		k_printf("Usage) writes <lba> <count>\n");
		k_printf("  - lba: logical block address\n");
		k_printf("  - count: read sector count\n");
		k_printf("  - example: writes 0 10\n");
		return;
	}
	
	lba = k_atol10(param);
	
	k_memset(param, '\0', SHELL_MAXPARAMETERLENGTH);
	
	// get No.2 parameter: sectorCount
	if (k_getNextParam(&list, param) <= 0) {
		k_printf("Usage) writes <lba> <count>\n");
		k_printf("  - lba: logical block address\n");
		k_printf("  - count: read sector count\n");
		k_printf("  - example: writes 0 10\n");
		return;
	}
	
	sectorCount = k_atol10(param);
	
	writeCount++;
	
	// allocate sector-count-sized memory, put data to it. (Data pattern is created by LBA address (4 bytes) and write count (4 bytes).)
	buffer = (char*)k_allocMem(sectorCount * 512);
	for (j = 0; j < sectorCount; j++) {
		for (i = 0; i < 512; i += 8) {
			*(dword*)&(buffer[j*512+i]) = lba + j;
			*(dword*)&(buffer[j*512+i+4]) = writeCount;
		}
	}
	
	// write sectors
	if (k_writeHddSector(true, true, lba, sectorCount, buffer) != sectorCount) {
		k_printf("HDD sector writing failure\n");
	}
	
	k_printf("HDD sector writing success: LBA: %d, count: %d", lba, sectorCount);
	
	// print memory buffer.
	for (j = 0; j < sectorCount; j++) {
		for (i = 0; i < 512; i++) {
			if (!((j == 0) && (i == 0)) && ((i % 256) == 0)) {
				k_printf("\nPress any key to continue...('q' is quit): ");
				if (k_getch() == 'q') {
					exit = true;
					break;
				}
			}
			
			if ((i % 16) == 0) {
				k_printf("\n<LBA:%d, Offset:%d>\t| ", lba + j, i);
			}
			
			// add 0 to the number less than 16 in order to print double digits.
			data = buffer[j*512+i] & 0xFF;
			if (data < 16) {
				k_printf("0");
			}
			
			k_printf("%x ", data);
		}
		
		if (exit == true) {
			break;
		}
	}
	
	k_printf("\n");
	
	k_freeMem(buffer);
}

static void k_readSector(const char* paramBuffer) {
	ParamList list;
	char param[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	dword lba;
	int sectorCount;
	char* buffer;
	int i, j;
	byte data;
	bool exit = false;
	
	// initialize parameter.
	k_initParam(&list, paramBuffer);
	
	// get No.1 parameter: lba
	if (k_getNextParam(&list, param) <= 0) {
		k_printf("Usage) reads <lba> <count>\n");
		k_printf("  - lba: logical block address\n");
		k_printf("  - count: read sector count\n");
		k_printf("  - example: reads 0 10\n");
		return;
	}
	
	lba = k_atol10(param);
	
	k_memset(param, '\0', SHELL_MAXPARAMETERLENGTH);
	
	// get No.2 parameter: sectorCount
	if (k_getNextParam(&list, param) <= 0) {
		k_printf("Usage) reads <lba> <count>\n");
		k_printf("  - lba: logical block address\n");
		k_printf("  - count: read sector count\n");
		k_printf("  - example: reads 0 10\n");
		return;
	}
	
	sectorCount = k_atol10(param);
	
	// allocate sector-count-sized memory.
	buffer = (char*)k_allocMem(sectorCount * 512);
	
	// read sectors
	if (k_readHddSector(true, true, lba, sectorCount, buffer) == sectorCount) {
		k_printf("HDD sector reading successs: LBA: %d, count: %d", lba, sectorCount);
		
		// print memory buffer.
		for (j = 0; j < sectorCount; j++) {
			for (i = 0; i < 512; i++) {
				if (!((j == 0) && (i == 0)) && ((i % 256) == 0)) {
					k_printf("\nPress any key to continue...('q' is quit): ");
					if (k_getch() == 'q') {
						exit = true;
						break;
					}
				}
				
				if ((i % 16) == 0) {
					k_printf("\n<LBA:%d, offset:%d>\t| ", lba + j, i);
				}
				
				// add 0 to the number less than 16 in order to print double digits.
				data = buffer[j*512+i] & 0xFF;
				if (data < 16) {
					k_printf("0");
				}
				
				k_printf("%x ", data);
			}
			
			if (exit == true) {
				break;
			}
		}
		
		k_printf("\n");
		
	} else {
		k_printf("HDD sector reading failure\n");
	}
	
	k_freeMem(buffer);
}

static void k_testFileIo(const char* paramBuffer) {
	File* file;
	byte* buffer;
	int i;
	int j;
	dword randomOffset;
	dword byteCount;
	byte tempBuffer[1024];
	dword maxFileSize;
	bool fail = false;
	
	k_printf("*** File IO Test (10 tests) ***\n");
	
	// allocate 4MB-sized memory.
	maxFileSize = 4 * 1024 * 1024;
	buffer = (byte*)k_allocMem(maxFileSize);
	if (buffer == null) {
		k_printf("test failure: memory allocation failure\n");
		return;
	}
	
	// remove file before test.
	remove("testfile.tmp");
	
	//----------------------------------------------------------------------------------------------------
	// 1. File Opening Failure Test
	//----------------------------------------------------------------------------------------------------
	k_printf("1> file opening failure test..................................");
	
	// Read mode (r) doesn't create file and return null when the file doesn't exist.
	file = fopen("testfile.tmp", "r");
	if (file == null) {
		k_printf("pass\n");
		
	} else {
		k_printf("fail\n");
		fclose(file);
	}
	
	//----------------------------------------------------------------------------------------------------
	// 2. File Creation Test
	//----------------------------------------------------------------------------------------------------
	k_printf("2> file creation test..........................................");
	
	// Write mode (w) create file and return file handle when the file doesn't exist.
	file = fopen("testfile.tmp", "w");
	if (file != null) {
		k_printf("pass\n");
		
	} else {
		k_printf("fail\n");
	}
	
	//----------------------------------------------------------------------------------------------------
	// 3. Sequential Writing Test
	//----------------------------------------------------------------------------------------------------
	k_printf("3> sequential writing test (cluster size)......................");
	fail = false;
	
	// write data to a opened file.
	for (i = 0; i < 100; i++) {
		
		k_memset(buffer, i, FS_CLUSTERSIZE);
		
		// write file.
		if (fwrite(buffer, 1, FS_CLUSTERSIZE, file) != FS_CLUSTERSIZE) {
			fail = true;
			k_printf("fail: %d cluster\n", i);
			break;
		}
	}
	
	if (fail == false) {
		k_printf("pass\n");
	}
	
	//----------------------------------------------------------------------------------------------------
	// 4. Sequential Reading/Verification Test
	//----------------------------------------------------------------------------------------------------
	k_printf("4> sequential reading/verification test (cluster size).........");
	fail = false;
	
	// move to the start of file.
	fseek(file, -100 * FS_CLUSTERSIZE, SEEK_END);
	
	// read data from opened file, and verify data.
	for (i = 0; i < 100; i++) {
		
		// read file.
		if (fread(buffer, 1, FS_CLUSTERSIZE, file) != FS_CLUSTERSIZE) {
			fail = true;
			k_printf("fail\n");
			break;
		}
		
		// verify data.
		for (j = 0; j < FS_CLUSTERSIZE; j++) {
			if (buffer[j] != (byte)i) {
				fail = true;
				k_printf("fail: %d cluster, 0x%x != 0x%x\n", i, buffer[j], (byte)i);
				break;
			}
		}
	}
	
	if (fail == false) {
		k_printf("pass\n");
	}
	
	//----------------------------------------------------------------------------------------------------
	// 5. Random Writing Test
	//----------------------------------------------------------------------------------------------------
	k_printf("5> random writing test.........................................");
	fail = false;
	
	// put 0 to buffer.
	k_memset(buffer, 0, maxFileSize);
	
	// move to the start of file, and read data from file, and copy it to buffer.
	fseek(file, -100 * FS_CLUSTERSIZE, SEEK_CUR);
	fread(buffer, 1, maxFileSize, file);
	
	// write the same data to both file and buffer.
	for (i = 0; i < 100; i++) {
		byteCount = (k_rand() % (sizeof(tempBuffer) - 1)) + 1;
		randomOffset = k_rand() % (maxFileSize - byteCount);
		
		// write data in the random position of file.
		fseek(file, randomOffset, SEEK_SET);
		k_memset(tempBuffer, i, byteCount);
		if (fwrite(tempBuffer, 1, byteCount, file) != byteCount) {
			fail = true;
			k_printf("fail: [%d] offset: %d, byte: %d\n", i, randomOffset, byteCount);
			break;
		}
		
		// write data in the random position of buffer.
		k_memset(buffer + randomOffset, i, byteCount);
	}
	
	if (fail == false) {
		k_printf("pass\n");
	}
	
	// move to the end of file and buffer, and write data (1 byte), and make the size 4MB.
	fseek(file, maxFileSize - 1, SEEK_SET);
	fwrite(&i, 1, 1, file);
	buffer[maxFileSize - 1] = (byte)i;
	
	//----------------------------------------------------------------------------------------------------
	// 6. Random Reading/Verification Test
	//----------------------------------------------------------------------------------------------------
	k_printf("6> random reading/verification test............................");
	fail = false;
	
	// read data from the random position of file and buffer, verify data.
	for (i = 0; i < 100; i++) {
		byteCount = (k_rand() % (sizeof(tempBuffer) - 1)) + 1;
		randomOffset = k_rand() % (maxFileSize - byteCount);
		
		// read data from the random position of file.
		fseek(file, randomOffset, SEEK_SET);
		if (fread(tempBuffer, 1, byteCount, file) != byteCount) {
			fail = true;
			k_printf("fail: file reading failure: [%d] offset: %d, byte: %d\n", i, randomOffset, byteCount);
			break;
		}
		
		// verify data.
		if (k_memcmp(buffer + randomOffset, tempBuffer, byteCount) != 0) {
			fail = true;
			k_printf("fail: data verification failure: [%d] offset: %d, byte: %d\n", i, randomOffset, byteCount);
			break;
		}
	}
	
	if (fail == false) {
		k_printf("pass\n");
	}
	
	//----------------------------------------------------------------------------------------------------
	// 7. Sequential Writing/Reading/Verification Test (1024 bytes)
	//----------------------------------------------------------------------------------------------------
	k_printf("7> sequential writing/reading/verification test (1024 bytes)...");
	fail = false;
	
	// move to the start of file.
	fseek(file, -maxFileSize, SEEK_CUR);
	
	// loop for writing file. (write data (2MB) from the start of file).
	for (i = 0; i < (2 * 1024 * 1024 / 1024); i++) {
		// write file (write data by 1024 bytes).
		if (fwrite(buffer + (i * 1024), 1, 1024, file) != 1024) {
			fail = true;
			k_printf("fail: file writing failure: [%d] offset: %d, byte: %d\n", i, i * 1024, 1024);
			break;
		}
	}
	
	// move to the start of file.
	fseek(file, -maxFileSize, SEEK_SET);
	
	// read data from file, and verify it.
	// verify the whole area (4MB), because wrong data could have been saved by the random write.
	for (i = 0; i < (maxFileSize / 1024); i++) {
		// read file. (read data by 1024 bytes.)
		if (fread(tempBuffer, 1, 1024, file) != 1024) {
			fail = true;
			k_printf("fail: file reading failure: [%d] offset: %d, byte: %d\n", i, i * 1024, 1024);
			break;
		}
		
		// verify data.
		if (k_memcmp(buffer + (i * 1024), tempBuffer, 1024) != 0) {
			fail = true;
			k_printf("fail: data verification failure: [%d] offset: %d, byte: %d\n", i, i * 1024, 1024);
			break;
		} 
	}
	
	if (fail == false) {
		k_printf("pass\n");
	}
	
	//----------------------------------------------------------------------------------------------------
	// 8. File Deletion Failure Test
	//----------------------------------------------------------------------------------------------------
	k_printf("8> file deletion failure test..................................");
	
	// the file delete test must fails, because the file is open.
	if (remove("testfile.tmp") != 0) {
		k_printf("pass\n");
		
	} else {
		k_printf("fail\n");
	}
	
	//----------------------------------------------------------------------------------------------------
	// 9. File Closing Failure Test
	//----------------------------------------------------------------------------------------------------
	k_printf("9> file closing failure test...................................");
	
	// close file.
	if (fclose(file) == 0) {
		k_printf("pass\n");
		
	} else {
		k_printf("fail\n");
	}
	
	//----------------------------------------------------------------------------------------------------
	// 10. File Deletion Test
	//----------------------------------------------------------------------------------------------------
	k_printf("10> file deletion test.........................................");
	
	// remove file.
	if (remove("testfile.tmp") == 0) {
		k_printf("pass\n");
		
	} else {
		k_printf("fail\n");
	}
	
	// free memory.
	k_freeMem(buffer);
}

static void k_testPerformance(const char* paramBuffer) {
	File* file;
	dword clusterTestFileSize;
	dword oneByteTestFileSize;
	qword lastTickCount;
	dword i;
	byte* buffer;
	
	k_printf("*** File IO Performance Test (2 tests) ***\n");
	
	// set file size (cluster-level: 1MB, byte-level: 16KB)
	clusterTestFileSize = 1024 * 1024;
	oneByteTestFileSize = 16 * 1024;
	
	// allocate memory.
	buffer = (byte*)k_allocMem(clusterTestFileSize);
	if (buffer == null) {
		k_printf("test failure: memory allocation failure\n");
		return;
	}
	
	k_memset(buffer, 0, FS_CLUSTERSIZE);
	
	//----------------------------------------------------------------------------------------------------
	// 1-1. Sequential Writing Test by Cluster Unit
	//----------------------------------------------------------------------------------------------------
	k_printf("1> sequential writing/reading test (cluster -> 1 MB)\n");
	
	// remove file, and create it.
	remove("testperf.tmp");
	file = fopen("testperf.tmp", "w");
	if (file == null) {
		k_printf("test failure: file opening failure\n");
		k_freeMem(buffer);
		return;
	}
	
	lastTickCount = k_getTickCount();
	
	// write test by cluster-level.
	for (i = 0; i < (clusterTestFileSize / FS_CLUSTERSIZE); i++) {
		if (fwrite(buffer, 1, FS_CLUSTERSIZE, file) != FS_CLUSTERSIZE) {
			k_printf("test failure: file writing failure\n");
			fclose(file);
			remove("testperf.tmp");
			k_freeMem(buffer);
			return;
		}
	}
	
	// print test time.
	k_printf("  - write : %d ms\n", k_getTickCount() - lastTickCount);
	
	//----------------------------------------------------------------------------------------------------
	// 1-2. Sequential Reading Test by Cluster Unit
	//----------------------------------------------------------------------------------------------------
	
	// move to the start of file.
	fseek(file, 0, SEEK_SET);
	
	lastTickCount = k_getTickCount();
	
	// read test by cluster-level.
	for (i = 0; i < (clusterTestFileSize / FS_CLUSTERSIZE); i++) {
		if (fread(buffer, 1, FS_CLUSTERSIZE, file) != FS_CLUSTERSIZE) {
			k_printf("test failure: file reading failure\n");
			fclose(file);
			remove("testperf.tmp");
			k_freeMem(buffer);
			return;
		}
	}
	
	// print test time.
	k_printf("  - read  : %d ms\n", k_getTickCount() - lastTickCount);
	
	//----------------------------------------------------------------------------------------------------
	// 2-1. Sequential Writing Test by Byte Unit
	//----------------------------------------------------------------------------------------------------
	k_printf("2> sequential writing/reading test (byte -> 16 KB)\n");
	
	// remove file and create it.
	fclose(file);
	remove("testperf.tmp");
	file = fopen("testperf.tmp", "w");
	if (file == null) {
		k_printf("test failure: file opening failure\n");
		k_freeMem(buffer);
		return;
	}
	
	lastTickCount = k_getTickCount();
	
	// write test by byte-level.
	for (i = 0; i < oneByteTestFileSize; i++) {
		if (fwrite(buffer, 1, 1, file) != 1) {
			k_printf("test failure: file writing failure\n");
			fclose(file);
			remove("testperf.tmp");
			k_freeMem(buffer);
			return;
		}
	}
	
	// print test time.
	k_printf("  - write : %d ms\n", k_getTickCount() - lastTickCount);
	
	//----------------------------------------------------------------------------------------------------
	// 2-2. Sequential Reading Test by Byte Unit
	//----------------------------------------------------------------------------------------------------
	
	// move to the start of file.
	fseek(file, 0, SEEK_SET);
	
	lastTickCount = k_getTickCount();
	
	// read test by byte-level.
	for (i = 0; i < oneByteTestFileSize; i++) {
		if (fread(buffer, 1, 1, file) != 1) {
			k_printf("test failure: file reading failure\n");
			fclose(file);
			remove("testperf.tmp");
			k_freeMem(buffer);
			return;
		}
	}
	
	// print test time.
	k_printf("  - read  : %d ms\n", k_getTickCount() - lastTickCount);
	
	// close file, and delete it, and free memory.
	fclose(file);
	remove("testperf.tmp");
	k_freeMem(buffer);
}

static void k_startAp(const char* paramBuffer) {
	k_printf("BSP (%d) wakes up APs.\n", k_getApicId());
	
	if (k_startupAp() == false) {
		k_printf("AP starting failure\n");
		return;
	}
}

static void k_startSymmetricIoMode(const char* paramBuffer) {
	MpConfigManager* mpManager;
	bool interruptFlag;
	
	if (k_analyzeMpConfigTable() == false) {
		k_printf("MP configuration table analysis failure\n");
		return;
	}
	
	mpManager = k_getMpConfigManager();
	
	// If it's PIC mode, disable PIC mode using IMCR Register.
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
	
	k_printf("symmetric IO mode switching success\n");
}

static void k_startInterruptLoadBalancing(const char* paramBuffer) {
	k_setInterruptLoadBalancing(true);
	k_printf("interrupt load balancing success\n");
}

static void k_startTaskLoadBalancing(const char* paramBuffer) {
	int i;
	
	for (i = 0; i < MAXPROCESSORCOUNT; i++) {
		k_setTaskLoadBalancing(i, true);
	}
	
	k_printf("task load balancing success\n");
}

static void k_startMultiprocessorMode(const char* paramBuffer) {
	k_startAp(paramBuffer);
	k_startSymmetricIoMode(paramBuffer);
	k_startInterruptLoadBalancing(paramBuffer);
	k_startTaskLoadBalancing(paramBuffer);
}

static void k_testScreenUpdatePerformance(const char* paramBuffer) {
	ParamList list;
	char option[SHELL_MAXPARAMETERLENGTH] = {'\0', };

	if (k_isGraphicMode() == false) {
		k_printf("screen update performance test failure: This command does not work in the text mode.\n");
		return;
	}

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: option
	if (k_getNextParam(&list, option) > 0) {
		if (k_equalStr(option, "-r") == false) {
			k_printf("Usage) testsup <option>\n");
			k_printf("  - option: -r (reset)\n");
			k_printf("  - example: testsup\n");
			k_printf("  - example: testsup -r\n");
			return;
		}
	}

	if (k_equalStr(option, "-r") == true) {
		g_winMgrMinLoopCount = 0xFFFFFFFFFFFFFFFF;

	} else {
		k_printf("window manager task min loop count: %d\n", g_winMgrMinLoopCount);
	}
}

static void k_testSyscall(const char* paramBuffer) {
	byte* taskMem;

	taskMem = (byte*)k_allocMem(0x1000); // 4 KB
	if (taskMem == null) {
		return;
	}

	k_memcpy(taskMem, k_syscallTestTask, 0x1000);

	k_createTask(TASK_FLAGS_PROCESS | TASK_FLAGS_USER, taskMem, 0x1000, (qword)taskMem, 0, TASK_AFFINITY_LB);
}

static void k_testWaitTask(const char* paramBuffer) {
	ParamList list;
	char option[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	char taskId_[SHELL_MAXPARAMETERLENGTH] = {'\0', };
	qword taskId;

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: option
	if ((k_getNextParam(&list, option) <= 0) || 
		(k_equalStr(option, "-w") == false && 
		 k_equalStr(option, "-n") == false && 
		 k_equalStr(option, "-i") == false &&
		 k_equalStr(option, "-b") == false &&
		 k_equalStr(option, "-nb") == false)) {
		k_printf("Usage) testwait <option> <taskId>\n");
		k_printf("  - option: -w (wait)\n");
		k_printf("  - option: -n (notify)\n");
		k_printf("  - option: -i (info)\n");
		k_printf("  - option: -b (blocking test)\n");
		k_printf("  - option: -nb (non-blocking test)\n");
		k_printf("  - example: testwait -w 0x300000002\n");
		k_printf("  - example: testwait -n 0x300000002\n");
		k_printf("  - example: testwait -i\n");
		k_printf("  - example: testwait -b\n");
		k_printf("  - example: testwait -nb\n");
		return;
	}

	if (k_equalStr(option, "-w") == true || k_equalStr(option, "-n") == true) {
		// get No.2 parameter: taskId
		if (k_getNextParam(&list, taskId_) <= 0) {
			k_printf("Usage) testwait <option> <taskId>\n");
			k_printf("  - option: -w (wait)\n");
			k_printf("  - option: -n (notify)\n");
			k_printf("  - option: -i (info)\n");
			k_printf("  - option: -b (blocking test)\n");
			k_printf("  - option: -nb (non-blocking test)\n");
			k_printf("  - example: testsup -w 0x300000002\n");
			k_printf("  - example: testsup -n 0x300000002\n");
			k_printf("  - example: testsup -i\n");
			k_printf("  - example: testsup -b\n");
			k_printf("  - example: testsup -nb\n");
			return;
		}

		if (k_memcmp(taskId_, "0x", 2) == 0) {
			taskId = k_atol16(taskId_ + 2);

		} else {
			taskId = k_atol10(taskId_);
		}
	}

	if (k_equalStr(option, "-w") == true) {
		k_waitTask(taskId);

	} else if (k_equalStr(option, "-n") == true) {
		k_notifyTask(taskId);

	} else if (k_equalStr(option, "-i") == true) {
		k_printWaitTaskInfo();

	} else if (k_equalStr(option, "-b") == true) {
		k_testBlockingQueue();

	} else if (k_equalStr(option, "-nb") == true) {
		k_testNonblockingQueue();
	}
}

#define MAXWAITQUEUECOUNT 100

static Mutex g_waitMutex;
static Queue g_waitQueue;
static int g_waitQueueBuffer[MAXWAITQUEUECOUNT];

static void k_testBlockingQueue(void) {
	int i;
	int data;
	qword taskIds[3];

	k_initMutex(&g_waitMutex);
	k_initQueue(&g_waitQueue, g_waitQueueBuffer, sizeof(int), MAXWAITQUEUECOUNT, true);

	for (i = 0; i < 3; i++) {
		taskIds[i] = k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_blockingTask, 0, TASK_AFFINITY_LB)->link.id;
	}

	for (i = 0; i < 10; i++) {
		k_sleep(1000); // sleep 1 second.
		data = i + 1;
		k_lock(&g_waitMutex);
		k_putQueue(&g_waitQueue, &data);
		k_unlock(&g_waitMutex);
	}

	data = 0x7FFFFFFF;

	k_lock(&g_waitMutex);
	for (i = 0; i < 3; i++) {
		k_putQueue(&g_waitQueue, &data);
	}
	k_unlock(&g_waitMutex);

	k_joinGroup(taskIds, 3);
	k_printf("-> Main task (0x%q) has ended.\n", k_getRunningTask(k_getApicId())->link.id);
}

static void k_blockingTask(void) {
	Task* task;
	int data;

	task = k_getRunningTask(k_getApicId());

	while (true) {
		k_lock(&g_waitMutex);

		if (k_getQueue(&g_waitQueue, &data, &g_waitMutex) == true) {
			k_unlock(&g_waitMutex);
			k_printf("[blocking got data] core %d, task 0x%q, data: %d\n", task->apicId, task->link.id, data);

		} else {
			k_unlock(&g_waitMutex);
			k_printf("[blocking no data]  core %d, task 0x%q, data: %d\n", task->apicId, task->link.id, 0);
		}

		if (data == 0x7FFFFFFF) {
			break;
		}
	}

	k_printf("-> Child task (0x%q) has ended.\n", task->link.id);
}

static void k_testNonblockingQueue(void) {
	int i;
	int data;
	qword taskIds[3];

	k_initMutex(&g_waitMutex);
	k_initQueue(&g_waitQueue, g_waitQueueBuffer, sizeof(int), MAXWAITQUEUECOUNT, false);

	for (i = 0; i < 3; i++) {
		taskIds[i] = k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_nonblockingTask, 0, TASK_AFFINITY_LB)->link.id;
	}

	for (i = 0; i < 10; i++) {
		k_sleep(1000); // sleep 1 second.
		data = i + 1;
		k_lock(&g_waitMutex);
		k_putQueue(&g_waitQueue, &data);
		k_unlock(&g_waitMutex);
	}

	data = 0x7FFFFFFF;

	k_lock(&g_waitMutex);
	for (i = 0; i < 3; i++) {
		k_putQueue(&g_waitQueue, &data);
	}
	k_unlock(&g_waitMutex);

	k_joinGroup(taskIds, 3);
	k_printf("-> Main task (0x%q) has ended.\n", k_getRunningTask(k_getApicId())->link.id);
}

static void k_nonblockingTask(void) {
	Task* task;
	int data;

	task = k_getRunningTask(k_getApicId());

	while (true) {
		k_lock(&g_waitMutex);

		if (k_getQueue(&g_waitQueue, &data, &g_waitMutex) == true) {
			k_unlock(&g_waitMutex);
			k_printf("[blocking got data] core %d, task 0x%q, data: %d\n", task->apicId, task->link.id, data);

		} else {
			k_unlock(&g_waitMutex);
			k_printf("[blocking no data]  core %d, task 0x%q, data: %d\n", task->apicId, task->link.id, 0);
			k_sleep(250);
		}

		if (data == 0x7FFFFFFF) {
			break;
		}
	}

	k_printf("-> Child task (0x%q) has ended.\n", task->link.id);
}

#endif // __DEBUG__