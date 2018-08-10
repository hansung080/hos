#include "util.h"
#include "asm_util.h"

// interrupt-occurring count by Timer(IRQ 0, PIT Controller)
volatile qword g_tickCount = 0;

//====================================================================================================
// k_memset, k_memcpy, k_memcmp (by 1 byte)
//====================================================================================================
#if 0
void k_memset(void* dest, byte data, int size) {
	int i;
	
	for (i = 0; i < size; i++) {
		((char*)dest)[i] = data;
	}
}

int k_memcpy(void* dest, const void* src, int size) {
	int i;
	
	for (i = 0; i < size; i++) {
		((char*)dest)[i] = ((char*)src)[i];
	}
	
	return size;
}

int k_memcmp(const void* dest, const void* src, int size) {
	int i;
	char temp;
	
	for (i = 0; i < size; i++) {
		temp = ((char*)dest)[i] - ((char*)src)[i];
		if (temp != 0) {
			return (int)temp;
		}
	}
	
	return 0;
}
#endif

//====================================================================================================
// k_memset, k_memcpy, k_memcmp (by 8 bytes, because general register size is 8 bytes in 64-bit mode)
//====================================================================================================
void k_memset(void* dest, byte data, int size) {
	int i;
	qword qwdata;
	int remainByteStartOffset;
	
	// make 8 bytes-sized data.
	qwdata = 0;
	for (i = 0; i < 8; i++) {
		qwdata = (qwdata << 8) | data;
	}
	
	// set memory by 8 bytes.
	for (i = 0; i < (size / 8); i++) {
		((qword*)dest)[i] = qwdata;
	}
	
	// set remained memory by 1 byte.
	remainByteStartOffset = i * 8;
	for (i = 0; i < (size % 8); i++) {
		((char*)dest)[remainByteStartOffset++] = data;
	}
}

int k_memcpy(void* dest, const void* src, int size) {
	int i;
	int remainByteStartOffset;
	
	// copy memory by 8 bytes.
	for (i = 0; i < (size / 8); i++) {
		((qword*)dest)[i] = ((qword*)src)[i];
	}
	
	// copy remained memory by 1 bytes.
	remainByteStartOffset = i * 8;
	for (i = 0; i < (size % 8); i++) {
		((char*)dest)[remainByteStartOffset] = ((char*)src)[remainByteStartOffset];
		remainByteStartOffset++;
	}
	
	// return copied memory size.
	return size;
}

int k_memcmp(const void* dest, const void* src, int size) {
	int i, j;
	int remainByteStartOffset;
	qword qwvalue;
	char cvalue;
	
	// compare memory by 8 bytes.
	for (i = 0; i < (size / 8); i++) {
		qwvalue = ((qword*)dest)[i] - ((qword*)src)[i];
		if (qwvalue != 0) {
			
			// compare memory by 1 byte in 8 bytes, and return the value which is not the same.
			for (j = 0; j < 8; j++) {
				if(((qwvalue >> (j * 8)) & 0xFF) != 0){
					return (qwvalue >> (j * 8)) & 0xFF;
				}
			}
		}
	}
	
	// compare remained memory by 1 byte.
	remainByteStartOffset = i * 8;
	for (i = 0; i < (size % 8); i++) {
		cvalue = ((char*)dest)[remainByteStartOffset] - ((char*)src)[remainByteStartOffset];
		if (cvalue != 0) {
			return cvalue;
		}
		
		remainByteStartOffset++;
	}
	
	// return 0 if values are the same.
	return 0;
}

bool k_equalStr(const char* str1, const char* str2) {
	int len1;
	int len2;
	
	len1 = k_strlen(str1);
	len2 = k_strlen(str2);
	
	if ((len1 == len2) && (k_memcmp(str1, str2, len1) == 0)) {
		return true;
	}
	
	return false;
}


