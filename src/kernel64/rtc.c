#include "rtc.h"
#include "asm_util.h"

void k_readRtcTime(byte* hour, byte* minute, byte* second) {
	byte data;

	// set Hours Register to CMOS Memory Address Register, read current hour from CMOS Memory Data Register.
	k_outPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_HOUR);
	data = k_inPortByte(RTC_CMOSDATA);
	*hour = RTC_BCDTOBINARY(data);

	// set Minutes Register to CMOS Memory Address Register, read current minute from CMOS Memory Data Register.
	k_outPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MINUTE);
	data = k_inPortByte(RTC_CMOSDATA);
	*minute = RTC_BCDTOBINARY(data);

	// set Seconds Register to CMOS Memory Address Register, read current second from CMOS Memory Data Register.
	k_outPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_SECOND);
	data = k_inPortByte(RTC_CMOSDATA);
	*second = RTC_BCDTOBINARY(data);
}

void k_readRtcDate(word* year, byte* month, byte* dayOfMonth, byte* dayOfWeek) {
	byte data;

	// set Year Register to CMOS Memory Address Register, read current year from CMOS Memory Data Register.
	k_outPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_YEAR);
	data = k_inPortByte(RTC_CMOSDATA);
	*year = RTC_BCDTOBINARY(data) + 2000;

	// set Month Register to CMOS Memory Address Register, read current month from CMOS Memory Data Register.
	k_outPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MONTH);
	data = k_inPortByte(RTC_CMOSDATA);
	*month = RTC_BCDTOBINARY(data);

	// set Day Of Month Register to CMOS Memory Address Register, read current day-of-month from CMOS Memory Data Register.
	k_outPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFMONTH);
	data = k_inPortByte(RTC_CMOSDATA);
	*dayOfMonth = RTC_BCDTOBINARY(data);

	// set Day Of Week Register to CMOS Memory Address Register, read current day-of-week from CMOS Memory Data Register.
	k_outPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFWEEK);
	data = k_inPortByte(RTC_CMOSDATA);
	*dayOfWeek = RTC_BCDTOBINARY(data);
}

char* k_convertDayOfWeekToStr(byte dayOfWeek) {
	// dayOfWeek = [1~7: Sunday-Monday-Tuesday-Wednesday-Thursday-Friday-Saturday]
	static char* vpcDayOfWeekStr[8] = {"Error", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

	if(dayOfWeek >= 8){
		return vpcDayOfWeekStr[0];
	}

	return vpcDayOfWeekStr[dayOfWeek];
}
