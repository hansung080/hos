#ifndef __MODESWITCH_H__
#define __MODESWITCH_H__

#include "types.h"

void k_readCpuid(dword eax_, dword* eax, dword* ebx, dword* ecx, dword* edx);
void k_switchToKernel64(void);

#endif // __MODESWITCH_H__