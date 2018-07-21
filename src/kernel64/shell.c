#include "shell.h"
#include "console.h"
#include "keyboard.h"
#include "util.h"
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

static ShellCommandEntry g_commandTable[] = {
		{"help", "Show Help", k_help},
		{"cls", "Clear Screen", k_cls},
		{"totalram", "Show Total RAM Size", k_showTotalRamSize},
		{"strtod", "String to Decimal/Hex Convert, ex) strtod 19 0x1F", k_strToDecimalHexTest},
		{"shutdown", "Shutdown and Reboot OS", k_shutdown},
		{"settimer", "Set PIT Controller Counter0, ex) settimer 1(ms) 1(periodic)", k_setTimer},
		{"wait", "Wait ms Using PIT, ex) wait 1(ms)", k_waitUsingPit},
		{"rdtsc", "Read Time Stamp Counter", k_readTimeStampCounter},
		{"cpuspeed", "Measure Processor Speed", k_measureProcessorSpeed},
		{"date", "Show Current Data and Time", k_showDateAndTime},
		{"createtask", "Create Task, ex) createtask 1(type) 1022(count)", k_createTestTask},
		{"changepriority" ,"Change Task Priority, ex) changepriority 0x300000002(taskId) 0(priority)", k_changeTaskPriority},
		{"tasklist", "Show Task List", k_showTaskList},
		{"killtask", "End Task, ex) killtask 0x300000002(taskId) or all", k_killTask},
		{"cpuload", "Show Processor Load", k_cpuLoad},
		{"testmutex", "Test Mutex Function", k_testMutex},
		{"testthread", "Test Thread and Process Function", k_testThread},
		{"showmatrix", "Show Matrix Screen", k_showMatrix},
		{"testpie", "Test Pi Calculation", k_testPi},
		{"dynamicmeminfo", "Show Dynamic Memory Information", k_showDynamicMemInfo},
		{"testseqalloc", "Test Sequential Allocation & Free", k_testSeqAlloc},
		{"testranalloc", "Test Random Allocation & Free", k_testRandomAlloc},
		{"hddinfo", "Show HDD Information", k_showHddInfo},
		{"readsector", "Read HDD Sector, ex) readsector 0(LBA) 10(count)", k_readSector},
		{"writesector", "Write HDD Sector, ex) writesector 0(LBA) 10(count)", k_writeSector},
		{"mounthdd", "Mount HDD", k_mountHdd},
		{"formathdd", "Format HDD", k_formatHdd},
		{"filesysteminfo", "Show File System Information", k_showFileSystemInfo},
		{"createfile", "Create File, ex) createfile a.txt", k_createFileInRootDir},
		{"deletefile", "Delete File, ex) deletefile a.txt", k_deleteFileInRootDir},
		{"dir", "Show Root Directory", k_showRootDir},
		{"writefile", "Write Data to File, ex) writefile a.txt", k_writeDataToFile},
		{"readfile", "Read Data from File, ex) readfile a.txt", k_readDataFromFile},
		{"testfileio", "Test File I/O Function", k_testFileIo},
		{"testperformance", "Test File Read/Write Performance", k_testPerformance},
		{"flush", "Flush File System Cache", k_flushCache},
		{"download", "Download Data from Serial, ex) download a.txt", k_downloadFile},
		{"showmpinfo", "Show MP Configuration Table Information", k_showMpConfigTable},
		{"startap", "Start Application Processor", k_startAp},
		{"startsymmetricio", "Start Symmetric IO Mode", k_startSymmetricIoMode},
		{"showirqintinmap", "Show IRQ to INTIN Map", k_showIrqToIntinMap}
};