bool k_setInterruptFlag(bool interruptFlag) {
	qword rflags;
	
	rflags = k_readRflags();
	
	if (interruptFlag == true) {
		k_enableInterrupt();
		
	} else {
		k_disableInterrupt();
	}
	
	// check IF(bit 9) of RFLAGS Register, and return previous interrupt flag.
	if (rflags & 0x0200) {
		return true;
	}
	
	return false;
}

int k_strlen(const char* buffer) {
	int i;
	
	for (i = 0; ; i++) {
		if (buffer[i] == '\0') {
			break;
		}
	}
	
	return i;
}

// total RAM size (MBytes)
static qword g_totalRamMbSize = 0;

void k_checkTotalRamSize(void) {
	dword* currentAddr;
	dword prevValue;
	
	// check 4 bytes by 4 MBytes from 64 MBytes.
	currentAddr = (dword*)0x4000000;
	
	while (true) {
		prevValue = *currentAddr;
		
		// check 4 bytes.
		*currentAddr = 0x12345678;
		if (*currentAddr != 0x12345678) {
			break;
		}
		
		*currentAddr = prevValue;
		
		// move 4 MBytes.
		currentAddr += (0x400000 / 4);
	}
	
	// calculate by MBytes.
	g_totalRamMbSize = (qword)currentAddr / 0x100000;
}

qword k_getTotalRamSize(void) {
	return g_totalRamMbSize;
}

long k_atoi(const char* buffer, int base) {
	long ret;
	
	switch (base) {
	case 16:
		ret = k_hexStrToQword(buffer);
		break;
		
	case 10:
	default:
		ret = k_decimalStrToLong(buffer);
		break;
	}
	
	return ret;
}

qword k_hexStrToQword(const char* buffer) {
	qword value = 0;
	int i;
	
	for (i = 0; buffer[i] != '\0'; i++) {
		value *= 16;
		
		if ('A' <= buffer[i] && buffer[i] <= 'Z') {
			value += (buffer[i] - 'A') + 10;
			
		} else if ('a' <= buffer[i] && buffer[i] <= 'z') {
			value += (buffer[i] - 'a') + 10;
			
		} else {
			value += (buffer[i] - '0');
		}
	}
	
	return value;
}

long k_decimalStrToLong(const char* buffer) {
	long value = 0;
	int i;
	
	if (buffer[0] == '-') {
		i = 1;
		
	} else {
		i = 0;
	}
	
	for ( ; buffer[i] != '\0'; i++) {
		value *= 10;
		value += (buffer[i] - '0');
	}
	
	if (buffer[0] == '-') {
		value = -value;
	}
	
	return value;
}

int k_itoa(long value, char* buffer, int base) {
	int ret;
	
	switch (base) {
	case 16:
		ret = k_hexToStr(value, buffer);
		break;
		
	case 10:
	default:
		ret = k_decimalToStr(value, buffer);
		break;
	}
	
	return ret;
}

int k_hexToStr(qword value, char* buffer) {
	qword i;
	qword currentValue;
	
	if (value == 0) {
		buffer[0] = '0';
		buffer[1] = '\0';
		return 1;
	}
	
	for (i = 0; value > 0; i++) {
		currentValue = value % 16;
		
		if (currentValue >= 10) {
			buffer[i] = (currentValue - 10) + 'A';
			
		} else {
			buffer[i] = currentValue + '0';
		}
		
		value /= 16;
	}
	
	buffer[i] = '\0';
	
	k_reverseStr(buffer);
	
	return i;
}

int k_decimalToStr(long value, char* buffer) {
	long i;
	
	if (value == 0) {
		buffer[0] = '0';
		buffer[1] = '\0';
		return 1;
	}
	
	if (value < 0) {
		i = 1;
		buffer[0] = '-';
		value = -value;
		
	} else {
		i = 0;
	}
	
	for ( ; value > 0; i++) {
		buffer[i] = (value % 10) + '0';
		value /= 10;
	}
	
	buffer[i] = '\0';
	
	if (buffer[0] == '-') {
		k_reverseStr(&(buffer[1]));
		
	} else {
		k_reverseStr(buffer);
	}
	
	return i;
}

