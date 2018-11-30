#include "util.h"
#include "../core/asm_util.h"
#include "../core/vbe.h"
#include "../core/sync.h"

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
	int remainBytesOffset;
	
	// make 8 bytes-sized data.
	qwdata = 0;
	for (i = 0; i < 8; i++) {
		qwdata = (qwdata << 8) | data;
	}
	
	// set memory by 8 bytes.
	for (i = 0; i < (size / 8); i++) {
		((qword*)dest)[i] = qwdata;
	}
	
	// set remaining memory by 1 byte.
	remainBytesOffset = i * 8;
	for (i = 0; i < (size % 8); i++) {
		((char*)dest)[remainBytesOffset++] = data;
	}
}

int k_memcpy(void* dest, const void* src, int size) {
	int i;
	int remainBytesOffset;
	
	// copy memory by 8 bytes.
	for (i = 0; i < (size / 8); i++) {
		((qword*)dest)[i] = ((qword*)src)[i];
	}
	
	// copy remaining memory by 1 byte.
	remainBytesOffset = i * 8;
	for (i = 0; i < (size % 8); i++) {
		((char*)dest)[remainBytesOffset] = ((char*)src)[remainBytesOffset];
		remainBytesOffset++;
	}
	
	// return copied memory size.
	return size;
}

int k_memcmp(const void* dest, const void* src, int size) {
	int i, j;
	int remainBytesOffset;
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
	
	// compare remaining memory by 1 byte.
	remainBytesOffset = i * 8;
	for (i = 0; i < (size % 8); i++) {
		cvalue = ((char*)dest)[remainBytesOffset] - ((char*)src)[remainBytesOffset];
		if (cvalue != 0) {
			return cvalue;
		}
		
		remainBytesOffset++;
	}
	
	// return 0 if values are the same.
	return 0;
}

