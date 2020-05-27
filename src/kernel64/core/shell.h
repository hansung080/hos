#ifndef __CORE_SHELL_H__
#define __CORE_SHELL_H__

#include "types.h"

#define SHELL_MAXCOMMANDBUFFERCOUNT 300
#define SHELL_PROMPTMESSAGE         "hsh$ "

// parameter-related macros
#define SHELL_MAXPARAMETERLENGTH           30 // It's including the last null character, so the max parameter length user can input is 29.
#define SHELL_ERROR_TOOLONGPARAMETERLENGTH -1 // too long parameter length error

typedef void (*CommandFunc)(const char* paramBuffer);

#pragma pack(push, 1)

typedef struct k_ShellCommandEntry {
	char* command;
	char* desc;
	CommandFunc func;
} ShellCommandEntry;

typedef struct k_ParamList {
	const char* buffer; // parameter buffer
	int len;            // parameter buffer length
	int currentIndex;   // current index in parameter buffer
} ParamList;

#pragma pack(pop)

// Shell Functions
void k_shellTask(void);
void k_executeCommand(const char* commandBuffer);
void k_initParam(ParamList* list, const char* paramBuffer);
int k_getNextParam(ParamList* list, char* param);

// Command Functions
static void k_help(const char* paramBuffer);
static void k_clear(const char* paramBuffer);
static void k_showTotalRamSize(const char* paramBuffer);
static void k_reboot(const char* paramBuffer);
static void k_measureCpuSpeed(const char* paramBuffer);
static void k_showDateAndTime(const char* paramBuffer);
static void k_changePriority(const char* paramBuffer);
static void k_showTaskStatus(const char* paramBuffer);
static void k_killTask(const char* paramBuffer);
static void k_showCpuLoad(const char* paramBuffer);
static void k_showMatrix(const char* paramBuffer);
static void k_matrixProcess(void);
static void k_charDropThread(void);
static void k_showDynamicMemInfo(const char* paramBuffer);
static void k_showHddInfo(const char* paramBuffer);
static void k_format(const char* paramBuffer);
static void k_mount(const char* paramBuffer);
static void k_showFileSystemInfo(const char* paramBuffer);
static void k_showRootDir(const char* paramBuffer);
static void k_createFileInRootDir(const char* paramBuffer);
static void k_deleteFileInRootDir(const char* paramBuffer);
static void k_writeDataToFile(const char* paramBuffer);
static void k_readDataFromFile(const char* paramBuffer);
static void k_flushCache(const char* paramBuffer);
static void k_downloadFile(const char* paramBuffer);
static void k_showMpConfigTable(const char* paramBuffer);
static void k_showIrqToIntinMap(const char* paramBuffer);
static void k_showInterruptCounts(const char* paramBuffer);
static void k_changeAffinity(const char* paramBuffer);
static void k_showVbeModeInfo(const char* paramBuffer);
static void k_runApp(const char* paramBuffer);
static void k_install(const char* paramBuffer);
static void k_uninstall(const char* paramBuffer);
static void k_exitShell(const char* paramBuffer);
#if __DEBUG__
static void k_testStrToDecimalHex(const char* paramBuffer);
static void k_setTimer(const char* paramBuffer);
static void k_waitUsingPit(const char* paramBuffer);
static void k_readTimeStampCounter(const char* paramBuffer);
static void k_createTestTask(const char* paramBuffer);
static void k_testTask1(void);
static void k_testTask2(void);
static void k_testTask3(void);
static void k_testMutex(const char* paramBuffer);
static void k_numberPrintTask(void);
static void k_testThread(const char* paramBuffer);
static void k_threadCreationTask(void);
static void k_testPi(const char* paramBuffer);
static void k_fpuTestTask(void);
static void k_testDynamicMem(const char* paramBuffer);
static void k_testSeqAlloc(void);
static void k_testRandomAlloc(void);
static void k_randomAllocTask(void);
static void k_writeSector(const char* paramBuffer);
static void k_readSector(const char* paramBuffer);
static void k_testFileIo(const char* paramBuffer);
static void k_testPerformance(const char* paramBuffer);
static void k_startAp(const char* paramBuffer);
static void k_startSymmetricIoMode(const char* paramBuffer);
static void k_startInterruptLoadBalancing(const char* paramBuffer);
static void k_startTaskLoadBalancing(const char* paramBuffer);
static void k_startMultiprocessorMode(const char* paramBuffer);
static void k_testScreenUpdatePerformance(const char* paramBuffer);
static void k_testSyscall(const char* paramBuffer);
static void k_testWaitTask(const char* paramBuffer);
static void k_testBlockingQueue(void);
static void k_blockingTask(void);
static void k_testNonblockingQueue(void);
static void k_nonblockingTask(void);
#endif // __DEBUG__

#endif // __CORE_SHELL_H__