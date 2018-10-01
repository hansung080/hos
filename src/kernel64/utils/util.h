#ifndef __UTILS_UTIL_H__
#define __UTILS_UTIL_H__

#include <stdarg.h>
#include "../core/types.h"

/* Macro Functions */
#define MIN(x, y)     (((x) < (y)) ? (x) : (y))
#define MAX(x, y)     (((x) > (y)) ? (x) : (y))
#define ABS(x)        (((x) >= 0) ? (x) : -(x))
#define SWAP(x, y, t) ((t) = (x)), ((x) = (y)), ((y) = (t))

/* Memory Functions */
void k_memset(void* dest, byte data, int size);
int k_memcpy(void* dest, const void* src, int size);
int k_memcmp(const void* dest, const void* src, int size);
void k_memsetWord(void* dest, word data, int wordSize);
void k_checkTotalRamSize(void);
qword k_getTotalRamSize(void);

/* String Functions */
int k_strlen(const char* str);
void k_reverseStr(char* str);
bool k_equalStr(const char* str1, const char* str2);
long k_atol10(const char* str);
qword k_atol16(const char* str);
int k_ltoa10(long value, char* str);
int k_ltoa16(qword value, char* str);
int k_sprintf(char* str, const char* format, ...);
int k_vsprintf(char* str, const char* format, va_list ap);

/* Time Functions */
qword k_getTickCount(void);
void k_sleep(qword millisecond);

/* Math Functions */
qword k_random(void);

/* ETC Functions */
bool k_setInterruptFlag(bool interruptFlag);
bool k_isGraphicMode(void);

extern volatile qword g_tickCount;

#endif // __UTILS_UTIL_H__