void k_startShell(void) {
	char commandBuffer[SHELL_MAXCOMMANDBUFFERCOUNT];
	int commandBufferIndex = 0;
	byte key;
	int x, y;

	k_printf(SHELL_PROMPTMESSAGE);

	// shell main loop
	while (true) {
		// wait until a key is received.
		key = k_getch();

		// process Backspace key.
		if (key == KEY_BACKSPACE) {
			if (commandBufferIndex > 0) {
				k_getCursor(&x, &y);
				k_printStrXy(x - 1, y, " ");
				k_setCursor(x - 1, y);
				commandBufferIndex--;
			}

		// process Enter key.
		} else if (key == KEY_ENTER) {
			k_printf("\n");

			// execute command.
			if (commandBufferIndex > 0) {
				commandBuffer[commandBufferIndex] = '\0';
				k_executeCommand(commandBuffer);
			}

			// initialize screen and command buffer.
			k_printf("%s", SHELL_PROMPTMESSAGE);
			k_memset(commandBuffer, '\0', SHELL_MAXCOMMANDBUFFERCOUNT);
			commandBufferIndex = 0;

		// ignore Shift, Caps Lock, Num Lock, Scroll Lock key.
		} else if (key == KEY_LSHIFT || key == KEY_RSHIFT || key == KEY_CAPSLOCK || key == KEY_NUMLOCK || key == KEY_SCROLLLOCK) {
			;

		// process other keys.
		} else {
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
	int i, spaceIndex;
	int commandBufferLen, commandLen;
	int count;

	commandBufferLen = k_strlen(commandBuffer);

	// get space position (command length)
	for (spaceIndex = 0; spaceIndex < commandBufferLen; spaceIndex++) {
		if (commandBuffer[spaceIndex] == ' ') {
			break;
		}
	}

	count = sizeof(g_commandTable) / sizeof(ShellCommandEntry);

	for (i = 0; i < count; i++) {
		commandLen = k_strlen(g_commandTable[i].command);

		// call a command function, if length and content of the command are right.
		if ((commandLen == spaceIndex) && (k_memcmp(g_commandTable[i].command, commandBuffer, spaceIndex) == 0)) {
			g_commandTable[i].func(commandBuffer + spaceIndex + 1); // call a command function, and pass parameters.
			break;
		}
	}

	// print error if the command dosen't exist in the command table.
	if (i >= count) {
		k_printf("'%s' is not found.\n", commandBuffer);
	}
}

void k_initParam(ParamList* list, const char* paramBuffer) {
	list->buffer = paramBuffer;
	list->len = k_strlen(paramBuffer);
	list->currentPos = 0;
}

int k_getNextParam(ParamList* list, char* param) {
	int i;
	int len;

	if (list->currentPos >= list->len) {
		return 0;
	}

	// get space position (parameter length).
	for (i = list->currentPos; i < list->len; i++) {
		if(list->buffer[i] == ' '){
			break;
		}
	}

	// copy parameter and update position.
	k_memcpy(param, list->buffer + list->currentPos, i);
	len = i - list->currentPos;
	param[len] = '\0';
	list->currentPos += len + 1;

	// return the current parameter length.
	return len;
}

static void k_help(const char* paramBuffer) {
	int i;
	int count;
	int x, y;
	int len, maxCommandLen = 0;

	k_printf("===============================================================================\n");
	k_printf("                               HansOS Shell Help                               \n");
	k_printf("-------------------------------------------------------------------------------\n");

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
		k_printf("%s", g_commandTable[i].command);
		k_getCursor(&x, &y);
		k_setCursor(maxCommandLen, y);
		k_printf(" - %s\n", g_commandTable[i].help);

		// ask a user to print more items, every after 15 items are printed.
		if ((i != 0) && ((i % 15) == 0)) {

			k_printf("Press any key to continue...('q' is exit):");

			if (k_getch() == 'q') {
				k_printf("\n");
				break;
			}

			k_printf("\n");
		}
	}

	k_printf("===============================================================================\n");
}

static void k_cls(const char* paramBuffer) {
	k_clearScreen();
	k_setCursor(0, 1); // move the cursor to the second line, because the interrupt is printed in the first line.
}

static void k_showTotalRamSize(const char* paramBuffer) {
	k_printf("Total RAM Size = %d MB\n", k_getTotalRamSize());
}

static void k_strToDecimalHexTest(const char* paramBuffer) {
	ParamList list;
	char param[100];
	int len;
	int count = 0;
	long value;

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	while (true) {

		// get parameter: Decimal/Hex String
		len = k_getNextParam(&list, param);

		if (len == 0) {
			break;
		}

		k_printf("Parameter %d = '%s', Length = %d, ", count + 1, param, len);

		// if parameter is a hexadecimal number.
		if (k_memcmp(param, "0x", 2) == 0) {
			value = k_atoi(param + 2, 16);
			k_printf("Hex Value = 0x%q\n", value); // add <0x> to the printed number.

		// if parameter is a decimal number.
		} else {
			value = k_atoi(param, 10);
			k_printf("Decimal Value = %d\n", value);
		}

		count++;
	}
}

static void k_shutdown(const char* paramBuffer) {
	k_printf("System Shutdown Start...\n");

	// flush file system cache buffer to hard disk.
	k_printf("Flush File System Cache...\n");
	if (k_flushFileSystemCache() == true) {
		k_printf("Success~!!\n");

	} else {
		k_printf("Fail~!!\n");
	}

	// reboot PC using Keyboard Controller.
	k_printf("Press any key to reboot PC...\n");
	k_getch();
	k_reboot();
}

static void k_setTimer(const char* paramBuffer) {
	ParamList list;
	char param[100];
	long millisecond;
	bool periodic;

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: Millisecond
	if (k_getNextParam(&list, param) == 0) {
		k_printf("Wrong Usage, ex) settimer 1(ms) 1(periodic)\n");
		return;
	}

	millisecond = k_atoi(param, 10);

	// get No.2 parameter: Periodic
	if (k_getNextParam(&list, param) == 0) {
		k_printf("Wrong Usage, ex) settimer 1(ms) 1(periodic)\n");
		return;
	}

	periodic = k_atoi(param, 10);

	// initialize PIT.
	k_initPit(MSTOCOUNT(millisecond), periodic);

	k_printf("Time = %d ms, Periodic = %d Change Complete\n", millisecond, periodic);
}

static void k_waitUsingPit(const char* paramBuffer) {
	char param[100];
	int len;
	ParamList list;
	long millisecond;
	int i;

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: Millisecond
	if (k_getNextParam(&list, param) == 0) {
		k_printf("Wrong Usage, ex) wait 1(ms)\n");
		return;
	}

	millisecond = k_atoi(param, 10);

	k_printf("%d ms Sleep Start...\n", millisecond);

	// disable interrupt, and measure time directly using PIT controller.
	k_disableInterrupt();
	for (i = 0; i < (millisecond / 30); i++) {
		k_waitUsingDirectPit(MSTOCOUNT(30));
	}
	k_waitUsingDirectPit(MSTOCOUNT(millisecond % 30));
	k_enableInterrupt();

	k_printf("%d ms Sleep Complete\n", millisecond);

	// restore timer.
	k_initPit(MSTOCOUNT(1), true);
}

static void k_readTimeStampCounter(const char* paramBuffer) {
	qword tsc;

	tsc = k_readTsc();

	k_printf("Time Stamp Counter = %q\n", tsc);
}

static void k_measureProcessorSpeed(const char* paramBuffer) {
	int i;
	qword lastTsc, totalTsc = 0;

	k_printf("Now Measuring");

	// measure process speed indirectly using the changes of Time Stamp Counter for 10 seconds.
	k_disableInterrupt();
	for (i = 0; i < 200; i++) {
		lastTsc = k_readTsc();
		k_waitUsingDirectPit(MSTOCOUNT(50));
		totalTsc += (k_readTsc() - lastTsc);
		k_printf(".");
	}

	// restore timer.
	k_initPit(MSTOCOUNT(1), true);
	k_enableInterrupt();

	k_printf("\nCPU Speed = %d MHz\n", totalTsc / 10 / 1000 / 1000);
}

static void k_showDateAndTime(const char* paramBuffer) {
	word year;
	byte month, dayOfMonth, dayOfWeek;
	byte hour, minute, second;

	// read current time and date from RTC controller.
	k_readRtcTime(&hour, &minute, &second);
	k_readRtcDate(&year, &month, &dayOfMonth, &dayOfWeek);

	k_printf("Date: %d-%d-%d %d:%d:%d (%s)\n", year, month, dayOfMonth, hour, minute, second, k_convertDayOfWeekToStr(dayOfWeek));
}

// Test Task 1: print characters moving around the border of screen.
static void k_testTask1(void) {
	byte data;
	int i = 0, x = 0, y = 0, margin, j;
	Char* screen = (Char*)CONSOLE_VIDEOMEMORYADDRESS;
	Tcb* runningTask;

	// use the serial number of TCB.ID as screen offset.
	runningTask = k_getRunningTask();
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

// Test Task 2: print rotating pinwheels in the position corresponding to the serial number of TCB.ID.
static void k_testTask2(void) {
	int i = 0, offset;
	Char* screen = (Char*)CONSOLE_VIDEOMEMORYADDRESS;
	Tcb* runningTask;
	char data[4] = {'-', '\\', '|', '/'};

	// use the offset of current task ID as screen offset.
	runningTask = k_getRunningTask();
	offset = (runningTask->link.id & 0xFFFFFFFF) * 2;
	offset = (CONSOLE_WIDTH * CONSOLE_HEIGHT) - (offset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	// infinite loop
	while (true) {
		// print rotating pinwheels.
		screen[offset].char_ = data[i % 4];
		screen[offset].attr = (offset % 15) + 1;
		i++;

		// It's commented out, because it has been upgraded from Round Robin Scheduler to Multilevel Queue Scheduler.
		//k_schedule();
	}
}

static void k_createTestTask(const char* paramBuffer) {
	ParamList list;
	char param[100];
	long type, count;
	int i;

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: Type
	if (k_getNextParam(&list, param) == 0) {
		k_printf("Wrong Usage, ex) createtask 1(type) 1022(count)\n");
		return;
	}

	type = k_atoi(param, 10);

	// get No.2 parameter: Count
	if (k_getNextParam(&list, param) == 0) {
		k_printf("Wrong Usage, ex) createtask 1(type) 1022(count)\n");
		return;
	}

	count = k_atoi(param, 10);

	switch (type) {
	case 1: // Test Task 1: print border characters.
		for (i = 0; i < count; i++) {
			if (k_createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (qword)k_testTask1) == null) {
				break;
			}
		}

		k_printf("k_testTask1 Created(%d)\n", i);
		break;

	case 2: // Test Task 2: print rotating pinwheels.
	default:
		for (i = 0; i < count; i++) {
			if (k_createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (qword)k_testTask2) == null) {
				break;
			}
		}

		k_printf("k_testTask2 Created(%d)\n", i);
		break;
	}
}

static void k_changeTaskPriority(const char* paramBuffer) {
	ParamList list;
	char taskId_[30];
	char priority_[30];
	qword taskId;
	byte priority;

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: TaskID
	if (k_getNextParam(&list, taskId_) == 0) {
		k_printf("Wrong Usage, ex) changepriority 0x300000002(taskId) 0(priority)\n");
		return;
	}

	// get No.2 parameter: Priority
	if (k_getNextParam(&list, priority_) == 0) {
		k_printf("Wrong Usage, ex) changepriority 0x300000002(taskId) 0(priority)\n");
		return;
	}

	if (k_memcmp(taskId_, "0x", 2) == 0) {
		taskId = k_atoi(taskId_ + 2, 16);

	} else {
		taskId = k_atoi(taskId_, 10);
	}

	priority = k_atoi(priority_, 10);

	k_printf("Change Task Priority : TaskID=[0x%q], Priority=[%d] : ", taskId, priority);

	if (k_changePriority(taskId, priority) == true) {
		k_printf("Success~!!\n");

	} else {
		k_printf("Fail~!!\n");
	}
}

static void k_showTaskList(const char* paramBuffer) {
	int i;
	Tcb* task;
	int count = 0;
	int taskCount;
	char linePadding[4] = {0, };

	taskCount = k_getTaskCount();

	// align Task Total Count line.
	if (taskCount < 10) {
		k_memcpy(linePadding, "===", 3);

	} else if (taskCount < 100) {
		k_memcpy(linePadding, "==", 2);

	} else if (taskCount < 1000) {
		k_memcpy(linePadding, "=", 1);
	}

	k_printf("=========================== Task Total Count [%d] ===========================%s\n", taskCount, linePadding);

	for (i = 0; i < TASK_MAXCOUNT; i++) {
		task = k_getTcbInTcbPool(i);

		// check if high 32 bits of task ID (TCB allocation count) != 0.
		if ((task->link.id >> 32) != 0) {

			// ask a user to print more items, every after 5 items are printed.
			if ((count != 0) && ((count % 5) == 0)) {

				k_printf("Press any key to continue...('q' is exit):");

				if (k_getch() == 'q') {
					k_printf("\n");
					break;
				}

				k_printf("\n");
			}

			// Task List Print Info.
			// 1. TID  : Task ID
			// 2. PRI  : Priority
			// 3. FG   : Flags
			// 4. CTH  : Child Thread Count
			// 5. PPID : Parent Process ID
			// 6. MA   : Memory Address
			// 7. MS   : Memory Size
			// 8. S    : System Task
			// 9. P/T  : Process/Thread
			k_printf("[%d] TID=[0x%Q], PRI=[%d], FG=[0x%Q], CTH=[%d]\n"
					,1 + count++, task->link.id, GETPRIORITY(task->flags), task->flags, k_getListCount(&(task->childThreadList)));
			k_printf("     PPID=[0x%Q], MA=[0x%Q], MS=[0x%Q], S=[%d], P/T=[%d/%d]\n"
					,task->parentProcessId, task->memAddr, task->memSize
					,(task->flags & TASK_FLAGS_SYSTEM) ? 1 : 0
					,(task->flags & TASK_FLAGS_PROCESS) ? 1 : 0
					,(task->flags & TASK_FLAGS_THREAD) ? 1 : 0);
			k_printf("-------------------------------------------------------------------------------\n");
		}
	}
}

static void k_killTask(const char* paramBuffer) {
	ParamList list;
	char taskId_[30];
	qword taskId;
	Tcb* task;
	int i;

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: TaskID
	if (k_getNextParam(&list, taskId_) == 0) {
		k_printf("Wrong Usage, ex) killtask 0x300000002(taskId) or all\n");
		return;
	}

	// exit a task with specific ID.
	if (k_memcmp(taskId_, "all", 3) != 0) {

		if (k_memcmp(taskId_, "0x", 2) == 0) {
			taskId = k_atoi(taskId_ + 2, 16);

		} else {
			taskId = k_atoi(taskId_, 10);
		}

		// [Note] To exit the task, it requires only the offset (low 32 bits) of parameter TaskID,
		//        because parameter TaskID has been overrided with real TaskID from task pool.
		task = k_getTcbInTcbPool(GETTCBOFFSET(taskId));
		taskId = task->link.id;

		// exit tasks except not-allocated tasks and system tasks.
		if (((taskId >> 32) != 0) && ((task->flags & TASK_FLAGS_SYSTEM) == 0)) {
			k_printf("Kill Task : TaskID=[0x%q] : ", taskId);

			if (k_endTask(taskId) == true) {
				k_printf("Success~!!\n");

			} else {
				k_printf("Fail~!!\n");
			}

		} else {
			if ((taskId >> 32) == 0) {
				k_printf("Kill Task Fail : Task does not exist.\n");

			} else if ((task->flags & TASK_FLAGS_SYSTEM) != 0) {
				k_printf("Kill Task Fail : Task is system task.\n");

			} else {
				k_printf("Kill Task Fail\n");
			}
		}

	// exit all tasks except a console shell task and a idle task.
	} else {
		for (i = 0; i < TASK_MAXCOUNT; i++) {
			task = k_getTcbInTcbPool(i);
			taskId = task->link.id;

			// exit tasks except not-allocated tasks and system tasks.
			if (((taskId >> 32) != 0) && ((task->flags & TASK_FLAGS_SYSTEM) == 0)) {
				k_printf("Kill Task : TaskID=[0x%q] : ", taskId);

				if (k_endTask(taskId) == true) {
					k_printf("Success~!!\n");

				} else {
					k_printf("Fail~!!\n");
				}
			}
		}
	}
}

static void k_cpuLoad(const char* paramBuffer) {
	k_printf("Processor Load: %d %%\n", k_getProcessorLoad());
}

// for mutex test.
static Mutex g_mutex;
static volatile qword g_adder;

static void k_printNumberTask(const char* paramBuffer) {
	int i, j;
	qword tickCount;

	// wait for 50 milliseconds in order to prevent the mutex test messages from duplicating with the console shell messages.
	tickCount = k_getTickCount();
	while ((k_getTickCount() - tickCount) < 50) {
		k_schedule();
	}

	// print mutex test number.
	for (i = 0; i < 5; i++) {
		k_lock(&(g_mutex));

		k_printf("Test Mutex : TaskID=[0x%Q] Value=[%d]\n", k_getRunningTask()->link.id, g_adder);
		g_adder++;

		k_unlock(&(g_mutex));

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

static void k_testMutex(const char* paramBuffer) {
	int i;

	g_adder = 1;

	// initialize mutex
	k_initMutex(&g_mutex);

	// create 3 tasks for mutex test.
	for (i = 0; i < 3; i++) {
		k_createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (qword)k_printNumberTask);
	}

	k_printf("Wait for the mutex test until [%d] tasks end...\n", i);
	k_getch();
}

static void k_createThreadTask(void) {
	int i;

	for (i = 0; i < 3; i++) {
		k_createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (qword)k_testTask2);
	}

	while (true) {
		k_sleep(1);
	}
}

static void k_testThread(const char* paramBuffer) {
	Tcb* process;

	// create 1 process and 3 threads.
	process = k_createTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xEEEEEEEE, 0x1000, (qword)k_createThreadTask);

	if (process != null) {
		k_printf("Process [0x%Q] Create Success\n", process->link.id);

	} else {
		k_printf("Process Create Fail\n");
	}
}

