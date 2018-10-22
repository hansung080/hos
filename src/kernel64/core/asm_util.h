#ifndef __CORE_ASMUTIL_H__
#define __CORE_ASMUTIL_H__

#include "types.h"
#include "task.h"

byte k_inPortByte(word port);
void k_outPortByte(word port, byte data);
word k_inPortWord(word port);
void k_outPortWord(word port, word data);
void k_loadGdt(qword gdtrAddr);
void k_loadTss(word tssOffset);
void k_loadIdt(qword idtrAddr);
void k_enableInterrupt(void);
void k_disableInterrupt(void);
qword k_readRflags(void);
qword k_readTsc(void);
void k_switchContext(Context* currentContext, Context* nextContext);
void k_halt(void);
void k_pause(void);
bool k_testAndSet(volatile byte* dest, byte cmp, byte src);
void k_initFpu(void);
void k_saveFpuContext(void* fpuContext);
void k_loadFpuContext(void* fpuContext);
void k_setTs(void);
void k_clearTs(void);
void k_enableGlobalLocalApic(void);
void k_readMsr(qword addr, qword* high32bits, qword* low32bits);
void k_writeMsr(qword addr, qword high32bits, qword low32bits);

#endif // __CORE_ASMUTIL_H__