inline void k_memsetWord(void* dest, word data, int wordSize) {
	int i;
	qword qwdata;
	int remainWordsOffset;
	
	// make 4 words-sized data.
	qwdata = 0;
	for (i = 0; i < 4; i++) {
		qwdata = (qwdata << 16) | data;
	}
	
	// set memory by 4 words.
	for (i = 0; i < (wordSize / 4); i++) {
		((qword*)dest)[i] = qwdata;
	}
	
	// set remaining memory by 1 word.
	remainWordsOffset = i * 4;
	for (i = 0; i < (wordSize % 4); i++) {
		((word*)dest)[remainWordsOffset++] = data;
	}
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

int k_strcpy(char* dest, const char* src) {
	int i;

	for (i = 0; src[i] != '\0'; i++) {
		dest[i] = src[i];
	}

	dest[i] = '\0';

	return i;
}

int k_strcmp(const char* dest, const char* src) {
	int i;

	for (i = 0; (dest[i] != '\0') && (src[i] != '\0'); i++) {
		if ((dest[i] - src[i]) != 0) {
			break;
		}
	}

	return (dest[i] - src[i]);
}

int k_strncpy(char* dest, const char* src, int size) {
	int i;

	for (i = 0; (i < size) && (src[i] != '\0'); i++) {
		dest[i] = src[i];
	}

	dest[i] = '\0';

	return i;	
}

int k_strncmp(const char* dest, const char* src, int size) {
	int i;

	for (i = 0; (i < size) && (dest[i] != '\0') && (src[i] != '\0'); i++) {
		if ((dest[i] - src[i]) != 0) {
			break;
		}
	}

	if (i == size) {
		return 0;
	}

	return (dest[i] - src[i]);
}

int k_strlen(const char* str) {
	int i;
	
	for (i = 0; str[i] != '\0'; i++) {
		;
	}
		
	return i;
}

void k_reverseStr(char* str) {
	int len;
	int i;
	char temp;
	
	len = k_strlen(str);
	
	for (i = 0; i < (len / 2); i++) {
		temp = str[i];
		str[i] = str[len - 1 - i];
		str[len - 1 - i] = temp;
	}
}

bool k_equalStr(const char* str1, const char* str2) {
	int len1;
	int len2;
	
	len1 = k_strlen(str1);
	len2 = k_strlen(str2);
	
	if ((len1 == len2) && (k_memcmp(str1, str2, len2) == 0)) {
		return true;
	}
	
	return false;
}

int k_atoi10(const char* str) {
	int value = 0;
	int i;
	
	if (str[0] == '-') {
		i = 1;
		
	} else {
		i = 0;
	}
	
	for ( ; str[i] != '\0'; i++) {
		value *= 10;
		value += (str[i] - '0');
	}
	
	if (str[0] == '-') {
		value = -value;
	}
	
	return value;
}

dword k_atoi16(const char* str) {
	dword value = 0;
	int i;
	
	for (i = 0; str[i] != '\0'; i++) {
		value *= 16;
		
		if ('A' <= str[i] && str[i] <= 'Z') {
			value += (str[i] - 'A') + 10;
			
		} else if ('a' <= str[i] && str[i] <= 'z') {
			value += (str[i] - 'a') + 10;
			
		} else {
			value += (str[i] - '0');
		}
	}
	
	return value;
}

long k_atol10(const char* str) {
	long value = 0;
	int i;
	
	if (str[0] == '-') {
		i = 1;
		
	} else {
		i = 0;
	}
	
	for ( ; str[i] != '\0'; i++) {
		value *= 10;
		value += (str[i] - '0');
	}
	
	if (str[0] == '-') {
		value = -value;
	}
	
	return value;
}

qword k_atol16(const char* str) {
	qword value = 0;
	int i;
	
	for (i = 0; str[i] != '\0'; i++) {
		value *= 16;
		
		if ('A' <= str[i] && str[i] <= 'Z') {
			value += (str[i] - 'A') + 10;
			
		} else if ('a' <= str[i] && str[i] <= 'z') {
			value += (str[i] - 'a') + 10;
			
		} else {
			value += (str[i] - '0');
		}
	}
	
	return value;
}

int k_itoa10(int value, char* str) {
	int i;
	
	if (value == 0) {
		str[0] = '0';
		str[1] = '\0';
		return 1;
	}
	
	if (value < 0) {
		i = 1;
		str[0] = '-';
		value = -value;
		
	} else {
		i = 0;
	}
	
	for ( ; value > 0; i++) {
		str[i] = (value % 10) + '0';
		value /= 10;
	}
	
	str[i] = '\0';
	
	if (str[0] == '-') {
		k_reverseStr(&(str[1]));
		
	} else {
		k_reverseStr(str);
	}
	
	return i;
}

int k_itoa16(dword value, char* str) {
	dword i;
	dword currentValue;
	
	if (value == 0) {
		str[0] = '0';
		str[1] = '\0';
		return 1;
	}
	
	for (i = 0; value > 0; i++) {
		currentValue = value % 16;
		
		if (currentValue >= 10) {
			str[i] = (currentValue - 10) + 'A';
			
		} else {
			str[i] = currentValue + '0';
		}
		
		value /= 16;
	}
	
	str[i] = '\0';
	
	k_reverseStr(str);
	
	return i;
}

int k_ltoa10(long value, char* str) {
	long i;
	
	if (value == 0) {
		str[0] = '0';
		str[1] = '\0';
		return 1;
	}
	
	if (value < 0) {
		i = 1;
		str[0] = '-';
		value = -value;
		
	} else {
		i = 0;
	}
	
	for ( ; value > 0; i++) {
		str[i] = (value % 10) + '0';
		value /= 10;
	}
	
	str[i] = '\0';
	
	if (str[0] == '-') {
		k_reverseStr(&(str[1]));
		
	} else {
		k_reverseStr(str);
	}
	
	return i;
}

int k_ltoa16(qword value, char* str) {
	qword i;
	qword currentValue;
	
	if (value == 0) {
		str[0] = '0';
		str[1] = '\0';
		return 1;
	}
	
	for (i = 0; value > 0; i++) {
		currentValue = value % 16;
		
		if (currentValue >= 10) {
			str[i] = (currentValue - 10) + 'A';
			
		} else {
			str[i] = currentValue + '0';
		}
		
		value /= 16;
	}
	
	str[i] = '\0';
	
	k_reverseStr(str);
	
	return i;
}

int k_sprintf(char* str, const char* format, ...) {
	va_list ap;
	int ret;
	
	va_start(ap, format);
	ret = k_vsprintf(str, format, ap);
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
int k_vsprintf(char* str, const char* format, va_list ap) {
	qword i, j, k;
	int index = 0; // string index
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
				k_memcpy(str + index, copyStr, copyLen);
				index += copyLen;
				break;
				
			case 'c':
				str[index] = (char)(va_arg(ap, int));
				index++;
				break;
				
			case 'd':
			case 'i':
				ivalue = (int)(va_arg(ap, int));
				index += k_ltoa10(ivalue, str + index);
				break;
				
			case 'x':
			case 'X':
				qwvalue = (dword)(va_arg(ap, dword)) & 0xFFFFFFFF;
				index += k_ltoa16(qwvalue, str + index);
				break;
				
			case 'q':
			case 'Q':
			case 'p':
				qwvalue = (qword)(va_arg(ap, qword));
				index += k_ltoa16(qwvalue, str + index);
				break;
				
			case 'f':
				dvalue = (double)(va_arg(ap, double));
				
				// half-round up at the third position below decimal point.
				dvalue += 0.005;
				
				// save value to string sequentially from the second position below decimal point in decimal part.
				str[index] = '0' + ((qword)(dvalue * 100) % 10);
				str[index + 1] = '0' + ((qword)(dvalue * 10) % 10);
				str[index + 2] = '.';
				
				for (k = 0; ; k++) {
					// If integer part == 0, break.
					if (((qword)dvalue == 0) && (k != 0)) {
						break;
					}
					
					// save value to string sequentially from the units digit in integer part.
					str[index + 3 + k] = '0' + ((qword)dvalue % 10);
					dvalue = dvalue / 10;
				}
				
				str[index + 3 + k] = '\0';
				
				// reverse as many as float-saved length, increase string index.
				k_reverseStr(str + index);
				index += 3 + k;
				break;
				
			default:
				str[index] = format[i];
				index++;
				break;
			}
			
		// If not '%', it's normal character.
		} else {
			str[index] = format[i];
			index++;
		}
	}
	
	str[index] = '\0';
	
	// return length of printed string.
	return index;
}

int k_findChar(const char* str, char ch) {
	int i;

	for (i = 0; str[i] != '\0'; i++) {
		if (str[i] == ch) {
			return i;
		}
	}

	return -1;
}

bool k_addFileExtension(char* fileName, const char* extension) {
	int i, j;

	if (k_findChar(fileName, '.') >= 0) {
		return false;
	}

	j = k_strlen(fileName);
	fileName[j++] = '.';
	for (i = 0; extension[i] != '\0'; i++) {
		fileName[j++] = extension[i];
	}

	fileName[j] = '\0';

	return true;
}

// interrupt-occurring count by Timer(IRQ 0, PIT Controller)
volatile qword g_tickCount = 0;

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

static volatile qword g_randomValue = 0;

qword k_rand(void) {
	g_randomValue = (g_randomValue * 412153 + 5571031) >> 16;
	return g_randomValue;
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

bool k_isGraphicMode(void) {
	if (VBE_GRAPHICMODEFLAG == VBE_GRAPHICMODEFLAG_TEXTMODE) {
		return false;
	}

	return true;
}
