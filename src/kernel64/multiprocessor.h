#ifndef __MULTIPROCESSOR_H__
#define __MULTIPROCESSOR_H__

#include "Types.h"

// BSP flag
#define BSP_FLAG     *(byte*)0x7C09 // BSP flag (1:BSP, 0:AP)
#define BSP_FLAG_BSP 0x01           // BSP
#define BSP_FLAG_AP  0x00           // AP

// max processor count
#define MAXPROCESSORCOUNT 16

bool k_startupAp(void);
byte k_getApicId(void);
static bool k_wakeupAp(void);

#endif // __MULTIPROCESSOR_H__