static volatile qword g_randomValue = 0;

qword k_random(void) {
	g_randomValue = (g_randomValue * 412153 + 5571031) >> 16;
	return g_randomValue;
}

static void k_dropCharThread(void) {
	int x;
	int i;
	char text[2] = {0, };

	x = k_random() % CONSOLE_WIDTH;

	while (true) {
		k_sleep(k_random() % 20);

		if ((k_random() % 20) < 15) {
			text[0] = ' ';
			for (i = 0; i < CONSOLE_HEIGHT - 1; i++) {
				k_printStrXy(x, i, text);
				k_sleep(50);
			}

		} else {
			for (i = 0; i < CONSOLE_HEIGHT - 1; i++) {
				text[0] = i + k_random();
				k_printStrXy(x, i, text);
				k_sleep(50);
			}
		}
	}
}

static void k_matrixProcess(void) {
	int i;

	for (i = 0; i < 300; i++) {
		if (k_createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (qword)k_dropCharThread) == null) {
			break;
		}

		k_sleep(k_random() % 5 + 5);
	}

	k_printf("%d Threads are created.\n", i);

	// exit process after key is received.
	k_getch();
}

static void k_showMatrix(const char* paramBuffer) {
	Tcb* process;

	process = k_createTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xE00000, 0xE00000, (qword)k_matrixProcess);

	if (process != null) {
		k_printf("Matrix Process [0x%Q] Create Success\n", process->link.id);

		// wait until process exits.
		while ((process->link.id >> 32) != 0) {
			k_sleep(100);
		}

	} else {
		k_printf("Matrix Process Create Fail\n");
	}
}

