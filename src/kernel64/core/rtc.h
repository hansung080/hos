#ifndef __CORE_RTC_H__
#define __CORE_RTC_H__

#include "types.h"

// RTC I/O port
#define RTC_CMOSADDRESS 0x70 // CMOS Memory Address Register
#define RTC_CMOSDATA    0x71 // CMOS Memory Data Register

// fields of CMOS Memory Address (1 byte)
#define RTC_ADDRESS_SECOND     0x00 // Seconds Register: register which saves second of current time.
#define RTC_ADDRESS_MINUTE     0x02 // Minutes Register: register which saves minute of current time.
#define RTC_ADDRESS_HOUR       0x04 // Hours Register: register which saves hour of current time.
#define RTC_ADDRESS_DAYOFWEEK  0x06 // Day Of Week Register: register which saves day-of-week of current date. (1~7: Sunday-Monday-Tuesday-Wednesday-Thursday-Friday-Saturday)
#define RTC_ADDRESS_DAYOFMONTH 0x07 // Day Of Month Register: register which saves day-of-month of current date.
#define RTC_ADDRESS_MONTH      0x08 // Month Register: register which saves month of current date.
#define RTC_ADDRESS_YEAR       0x09 // Year Register: register which saves year of current date.

// macro to convert from BCD format to Binary format
#define RTC_BCDTOBINARY(x) ((((x) >> 4) * 10) + ((x) & 0x0F))

void k_readRtcTime(byte* hour, byte* minute, byte* second);
void k_readRtcDate(word* year, byte* month, byte* dayOfMonth, byte* dayOfWeek);
char* k_convertDayOfWeekToStr(byte dayOfWeek);

#endif // __CORE_RTC_H__