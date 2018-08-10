#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdarg.h>
#include "types.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

void k_memset(void* dest, byte data, int size);
int k_memcpy(void* dest, const void* src, int size);
int k_memcmp(const void* dest, const void* src, int size);
bool k_equalStr(const char* str1, const char* str2);
bool k_setInterruptFlag(bool interruptFlag);
int k_strlen(const char* buffer);
void k_checkTotalRamSize(void);
qword k_getTotalRamSize(void);
void k_reverseStr(char* buffer);
long k_atoi(const char* buffer, int base);
qword k_hexStrToQword(const char* buffer);
long k_decimalStrToLong(const char* buffer);
int k_itoa(long value, char* buffer, int base);
int k_hexToStr(qword value, char* buffer);
int k_decimalToStr(long value, char* buffer);
int k_sprintf(char* buffer, const char* format, ...);
int k_vsprintf(char* buffer, const char* format, va_list ap);
qword k_getTickCount(void);
void k_sleep(qword millisecond);

extern volatile qword g_tickCount;

#endif // __UTIL_H__
