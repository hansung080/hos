#ifndef __MULTIPROCESSOR_H__
#define __MULTIPROCESSOR_H__

#include "types.h"

// BSP flag
#define BSPFLAG     *(byte*)0x7C09 // BSP flag (0: AP, 1: BSP): BSP_FLAG is defined in boot_loader.asm.
#define BSPFLAG_AP  0x00           // AP
#define BSPFLAG_BSP 0x01           // BSP

// max processor count
#define MAXPROCESSORCOUNT 16

// APIC ID
#define APICID_BSP       0x00 // BSP APIC ID
#define APICID_BROADCAST 0xFF // broadcast
#define APICID_INVALID   0xFF // invalid APIC ID

bool k_startupAp(void);
byte k_getApicId(void); // get Local APIC ID of current core. [REF] in HansOS, Local APIC ID == core index == scheduler index
static bool k_wakeupAp(void);

#endif // __MULTIPROCESSOR_H__
