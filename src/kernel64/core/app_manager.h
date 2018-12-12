#ifndef __CORE_APPMANAGER_H__
#define __CORE_APPMANAGER_H__

#include "types.h"
#include "../utils/elf64.h"
#include "task.h"

// max argument string length
#define APPMGR_MAXARGSLENGTH 1023

qword k_executeApp(const char* fileName, const char* args, byte affinity);
static bool k_loadSections(const byte* fileBuffer, qword* appMemAddr, qword* appMemSize, qword* entryPointAddr);
static bool k_relocateSections(const byte* fileBuffer);
static void k_addArgsToTask(Task* task, const char* args);
bool k_installApp(const char* fileName);
bool k_uninstallApp(const char* fileName);

#endif // __CORE_APPMANAGER_H__