static void k_fpuTestTask(void) {
	double value1;
	double value2;
	Tcb* runningTask;
	qword count = 0;
	qword randomValue;
	int i;
	int offset;
	char data[4] = {'-', '\\', '|', '/'};
	Char* screen = (Char*)CONSOLE_VIDEOMEMORYADDRESS;

	// use the offset of current tast ID as screen offset.
	runningTask = k_getRunningTask();
	offset = (runningTask->link.id & 0xFFFFFFFF) * 2;
	offset = (CONSOLE_WIDTH * CONSOLE_HEIGHT) - (offset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	// infinite loop
	while (true) {
		value1 = 1;
		value2 = 1;

		// calculate 2 times for test.
		for (i = 0; i < 10; i++) {
			randomValue = k_random();
			value1 *= (double)randomValue;
			value2 *= (double)randomValue;

			k_sleep(1);

			randomValue = k_random();
			value1 /= (double)randomValue;
			value2 /= (double)randomValue;

		}

		// If FPU operation has problems, exit a task with a error message.
		if (value1 != value2) {
			k_printf("Value is not same~!!, [%f] != [%f]\n", value1, value2);
			break;
		}

		// If FPU operation has no problems, print rotating pinwheels.
		screen[offset].char_ = data[count % 4];
		screen[offset].attr = (offset % 15) + 1;
		count++;
	}
}

static void k_testPi(const char* paramBuffer) {
	double result;
	int i;

	// print Pi after calculating it.
	k_printf("Pi Calculation Test\n");
	k_printf("Pi : 355 / 113 = ");
	result = (double)355 / 113;
	//k_printf("%d.%d%d\n", (qword)result, ((qword)(result * 10) % 10), ((qword)(result * 100) % 10));
	k_printf("%f\n", result);

	// create 100 tasks for calculating float numbers (rotating pinwheels).
	for (i = 0; i < 100; i++) {
		k_createTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (qword)k_fpuTestTask);
	}
}

static void k_showDynamicMemInfo(const char* paramBuffer) {
	qword startAddr, totalSize, metaSize, usedSize;
	qword endAddredss;
	qword totalRamSize;

	k_getDynamicMemInfo(&startAddr, &totalSize, &metaSize, &usedSize);
	endAddredss = startAddr + totalSize;
	totalRamSize = k_getTotalRamSize();

	k_printf("========================= Dynamic Memory Information ==========================\n");
	k_printf("Start Address  : 0x%Q byte, %d MB\n", startAddr, startAddr / 1024 / 1024);
	k_printf("End Address    : 0x%Q byte, %d MB\n", endAddredss, endAddredss / 1024 / 1024);
	k_printf("Total Size     : 0x%Q byte, %d MB\n", totalSize, totalSize / 1024 / 1024);
	k_printf("Meta Size      : 0x%Q byte, %d KB\n", metaSize, metaSize / 1024);
	k_printf("Used Size      : 0x%Q byte, %d KB\n", usedSize, usedSize / 1024);
	k_printf("-------------------------------------------------------------------------------\n");
	k_printf("Total RAM Size : 0x%Q byte, %d MB\n", totalRamSize * 1024 * 1024, totalRamSize);
	k_printf("===============================================================================\n");
}

static void k_testSeqAlloc(const char* paramBuffer){
	DynamicMemManager* manager;
	long i, j, k;
	qword* buffer;

	k_printf("====>>>> Dynamic Memory Sequential Test\n");

	manager = k_getDynamicMemManager();

	for (i = 0; i < manager->maxLevelCount; i++) {

		k_printf("Block List [%d] Test Start~!!\n", i);

		// allocate and compare every size of blocks.
		k_printf("Allocation and Compare: ");

		for (j = 0; j < (manager->smallestBlockCount >> i); j++) {

			buffer = (qword*)k_allocMem(DMEM_MIN_SIZE << i);
			if (buffer == null) {
				k_printf("\nAllocation Fail~!!\n");
				return;
			}

			// put value to allocated memory.
			for (k = 0; k < ((DMEM_MIN_SIZE << i) / 8); k++) {
				buffer[k] = k;
			}

			// compare
			for (k = 0; k < ((DMEM_MIN_SIZE << i) / 8); k++) {
				if (buffer[k] != k) {
					k_printf("\nCompare Fail~!!\n");
					return;
				}
			}

			// print <.> to show the progress.
			k_printf(".");
		}

		// free all blocks.
		k_printf("\nFree: ");
		for (j = 0; j < (manager->smallestBlockCount >> i); j++) {
			if (k_freeMem((void*)(manager->startAddr + ((DMEM_MIN_SIZE << i) * j))) == false) {
				k_printf("\nFree Fail~!!\n");
				return;
			}

			// print <.> to show the progress.
			k_printf(".");
		}

		k_printf("\n");
	}

	k_printf("Test Complete~!!\n");
}

static void k_randomAllocTask(void) {
	Tcb* task;
	qword memSize;
	char buffer[200];
	byte* allocBuffer;
	int i, j;
	int y;

	task = k_getRunningTask();
	y = (task->link.id) % 15 + 9;

	for (j = 0; j < 10; j++) {
		// allocate 1KB ~ 32MB size of memory.
		do {
			memSize = ((k_random() % (32 * 1024)) + 1) * 1024;
			allocBuffer = (byte*)k_allocMem(memSize);

			// If memory allocation fails, wait for a while, because other tasks could be using memory.
			if (allocBuffer == 0) {
				k_sleep(1);
			}

		} while (allocBuffer == 0);

		k_sprintf(buffer, "|Address=[0x%Q], Size=[0x%Q] Allocation Success~!!", allocBuffer, memSize);
		k_printStrXy(20, y, buffer);
		k_sleep(200);

		// divide buffer half, put the same random data to both of them.
		k_sprintf(buffer, "|Address=[0x%Q], Size=[0x%Q] Data Write...", allocBuffer, memSize);
		k_printStrXy(20, y, buffer);

		for (i = 0; i < (memSize / 2); i++) {
			allocBuffer[i] = k_random() & 0xFF;
			allocBuffer[i+(memSize/2)] = allocBuffer[i];
		}

		k_sleep(200);

		// verify data.
		k_sprintf(buffer, "|Address=[0x%Q], Size=[0x%Q] Data Verify...", allocBuffer, memSize);
		k_printStrXy(20, y, buffer);

		for (i = 0; i < (memSize / 2); i++) {
			if (allocBuffer[i] != allocBuffer[i+(memSize/2)]) {
				k_printf("TaskID=[0x%Q] Verify Fail~!!\n", task->link.id);
				k_exitTask();
			}
		}

		k_freeMem(allocBuffer);
		k_sleep(200);
	}

	k_exitTask();
}

