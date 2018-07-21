#ifndef __SHELL_H__
#define __SHELL_H__

#include "types.h"

#define SHELL_MAXCOMMANDBUFFERCOUNT 300
#define SHELL_PROMPTMESSAGE         "hans$"

typedef void (*CommandFunc)(const char* pcParam);

#pragma pack(push, 1)

typedef struct k_ShellCommandEntry {
	char* command;
	char* help;
	CommandFunc func;
} ShellCommandEntry;

typedef struct k_ParamList {
	const char* buffer; // parameter list buffer
	int len;            // parameter list buffer length
	int currentPos;     // current parameter position.
} ParamList;

#pragma pack(pop)

// Shell Functions
void k_startShell(void);
void k_executeCommand(const char* commandBuffer);
void k_initParam(ParamList* list, const char* paramBuffer);
int k_getNextParam(ParamList* list, char* param);

// Command Functions
static void k_help(const char* paramBuffer);
static void k_cls(const char* paramBuffer);
static void k_showTotalRamSize(const char* paramBuffer);
static void k_strToDecimalHexTest(const char* paramBuffer);
static void k_shutdown(const char* paramBuffer);
static void k_setTimer(const char* paramBuffer);
static void k_waitUsingPit(const char* paramBuffer);
static void k_readTimeStampCounter(const char* paramBuffer);
static void k_measureProcessorSpeed(const char* paramBuffer);
static void k_showDateAndTime(const char* paramBuffer);
static void k_testTask1(void);
static void k_testTask2(void);
static void k_createTestTask(const char* paramBuffer);
static void k_changeTaskPriority(const char* paramBuffer);
static void k_showTaskList(const char* paramBuffer);
static void k_killTask(const char* paramBuffer);
static void k_cpuLoad(const char* paramBuffer);
static void k_printNumberTask(const char* paramBuffer);
static void k_testMutex(const char* paramBuffer);
static void k_createThreadTask(void);
static void k_testThread(const char* paramBuffer);
qword k_random(void);
static void k_dropCharThread(void);
static void k_matrixProcess(void);
static void k_showMatrix(const char* paramBuffer);
static void k_fpuTestTask(void);
static void k_testPi(const char* paramBuffer);
static void k_showDynamicMemInfo(const char* paramBuffer);
static void k_testSeqAlloc(const char* paramBuffer);
static void k_randomAllocTask(void);
static void k_testRandomAlloc(const char* paramBuffer);
static void k_showHddInfo(const char* paramBuffer);
static void k_readSector(const char* paramBuffer);
static void k_writeSector(const char* paramBuffer);
static void k_mountHdd(const char* paramBuffer);
static void k_formatHdd(const char* paramBuffer);
static void k_showFileSystemInfo(const char* paramBuffer);
static void k_createFileInRootDir(const char* paramBuffer);
static void k_deleteFileInRootDir(const char* paramBuffer);
static void k_showRootDir(const char* paramBuffer);
static void k_writeDataToFile(const char* paramBuffer);
static void k_readDataFromFile(const char* paramBuffer);
static void k_testFileIo(const char* paramBuffer);
static void k_testPerformance(const char* paramBuffer);
static void k_flushCache(const char* paramBuffer);
static void k_downloadFile(const char* paramBuffer);
static void k_showMpConfigTable(const char* paramBuffer);
static void k_startAp(const char* paramBuffer);
static void k_startSymmetricIoMode(const char* paramBuffer);
static void k_showIrqToIntinMap(const char* paramBuffer);

#endif // __SHELL_H__
