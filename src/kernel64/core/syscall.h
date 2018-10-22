#ifndef __CORE_SYSCALL_H__
#define __CORE_SYSCALL_H__

#include "types.h"
#include "syscall_numbers.h"

#define SC_MAXPARAMCOUNT 10

/* Macro Function */
#define PARAM(x) (paramTable->values[(x)])

#pragma pack(push, 1)

typedef struct k_ParamTable {
	qword values[SC_MAXPARAMCOUNT];
} ParamTable;

#pragma pack(pop)

/*** Functions defined in syscall_asm.asm ***/
void k_syscallEntryPoint(qword syscallNumber, const ParamTable* paramTable);
void k_syscallTestTask(void);

/*** Functions defined in syscall.c ***/
void k_initSyscall(void);
qword k_processSyscall(qword syscallNumber, const ParamTable* paramTable);

#endif // __CORE_SYSCALL_H__