static void k_testRandomAlloc(const char* paramBuffer) {
	int i;

	k_printf("====>>>> Dynamic Memory Random Test\n");

	for (i = 0; i < 1000; i++) {
		k_createTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, (qword)k_randomAllocTask);
	}
}

static void k_showHddInfo(const char* paramBuffer) {
	HddInfo hddInfo;
	char buffer[100];

	// read hard disk info.
	if (k_getHddInfo(&hddInfo) == false) {
		k_printf("HDD Information Read Fail~!!");
		return;
	}

	k_printf("======================= Primary Master HDD Information ========================\n");

	// print model number.
	k_memcpy(buffer, hddInfo.modelNumber, sizeof(hddInfo.modelNumber));
	buffer[sizeof(hddInfo.modelNumber) - 1] = '\0';
	k_printf("Model Number   : %s\n", buffer);

	// print serial number.
	k_memcpy(buffer, hddInfo.serialNumber, sizeof(hddInfo.serialNumber));
	buffer[sizeof(hddInfo.serialNumber) - 1] = '\0';
	k_printf("Serial Number  : %s\n", buffer);

	// print cylinder count, head count, sector count per cylinder.
	k_printf("Cylinder Count : %d\n", hddInfo.numberOfCylinder);
	k_printf("Head Count     : %d\n", hddInfo.numberOfHead);
	k_printf("Sector Count   : %d\n", hddInfo.numberOfSectorPerCylinder);

	// print total sector count.
	k_printf("Total Sectors  : %d sector, %d MB\n", hddInfo.totalSectors, hddInfo.totalSectors / 2 / 1024);

	k_printf("===============================================================================\n");
}

static void k_readSector(const char* paramBuffer) {
	ParamList list;
	char lba_[50];
	char sectorCount_[50];
	dword lba;
	int sectorCount;
	char* buffer;
	int i, j;
	byte data;
	bool exit = false;

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: LBA
	if (k_getNextParam(&list, lba_) == 0) {
		k_printf("Wrong Usage, ex) readsector 0(LBA) 10(count)\n");
		return;
	}

	// get No.2 parameter: SectorCount
	if (k_getNextParam(&list, sectorCount_) == 0) {
		k_printf("Wrong Usage, ex) readsector 0(LBA) 10(count)\n");
		return;
	}

	lba = k_atoi(lba_, 10);
	sectorCount = k_atoi(sectorCount_, 10);

	// allocate sector-count-sized memory.
	buffer = (char*)k_allocMem(sectorCount * 512);

	// read sectors
	if (k_readHddSector(true, true, lba, sectorCount, buffer) == sectorCount) {
		k_printf("HDD Sector Read : LBA=[%d], Count=[%d] : Success~!!", lba, sectorCount);

		// print memory buffer.
		for (j = 0; j < sectorCount; j++) {
			for (i = 0; i < 512; i++) {
				if (!((j == 0) && (i == 0)) && ((i % 256) == 0)) {
					k_printf("\nPress any key to continue...('q' is exit):");
					if (k_getch() == 'q') {
						exit = true;
						break;
					}
				}

				if ((i % 16) == 0) {
					k_printf("\n[LBA:%d, Offset:%d]\t| ", lba + j, i);
				}

				// add 0 to the number less than 16 in order to print double digits.
				data = buffer[j*512+i] & 0xFF;
				if (data < 16) {
					k_printf("0");
				}

				k_printf("%X ", data);
			}

			if (exit == true) {
				break;
			}
		}

		k_printf("\n");

	} else {
		k_printf("HDD Sector Read : Fail~!!\n");
	}

	k_freeMem(buffer);
}

static void k_writeSector(const char* paramBuffer) {
	ParamList list;
	char lba_[50];
	char sectorCount_[50];
	dword lba;
	int sectorCount;
	char* buffer;
	int i, j;
	byte data;
	bool exit = false;
	static dword writeCount = 0;

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: LBA
	if (k_getNextParam(&list, lba_) == 0) {
		k_printf("Wrong Usage, ex) writesector 0(LBA) 10(count)\n");
		return;
	}

	// get Not.2 parameter: SectorCount
	if (k_getNextParam(&list, sectorCount_) == 0) {
		k_printf("Wrong Usage, ex) writesector 0(LBA) 10(count)\n");
		return;
	}

	lba = k_atoi(lba_, 10);
	sectorCount = k_atoi(sectorCount_, 10);

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
		k_printf("HDD Sector Write : Fail~!!\n");
	}

	k_printf("HDD Sector Write : LBA=[%d], Count=[%d] : Success~!!", lba, sectorCount);

	// print memory buffer.
	for (j = 0; j < sectorCount; j++) {
		for (i = 0; i < 512; i++) {
			if (!((j == 0) && (i == 0)) && ((i % 256) == 0)) {
				k_printf("\nPress any key to continue...('q' is exit):");
				if (k_getch() == 'q') {
					exit = true;
					break;
				}
			}

			if ((i % 16) == 0) {
				k_printf("\n[LBA:%d, Offset:%d]\t| ", lba + j, i);
			}

			// add 0 to the number less than 16 in order to print double digits.
			data = buffer[j*512+i] & 0xFF;
			if (data < 16) {
				k_printf("0");
			}

			k_printf("%X ", data);
		}

		if (exit == true) {
			break;
		}
	}

	k_printf("\n");

	k_freeMem(buffer);
}

static void k_mountHdd(const char* paramBuffer) {
	if (k_mount() == false) {
		k_printf("HDD Mount Fail~!!\n");
		return;
	}

	k_printf("HDD Mount Success~!!\n");
}

static void k_formatHdd(const char* paramBuffer) {
	if (k_format() == false) {
		k_printf("HDD Format Fail~!!\n");
		return;
	}

	k_printf("HDD Format Success~!!\n");
}

static void k_showFileSystemInfo(const char* paramBuffer) {
	FileSystemManager manager;

	k_getFileSystemInfo(&manager);

	k_printf("=========================== File System Information ===========================\n");
	k_printf("Mounted                          : %s\n",         (manager.mounted == true) ? "true" : "false");
	k_printf("MBR Sector Count                 : %d sector\n",  (manager.mounted == true) ? 1 : 0);
	k_printf("Reserved Sector Count            : %d sector\n",  manager.reservedSectorCount);
	k_printf("Cluster Link Area Start Address  : %d sector\n",  manager.clusterLinkAreaStartAddr);
	k_printf("Cluster Link Area Size           : %d sector\n",  manager.clusterLinkAreaSize);
	k_printf("Data Area Start Address          : %d sector\n",  manager.dataAreaStartAddr);
	k_printf("Total Cluster Count              : %d cluster\n", manager.totalClusterCount);
	k_printf("Cache Enable                     : %s\n",         (manager.cacheEnabled == true) ? "true" : "false");
	k_printf("===============================================================================\n");
}

