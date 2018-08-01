#ifndef __MULTIPROCESSOR_H__
#define __MULTIPROCESSOR_H__

#include "types.h"

// BSP flag
#define BSPFLAG     *(byte*)0x7C09 // BSP flag (1:BSP, 0:AP)
#define BSPFLAG_BSP 0x01           // BSP
#define BSPFLAG_AP  0x00           // AP

// max processor count
#define MAXPROCESSORCOUNT 16

// APIC ID
#define APICID_BSP       0x00 // BSP APIC ID
#define APICID_BROADCAST 0xFF // broadcast
#define APICID_INVALID   0xFF // invalid APIC ID

bool k_startupAp(void);
byte k_getApicId(void);
static bool k_wakeupAp(void);

#endif // __MULTIPROCESSOR_H__
