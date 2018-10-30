#ifndef __CORE_LOADER_H__
#define __CORE_LOADER_H__

#include "types.h"
#include "../utils/elf64.h"
#include "task.h"

// max argument string length
#define LOADER_MAXARGSLENGTH 1023

qword k_executeApp(const char* fileName, const char* args, byte affinity);
static bool k_loadApp(const byte* fileBuffer, qword* appMemAddr, qword* appMemSize, qword* entryPointAddr);
static bool k_relocateSection(const byte* fileBuffer);
static void k_addArgsToTask(Task* task, const char* args);

#endif // __CORE_LOADER_H__