static void k_createFileInRootDir(const char* paramBuffer) {
	ParamList list;
	char fileName[50];
	int len;
	File* file;

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: FileName
	if ((len = k_getNextParam(&list, fileName)) == 0) {
		k_printf("Wrong Usage, ex) createfile a.txt\n");
		return;
	}

	fileName[len] = '\0';

	if (len > (FS_MAXFILENAMELENGTH - 1)) {
		k_printf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// open a file.
	file = fopen(fileName, "w");
	if (file == null) {
		k_printf("'%s' File Create Fail~!!\n", fileName);
		return;
	}

	// close a file.
	fclose(file);
}

static void k_deleteFileInRootDir(const char* paramBuffer) {
	ParamList list;
	char fileName[50];
	int len;

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: FileName
	if ((len = k_getNextParam(&list, fileName)) == 0) {
		k_printf("Wrong Usage, ex) deletefile a.txt\n");
		return;
	}

	fileName[len] = '\0';

	if (len > (FS_MAXFILENAMELENGTH - 1)) {
		k_printf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// remove a file.
	if (remove(fileName) != 0) {
		k_printf("'%s' File Delete Fail~!!\n", fileName);
		return;
	}
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
		k_printf("Root Directory Open Fail~!!\n");
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
			// align file size with cluster size unit (rounding up), get used cluster count.
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
		k_sprintf(tempValue, "%d byte", entry->fileSize);
		k_memcpy(buffer + 30, tempValue, k_strlen(tempValue));

		// set start cluster index to buffer.
		k_sprintf(tempValue, "0x%X cluster", entry->startClusterIndex);
		k_memcpy(buffer + 55, tempValue, k_strlen(tempValue));

		// print file list.
		k_printf("    %s\n", buffer);

		// ask a user to print more items, every after 15 items are printed.
		if ((count != 0) && ((count % 15) == 0)) {

			k_printf("Press any key to continue...('q' is exit):");

			if (k_getch() == 'q') {
				k_printf("\n");
				break;
			}

			k_printf("\n");
		}

		count++;
	}

	// print total file count, total file size, free space of hard disk.
	k_printf("\t\tTotal File Count : %d\n", totalCount);
	k_printf("\t\tTotal File Size  : %d byte (%d cluster)\n", totalByte, usedClusterCount);
	k_printf("\t\tFree Space       : %d KB (%d cluster)\n", (manager.totalClusterCount - usedClusterCount) * FS_CLUSTERSIZE / 1024, manager.totalClusterCount - usedClusterCount);

	// close root directory.
	closedir(dir);
}

static void k_writeDataToFile(const char* paramBuffer) {
	ParamList list;
	char fileName[50];
	int len;
	File* file;
	int enterCount;
	byte key;

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: FileName
	if ((len = k_getNextParam(&list, fileName)) == 0) {
		k_printf("Wrong Usage, ex) writefile a.txt\n");
		return;
	}

	fileName[len] = '\0';

	if (len > (FS_MAXFILENAMELENGTH - 1)) {
		k_printf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// open a file.
	file = fopen(fileName, "w");
	if (file == null) {
		k_printf("'%s' File Open Fail~!!\n", fileName);
		return;
	}

	// loop for writing a file.
	enterCount = 0;
	while (true) {
		key = k_getch();

		// press Enter key 3 times continuously to finish writing a file.
		if (key == KEY_ENTER) {
			enterCount++;
			if (enterCount >= 3) {
				break;
			}
		} else {
			enterCount = 0;
		}

		k_printf("%c", key);

		// write a file.
		if (fwrite(&key, 1, 1, file) != 1) {
			k_printf("'%s' File Write Fail~!!\n", fileName);
			break;
		}
	}

	// close a file.
	fclose(file);
}

static void k_readDataFromFile(const char* paramBuffer) {
	ParamList list;
	char fileName[50];
	int len;
	File* file;
	int enterCount;
	byte key;

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: FileName
	if ((len = k_getNextParam(&list, fileName)) == 0) {
		k_printf("Wrong Usage, ex) readfile a.txt\n");
		return;
	}

	fileName[len] = '\0';

	if (len > (FS_MAXFILENAMELENGTH - 1)) {
		k_printf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// open a file.
	file = fopen(fileName, "r");
	if (file == null) {
		k_printf("'%s' File Open Fail~!!\n", fileName);
		return;
	}

	// loop for reading a file.
	enterCount = 0;
	while (true) {
		// read a file.
		if (fread(&key, 1, 1, file) != 1) {
			break;
		}

		k_printf("%c", key);

		// increase count if Enter key is pressed.
		if (key == KEY_ENTER) {
			enterCount++;

			// ask a user to print more lines, every after 15 lines are printed.
			if ((enterCount != 0) && ((enterCount % 15) == 0)) {

				k_printf("Press any key to continue...('q' is exit):");

				if (k_getch() == 'q') {
					k_printf("\n");
					break;
				}

				k_printf("\n");
				enterCount = 0;
			}
		}
	}

	// close a file.
	fclose(file);
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

	k_printf("====>>>> File I/O Test Start\n");

	// allocate 4MB-sized memory.
	maxFileSize = 4 * 1024 * 1024;
	buffer = (byte*)k_allocMem(maxFileSize);
	if (buffer == null) {
		k_printf("Memory Allocation Fail~!!\n");
		return;
	}

	// remove the test file.
	remove("testfileio.bin");

	//----------------------------------------------------------------------------------------------------
	// 1. File Open Fail Test
	//----------------------------------------------------------------------------------------------------
	k_printf("1. File Open Fail Test...");

	// Read mode (r) dosen't create a file and return null when the file dosen't exist.
	file = fopen("testfileio.bin", "r");
	if (file == null) {
		k_printf("[Pass]\n");

	} else {
		k_printf("[Fail]\n");
		fclose(file);
	}

	//----------------------------------------------------------------------------------------------------
	// 2. File Create Test
	//----------------------------------------------------------------------------------------------------
	k_printf("2. File Create Test...");

	// Write mode (w) create a file and return a file handle when the file dosen't exist.
	file = fopen("testfileio.bin", "w");
	if (file != null) {
		k_printf("[Pass]\n");
		k_printf("    FileHandle=[0x%Q]\n", file);

	} else {
		k_printf("[Fail]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 3. Sequential Write Test
	//----------------------------------------------------------------------------------------------------
	k_printf("3. Sequential Write Test(Cluster Size)...");

	// write data to a opened file.
	for (i = 0; i < 100; i++) {

		k_memset(buffer, i, FS_CLUSTERSIZE);

		// write a file.
		if (fwrite(buffer, 1, FS_CLUSTERSIZE, file) != FS_CLUSTERSIZE) {
			k_printf("[Fail]\n");
			k_printf("    %d Cluster Error\n", i);
			break;
		}
	}

	if (i >= 100) {
		k_printf("[Pass]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 4. Sequential Read and Verify Test
	//----------------------------------------------------------------------------------------------------
	k_printf("4. Sequential Read and Verify Test(Cluster Size)...");

	// move to the start of file.
	fseek(file, -100 * FS_CLUSTERSIZE, SEEK_END);

	// read data from opened file, and verify data.
	for (i = 0; i < 100; i++) {

		// read a file.
		if (fread(buffer, 1, FS_CLUSTERSIZE, file) != FS_CLUSTERSIZE) {
			k_printf("[Fail]\n");
			break;
		}

		// verify data.
		for (j = 0; j < FS_CLUSTERSIZE; j++) {
			if (buffer[j] != (byte)i) {
				k_printf("[Fail]\n");
				k_printf("    %d Cluster Error, [0x%X] != [0x%X]", i, buffer[j], (byte)i);
				break;
			}
		}
	}

	if (i >= 100) {
		k_printf("[Pass]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 5. Random Write Test
	//----------------------------------------------------------------------------------------------------
	k_printf("5. Random Write Test\n");

	// put 0 to buffer.
	k_memset(buffer, 0, maxFileSize);

	// move to the start of file, and read data from file, and copy it to buffer.
	fseek(file, -100 * FS_CLUSTERSIZE, SEEK_CUR);
	fread(buffer, 1, maxFileSize, file);

	// write the same data to both file and buffer.
	for (i = 0; i < 100; i++) {
		byteCount = (k_random() % (sizeof(tempBuffer) - 1)) + 1;
		randomOffset = k_random() % (maxFileSize - byteCount);
		k_printf("    [%d] Offset=[%d], Byte=[%d]...", i, randomOffset, byteCount);

		// write data in the random position of file.
		fseek(file, randomOffset, SEEK_SET);
		k_memset(tempBuffer, i, byteCount);
		if (fwrite(tempBuffer, 1, byteCount, file) != byteCount) {
			k_printf("[Fail]\n");
			break;

		} else {
			k_printf("[Pass]\n");
		}

		// write data in the random position of buffer.
		k_memset(buffer + randomOffset, i, byteCount);
	}

	// move to the end of file and buffer, and write data (1 byte), and make the size 4MB.
	fseek(file, maxFileSize - 1, SEEK_SET);
	fwrite(&i, 1, 1, file);
	buffer[maxFileSize - 1] = (byte)i;

	//----------------------------------------------------------------------------------------------------
	// 6. Random Read and Verify Test
	//----------------------------------------------------------------------------------------------------
	k_printf("6. Random Read and Verify Test\n");

	// read data from the random position of file and buffer, verify data.
	for (i = 0; i < 100; i++) {
		byteCount = (k_random() % (sizeof(tempBuffer) - 1)) + 1;
		randomOffset = k_random() % (maxFileSize - byteCount);
		k_printf("    [%d] Offset=[%d], Byte=[%d]...", i, randomOffset, byteCount);

		// read data from the random position of file.
		fseek(file, randomOffset, SEEK_SET);
		if (fread(tempBuffer, 1, byteCount, file) != byteCount) {
			k_printf("[Fail]\n");
			k_printf("    File Read Fail\n");
			break;
		}

		// verify data.
		if (k_memcmp(buffer + randomOffset, tempBuffer, byteCount) != 0) {
			k_printf("[Fail]\n");
			k_printf("    Data Verify Fail\n");
			break;
		}

		k_printf("[Pass]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 7. Sequential Write, Read and Verify Test(1024 bytes)
	//----------------------------------------------------------------------------------------------------
	k_printf("7. Sequential Write, Read and Verify Test(1024 bytes)\n");

	// move to the start of file.
	fseek(file, -maxFileSize, SEEK_CUR);

	// loop for writing a file. (write data (2MB) from the start of file).
	for (i = 0; i < (2 * 1024 * 1024 / 1024); i++) {
		k_printf("    [%d] Offset=[%d], Byte=[%d] Write...", i, i * 1024, 1024);

		// write a file (write data by 1024 bytes).
		if (fwrite(buffer + (i * 1024), 1, 1024, file) != 1024) {
			k_printf("[Fail]\n");
			break;

		} else {
			k_printf("[Pass]\n");
		}
	}

	// move to the start of file.
	fseek(file, -maxFileSize, SEEK_SET);

	// read data from a file, and verify it.
	// verify the whole area (4MB), because wrong data could have been saved by the random write.
	for (i = 0; i < (maxFileSize / 1024); i++) {
		k_printf("    [%d] Offset=[%d], Byte=[%d] Read and Verify...", i, i * 1024, 1024);

		// read a file. (read data by 1024 bytes.)
		if (fread(tempBuffer, 1, 1024, file) != 1024) {
			k_printf("[Fail]\n");
			break;
		}

		// verify data.
		if (k_memcmp(buffer + (i * 1024), tempBuffer, 1024) != 0) {
			k_printf("[Fail]\n");
			break;

		} else {
			k_printf("[Pass]\n");
		}
	}

	//----------------------------------------------------------------------------------------------------
	// 8. File Delete Fail Test
	//----------------------------------------------------------------------------------------------------
	k_printf("8. File Delete Fail Test...");

	// the file delete test must fails, because the file is open.
	if (remove("testfileio.bin") != 0) {
		k_printf("[Pass]\n");

	} else {
		k_printf("[Fail]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 9. File Close Fail Test
	//----------------------------------------------------------------------------------------------------
	k_printf("9. File Close Fail Test...");

	// close a file.
	if (fclose(file) == 0) {
		k_printf("[Pass]\n");

	} else {
		k_printf("[Fail]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 10. File Delete Test
	//----------------------------------------------------------------------------------------------------
	k_printf("10. File Delete Test...");

	// remove a file.
	if (remove("testfileio.bin") == 0) {
		k_printf("[Pass]\n");

	} else {
		k_printf("[Fail]\n");
	}

	// free memory.
	k_freeMem(buffer);

	k_printf("====>>>> File I/O Test End\n");
}

static void k_testPerformance(const char* paramBuffer) {
	File* file;
	dword clusterTestFileSize;
	dword oneByteTestFileSize;
	qword lastTickCount;
	dword i;
	byte* buffer;

	// set file size (cluster unit: 1MB, byte unit: 16KB)
	clusterTestFileSize = 1024 * 1024;
	oneByteTestFileSize = 16 * 1024;

	// allocate memory.
	buffer = (byte*)k_allocMem(clusterTestFileSize);
	if (buffer == null) {
		k_printf("Memory Allocate Fail~!!\n");
		return;
	}

	k_memset(buffer, 0, FS_CLUSTERSIZE);

	k_printf("====>>>> File I/O Performance Test Start\n");

	//----------------------------------------------------------------------------------------------------
	// 1-1. Sequential Write Test by Cluster Unit
	//----------------------------------------------------------------------------------------------------
	k_printf("1. Sequential Read/Write Test (Cluster -> 1MB)\n");

	// remove a file, and create it.
	remove("performance.txt");
	file = fopen("performance.txt", "w");
	if (file == null) {
		k_printf("File Open Fail~!!\n");
		k_freeMem(buffer);
		return;
	}

	lastTickCount = k_getTickCount();

	// write test by cluster unit.
	for (i = 0; i < (clusterTestFileSize / FS_CLUSTERSIZE); i++) {
		if (fwrite(buffer, 1, FS_CLUSTERSIZE, file) != FS_CLUSTERSIZE) {
			k_printf("File Write Fail~!!\n");
			fclose(file);
			remove("performance.txt");
			k_freeMem(buffer);
			return;
		}
	}

	// print test time.
	k_printf("    - Write : %d ms\n", k_getTickCount() - lastTickCount);

	//----------------------------------------------------------------------------------------------------
	// 1-2. Sequential Read Test by Cluster Unit
	//----------------------------------------------------------------------------------------------------

	// move to the start of file.
	fseek(file, 0, SEEK_SET);

	lastTickCount = k_getTickCount();

	// read test by cluster unit.
	for (i = 0; i < (clusterTestFileSize / FS_CLUSTERSIZE); i++) {
		if (fread(buffer, 1, FS_CLUSTERSIZE, file) != FS_CLUSTERSIZE) {
			k_printf("File Read Fail~!!\n");
			fclose(file);
			remove("performance.txt");
			k_freeMem(buffer);
			return;
		}
	}

	// print test time.
	k_printf("    - Read  : %d ms\n", k_getTickCount() - lastTickCount);

	//----------------------------------------------------------------------------------------------------
	// 2-1. Sequential Write Test by Byte Unit
	//----------------------------------------------------------------------------------------------------
	k_printf("2. Sequential Read/Write Test (Byte -> 16KB)\n");

	// remove a file and create it.
	fclose(file);
	remove("performance.txt");
	file = fopen("performance.txt", "w");
	if (file == null) {
		k_printf("File Open Fail~!!\n");
		k_freeMem(buffer);
		return;
	}

	lastTickCount = k_getTickCount();

	// write test by byte unit.
	for (i = 0; i < oneByteTestFileSize; i++) {
		if (fwrite(buffer, 1, 1, file) != 1) {
			k_printf("File Write Fail~!!\n");
			fclose(file);
			remove("performance.txt");
			k_freeMem(buffer);
			return;
		}
	}

	// print test time.
	k_printf("    - Write : %d ms\n", k_getTickCount() - lastTickCount);

	//----------------------------------------------------------------------------------------------------
	// 2-2. Sequential Read Test by Byte Unit
	//----------------------------------------------------------------------------------------------------

	// move to the start of file.
	fseek(file, 0, SEEK_SET);

	lastTickCount = k_getTickCount();

	// read test by byte unit.
	for (i = 0; i < oneByteTestFileSize; i++) {
		if (fread(buffer, 1, 1, file) != 1) {
			k_printf("File Read Fail~!!\n");
			fclose(file);
			remove("performance.txt");
			k_freeMem(buffer);
			return;
		}
	}

	// print test time.
	k_printf("    - Read  : %d ms\n", k_getTickCount() - lastTickCount);

	// close a file, and delete it, and free memory.
	fclose(file);
	remove("performance.txt");
	k_freeMem(buffer);

	k_printf("====>>>> File I/O Performance Test End\n");
}

static void k_flushCache(const char* paramBuffer) {
	qword tickCount;

	tickCount = k_getTickCount();

	k_printf("Flush File System Cache...");
	if (k_flushFileSystemCache() == true) {
		k_printf("Success~!!\n");

	} else {
		k_printf("Fail~!!\n");
	}

	k_printf("Flush Time = %d ms\n", k_getTickCount() - tickCount);
}

static void k_downloadFile(const char* paramBuffer) {
	ParamList list;
	char fileName[50];
	int fileNameLen;
	dword dataLen;
	File* file;
	dword receivedSize;
	dword tempSize;
	byte dataBuffer[SERIAL_FIFOMAXSIZE];
	qword lastReceivedTickCount;

	// initialize parameter.
	k_initParam(&list, paramBuffer);

	// get No.1 parameter: FileName
	if ((fileNameLen = k_getNextParam(&list, fileName)) == 0) {
		k_printf("Wrong Usage, ex) download a.txt\n");
		return;
	}

	fileName[fileNameLen] = '\0';

	if (fileNameLen > (FS_MAXFILENAMELENGTH - 1)) {
		k_printf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// clear send/receive FIFO.
	k_clearSerialFifo();

	//----------------------------------------------------------------------------------------------------
	// receive data length (4 bytes), and send ACK
	//----------------------------------------------------------------------------------------------------
	k_printf("Waiting for Data Length...");
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
				k_printf("Time Out~!!\n");
				return;
			}

		} else {
			lastReceivedTickCount = k_getTickCount();
		}
	}

	k_printf("[%d] byte\n", dataLen);

	// received data length successfully, so send ACK.
	k_sendSerialData("A", 1);

	//----------------------------------------------------------------------------------------------------
	// save data from serial port to file.
	//----------------------------------------------------------------------------------------------------

	// create a file.
	file = fopen(fileName, "w");
	if (file == null) {
		k_printf("'%s' File Open Fail~!!\n", fileName);
		return;
	}

	k_printf("Data Receive Start...");
	receivedSize = 0;
	lastReceivedTickCount = k_getTickCount();

	// loop for receiving data.
	while (receivedSize < dataLen) {
		// receive data.
		tempSize = k_recvSerialData(dataBuffer, SERIAL_FIFOMAXSIZE);
		receivedSize += tempSize;

		// If the receive data exists, send ACK and write a file.
		if (tempSize != 0) {

			// send ACK every when it reaches FIFO max size, and when receiving completes.
			if (((receivedSize % SERIAL_FIFOMAXSIZE) == 0) || (receivedSize == dataLen)) {
				k_sendSerialData("A", 1);
				k_printf("#");
			}

			// write a file.
			if (fwrite(dataBuffer, 1, tempSize, file) !=  tempSize) {
				k_printf("'%s' File Write Fail~!!\n", fileName);
				break;
			}

			lastReceivedTickCount = k_getTickCount();

		// If the receive data dosen't exist, wait for a while.
		} else {
			k_sleep(0);

			// break if the wait time exceeds 10 seconds.
			if ((k_getTickCount() - lastReceivedTickCount) > 10000) {
				k_printf("Time Out~!!\n");
				break;
			}
		}
	}

	//----------------------------------------------------------------------------------------------------
	// check if data is received successfully, and close a file, and flush file system cache
	//----------------------------------------------------------------------------------------------------

	// check if data is received successfully
	if (receivedSize != dataLen) {
		k_printf("\nReceive Fail~!! Received Size = [%d] byte\n", receivedSize);

	} else {
		k_printf("\nReceive Complete~!! Received Size = [%d] byte\n", receivedSize);
	}

	// close a file, and flush file system cache
	fclose(file);
	k_flushFileSystemCache();
}

static void k_showMpConfigTable(const char* paramBuffer) {
	k_printMpConfigTable();
}

static void k_startAp(const char* paramBuffer) {
	if (k_startupAp() == false) {
		k_printf("====>>>> Application Processor Start Fail~!!\n");
		return;
	}
	
	k_printf("====>>>> Application Processor Start Success~!!\n");
	k_printf("-> Bootstrap Processor(APIC ID:%d) started Application Processor.\n", k_getApicId());
}

static void k_startSymmetricIoMode(const char* paramBuffer) {
	MpConfigManager* mpManager;
	bool interruptFlag;
	
	if (k_analyzeMpConfigTable() == false) {
		k_printf("fail to analyze MP configuration table\n");
		return;
	}
	
	mpManager = k_getMpConfigManager();
	
	// If it's PIC mode, disable PIC mode using IMCR Register.
	if (mpManager->usePicMode == true) {
		k_printf("disable PIC mode\n");
		k_outPortByte(0x22, 0x70);
		k_outPortByte(0x23, 0x01);
	}
	
	k_printf("mask PIC interrupt\n");
	k_maskPicInterrupt(0xFFFF);
	
	k_printf("enable global Local APIC\n");
	k_enableGlobalLocalApic();
	
	k_printf("enable software Local APIC\n");
	k_enableSoftwareLocalApic();
	
	k_printf("disable CPU interrupt flag\n");
	interruptFlag = k_setInterruptFlag(false);
	
	k_printf("set 0 priority to Task Priority Register\n");
	k_setInterruptPriority(0);
	
	k_printf("initialize Local Vector Table\n");
	k_initLocalVectorTable();
	
	k_printf("initialize IO Redirection Table\n");
	k_initIoRedirectionTable();
	
	k_printf("restore CPU interrupt flag\n");
	k_setInterruptFlag(interruptFlag);
	
	k_printf("succeed to start symmetric IO mode\n");
}

static void k_showIrqToIntinMap(const char* paramBuffer) {
	k_printIrqToIntinMap();
}