void k_reverseStr(char* buffer) {
	int len;
	int i;
	char temp;
	
	len = k_strlen(buffer);
	
	for (i = 0; i < (len / 2); i++) {
		temp = buffer[i];
		buffer[i] = buffer[len - 1 - i];
		buffer[len - 1 - i] = temp;
	}
}

int k_sprintf(char* buffer, const char* format, ...) {
	va_list ap;
	int ret;
	
	va_start(ap, format);
	ret = k_vsprintf(buffer, format, ap);
	va_end(ap);
	
	// return length of printed string.
	return ret;
}

/**
  < Data Types Supported by k_vsprintf Function >
  - %s         : string
  - %c         : char
  - %d, %i     : int (decimal, signed)
  - %x, %X     : dword (hexadecimal, unsigned)
  - %q, %Q, %p : qword (hexadecimal, unsigned)
  - %f         : float (print down to the second position below decimal point by half-rounding up at the third position below decimal point.)
 */
int k_vsprintf(char* buffer, const char* format, va_list ap) {
	qword i, j, k;
	int index = 0; // buffer index
	int formatLen, copyLen;
	char* copyStr;
	qword qwvalue;
	int ivalue;
	double dvalue;
	
	formatLen = k_strlen(format);
	
	for (i = 0; i < formatLen; i++) {
		// If '%', it's data type.
		if (format[i] == '%') {
			i++;
			switch(format[i]){
			case 's':
				copyStr = (char*)(va_arg(ap, char*));
				copyLen = k_strlen(copyStr);
				k_memcpy(buffer + index, copyStr, copyLen);
				index += copyLen;
				break;
				
			case 'c':
				buffer[index] = (char)(va_arg(ap, int));
				index++;
				break;
				
			case 'd':
			case 'i':
				ivalue = (int)(va_arg(ap, int));
				index += k_itoa(ivalue, buffer + index, 10);
				break;
				
			case 'x':
			case 'X':
				qwvalue = (dword)(va_arg(ap, dword)) & 0xFFFFFFFF;
				index += k_itoa(qwvalue, buffer + index, 16);
				break;
				
			case 'q':
			case 'Q':
			case 'p':
				qwvalue = (qword)(va_arg(ap, qword));
				index += k_itoa(qwvalue, buffer + index, 16);
				break;
				
			case 'f':
				dvalue = (double)(va_arg(ap, double));
				
				// half-round up at the third position below decimal point.
				dvalue += 0.005;
				
				// save value to buffer sequentially from the second position below decimal point in decimal part.
				buffer[index] = '0' + ((qword)(dvalue * 100) % 10);
				buffer[index + 1] = '0' + ((qword)(dvalue * 10) % 10);
				buffer[index + 2] = '.';
				
				for (k = 0; ; k++) {
					// If integer part == 0, break.
					if (((qword)dvalue == 0) && (k != 0)) {
						break;
					}
					
					// save value to buffer sequentially from the units digit in integer part.
					buffer[index + 3 + k] = '0' + ((qword)dvalue % 10);
					dvalue = dvalue / 10;
				}
				
				buffer[index + 3 + k] = '\0';
				
				// reverse as many as float-saved length, increase buffer index.
				k_reverseStr(buffer + index);
				index += 3 + k;
				break;
				
			default:
				buffer[index] = format[i];
				index++;
				break;
			}
			
		// If not '%', it's normal character.
		} else {
			buffer[index] = format[i];
			index++;
		}
	}
	
	buffer[index] = '\0';
	
	// return length of printed string.
	return index;
}

qword k_getTickCount(void) {
	return g_tickCount;
}

void k_sleep(qword millisecond) {
	qword lastTickCount;
	
	lastTickCount = g_tickCount;
	
	while ((g_tickCount - lastTickCount) <= millisecond) {
		k_schedule();
	}
}
