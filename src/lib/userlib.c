#include "userlib.h"
#include "syscall.h"

void initArgs(ArgList* list, const char* args) {
	list->args = args;
	list->len = strlen(args);
	list->currentIndex = 0;
}

int getNextArg(ArgList* list, char* arg) {
	int spaceIndex;
	int len;

	if (list->currentIndex >= list->len) {
		return 0;
	}

	// get space index (argument length).
	for (spaceIndex = list->currentIndex; spaceIndex < list->len; spaceIndex++) {
		if (list->args[spaceIndex] == ' ') {
			break;
		}
	}

	// copy argument and update current index.
	memcpy(arg, list->args + list->currentIndex, spaceIndex);
	len = spaceIndex - list->currentIndex;
	arg[len] = '\0';
	list->currentIndex += len + 1;

	if (len >= ARG_MAXLENGTH) {
		printf("too long argument length: Argument length must be less than %d\n", ARG_MAXLENGTH - 1);
		return ARG_ERROR_TOOLONGARGLENGTH;
	}

	// return current argument length.
	return len;
}

void memset(void* dest, byte data, int size) {
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

int memcpy(void* dest, const void* src, int size) {
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

int memcmp(const void* dest, const void* src, int size) {
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

int strcpy(char* dest, const char* src) {
	int i;

	for (i = 0; src[i] != '\0'; i++) {
		dest[i] = src[i];
	}

	dest[i] = '\0';

	return i;
}

int strcmp(const char* dest, const char* src) {
	int i;

	for (i = 0; (dest[i] != '\0') && (src[i] != '\0'); i++) {
		if ((dest[i] - src[i]) != 0) {
			break;
		}
	}

	return (dest[i] - src[i]);
}

int strncpy(char* dest, const char* src, int size) {
	int i;

	for (i = 0; (i < size) && (src[i] != '\0'); i++) {
		dest[i] = src[i];
	}

	dest[i] = '\0';

	return i;	
}

int strncmp(const char* dest, const char* src, int size) {
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

int strlen(const char* str) {
	int i;
	
	for (i = 0; str[i] != '\0'; i++) {
		;
	}
		
	return i;
}

void reverseStr(char* str) {
	int len;
	int i;
	char temp;
	
	len = strlen(str);
	
	for (i = 0; i < (len / 2); i++) {
		temp = str[i];
		str[i] = str[len - 1 - i];
		str[len - 1 - i] = temp;
	}
}

int atoi10(const char* str) {
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

dword atoi16(const char* str) {
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

long atol10(const char* str) {
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

qword atol16(const char* str) {
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

int itoa10(int value, char* str) {
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
		reverseStr(&(str[1]));
		
	} else {
		reverseStr(str);
	}
	
	return i;
}

int itoa16(dword value, char* str) {
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
	
	reverseStr(str);
	
	return i;
}

int ltoa10(long value, char* str) {
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
		reverseStr(&(str[1]));
		
	} else {
		reverseStr(str);
	}
	
	return i;
}

int ltoa16(qword value, char* str) {
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
	
	reverseStr(str);
	
	return i;
}

int sprintf(char* str, const char* format, ...) {
	va_list ap;
	int ret;
	
	va_start(ap, format);
	ret = vsprintf(str, format, ap);
	va_end(ap);
	
	// return length of printed string.
	return ret;
}

/**
  < Data Types Supported by vsprintf Function >
  - %s         : string
  - %c         : char
  - %d, %i     : int (decimal, signed)
  - %x, %X     : dword (hexadecimal, unsigned)
  - %q, %Q, %p : qword (hexadecimal, unsigned)
  - %f         : float (print down to the second position below decimal point by half-rounding up at the third position below decimal point.)
 */
int vsprintf(char* str, const char* format, va_list ap) {
	qword i, j, k;
	int index = 0; // string index
	int formatLen, copyLen;
	char* copyStr;
	qword qwvalue;
	int ivalue;
	double dvalue;
	
	formatLen = strlen(format);
	
	for (i = 0; i < formatLen; i++) {
		// If '%', it's data type.
		if (format[i] == '%') {
			i++;
			switch(format[i]){
			case 's':
				copyStr = (char*)(va_arg(ap, char*));
				copyLen = strlen(copyStr);
				memcpy(str + index, copyStr, copyLen);
				index += copyLen;
				break;
				
			case 'c':
				str[index] = (char)(va_arg(ap, int));
				index++;
				break;
				
			case 'd':
			case 'i':
				ivalue = (int)(va_arg(ap, int));
				index += ltoa10(ivalue, str + index);
				break;
				
			case 'x':
			case 'X':
				qwvalue = (dword)(va_arg(ap, dword)) & 0xFFFFFFFF;
				index += ltoa16(qwvalue, str + index);
				break;
				
			case 'q':
			case 'Q':
			case 'p':
				qwvalue = (qword)(va_arg(ap, qword));
				index += ltoa16(qwvalue, str + index);
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
				reverseStr(str + index);
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

int findChar(const char* str, char ch) {
	int i;

	for (i = 0; str[i] != '\0'; i++) {
		if (str[i] == ch) {
			return i;
		}
	}

	return -1;
}

bool addFileExtension(char* fileName, const char* extension) {
	int i, j;

	if (findChar(fileName, '.') >= 0) {
		return false;
	}

	j = strlen(fileName);
	fileName[j++] = '.';
	for (i = 0; extension[i] != '\0'; i++) {
		fileName[j++] = extension[i];
	}

	fileName[j] = '\0';

	return true;
}

static volatile qword g_randomValue = 0;

qword srand(qword seed) {
	g_randomValue = seed;
}

qword rand(void) {
	g_randomValue = (g_randomValue * 412153 + 5571031) >> 16;
	return g_randomValue;
}

void printf(const char* format, ...) {
	va_list ap;
	char str[1024];
	int nextPrintOffset;
	
	va_start(ap, format);
	vsprintf(str, format, ap);
	va_end(ap);
	
	nextPrintOffset = printStr(str);
	
	setCursor(nextPrintOffset % CONSOLE_WIDTH, nextPrintOffset / CONSOLE_WIDTH);
}

void initMutex(Mutex* mutex) {
	mutex->lockFlag = false;
	mutex->lockCount = 0;
	mutex->taskId = TASK_INVALIDID;
}

Color changeColorBrightness(Color color, int r, int g, int b) {
	int red, green, blue;

	red = GETR(color);
	green = GETG(color);
	blue = GETB(color);

	red += r;
	green += g;
	blue += b;

	if (red < 0) {
		red = 0;

	} else if (red > 255) {
		red = 255;
	}

	if (green < 0) {
		green = 0;

	} else if (green > 255) {
		green = 255;
	}

	if (blue < 0) {
		blue = 0;

	} else if (blue > 255) {
		blue = 255;
	}

	return RGB(red, green, blue);
}

Color changeColorBrightness2(Color color, int r, int g, int b) {
	int red, green, blue;

	red = GETR(color);
	green = GETG(color);
	blue = GETB(color);

	red += r;
	green += g;
	blue += b;

	if (red < 86) {
		red = 86;

	} else if (red > 229) {
		red = 229;
	}

	if (green < 86) {
		green = 86;

	} else if (green > 229) {
		green = 229;
	}

	if (blue < 86) {
		blue = 86;

	} else if (blue > 229) {
		blue = 229;
	}

	return RGB(red, green, blue);
}

void setRect(Rect* rect, int x1, int y1, int x2, int y2) {
	// This logic guarantee the rule that rect.x1 < rect.x2.
	if (x1 < x2) {
		rect->x1 = x1;
		rect->x2 = x2;

	} else {
		rect->x1 = x2;
		rect->x2 = x1;		
	}

	// This logic guarantee the rule that rect.y1 < rect.y2.
	if (y1 < y2) {
		rect->y1 = y1;
		rect->y2 = y2;

	} else {
		rect->y1 = y2;
		rect->y2 = y1;		
	}
}

int getRectWidth(const Rect* rect) {
	int width;

	width = rect->x2 - rect->x1 + 1;
	if (width < 0) {
		return -width;
	}

	return width;
}

int getRectHeight(const Rect* rect) {
	int height;

	height = rect->y2 - rect->y1 + 1;
	if (height < 0) {
		return -height;
	}

	return height;
}

bool isPointInRect(const Rect* rect, int x, int y) {
	// If (x, y) is outside rectangle, return false.
	if ((x < rect->x1) || (x > rect->x2) || (y < rect->y1) || (y > rect->y2)) {
		return false;
	}

	return true;
}

bool isRectOverlapped(const Rect* rect1, const Rect* rect2) {
	// If rectangle1 and rectangle2 are not overlapped, return false.
	if ((rect1->x1 > rect2->x2) || (rect1->x2 < rect2->x1) || (rect1->y1 > rect2->y2) || (rect1->y2 < rect2->y1)) {
		return false;
	}

	return true;
}

bool getOverlappedRect(const Rect* rect1, const Rect* rect2, Rect* overRect) {
	int maxX1;
	int minX2;
	int maxY1;
	int minY2;

	maxX1 = MAX(rect1->x1, rect2->x1);
	minX2 = MIN(rect1->x2, rect2->x2);
	// If rectangle1 and rectangle2 are not overlapped, return false.
	if (maxX1 > minX2) {
		return false;
	}

	maxY1 = MAX(rect1->y1, rect2->y1);
	minY2 = MIN(rect1->y2, rect2->y2);
	// If rectangle1 and rectangle2 are not overlapped, return false.
	if (maxY1 > minY2) {
		return false;
	}

	// If rectangle1 and rectangle2 are overlapped, set overlapped rectangle and return true.
	overRect->x1 = maxX1;
	overRect->y1 = maxY1;
	overRect->x2 = minX2;
	overRect->y2 = minY2;

	return true;
}

bool convertPointScreenToWindow(qword windowId, const Point* screenPoint, Point* windowPoint) {
	Rect area;

	if (getWindowArea(windowId, &area) == false) {
		return false;
	}

	windowPoint->x = screenPoint->x - area.x1;
	windowPoint->y = screenPoint->y - area.y1;

	return true;
}

bool convertPointWindowToScreen(qword windowId, const Point* windowPoint, Point* screenPoint) {
	Rect area;

	if (getWindowArea(windowId, &area) == false) {
		return false;
	}

	screenPoint->x = windowPoint->x + area.x1;
	screenPoint->y = windowPoint->y + area.y1;

	return true;
}

bool convertRectScreenToWindow(qword windowId, const Rect* screenRect, Rect* windowRect) {
	Rect area;

	if (getWindowArea(windowId, &area) == false) {
		return false;
	}

	windowRect->x1 = screenRect->x1 - area.x1;
	windowRect->y1 = screenRect->y1 - area.y1;
	windowRect->x2 = screenRect->x2 - area.x1;
	windowRect->y2 = screenRect->y2 - area.y1;

	return true;
}

bool convertRectWindowToScreen(qword windowId, const Rect* windowRect, Rect* screenRect) {
	Rect area;

	if (getWindowArea(windowId, &area) == false) {
		return false;
	}

	screenRect->x1 = windowRect->x1 + area.x1;
	screenRect->y1 = windowRect->y1 + area.y1;
	screenRect->x2 = windowRect->x2 + area.x1;
	screenRect->y2 = windowRect->y2 + area.y1;

	return true;	
}

bool setMouseEvent(Event* event, qword windowId, qword eventType, int mouseX, int mouseY, byte buttonStatus) {
	Point screenMousePoint;
	Point windowMousePoint;

	switch (eventType) {
	case EVENT_MOUSE_MOVE:
	case EVENT_MOUSE_LBUTTONDOWN:
	case EVENT_MOUSE_LBUTTONUP:
	case EVENT_MOUSE_RBUTTONDOWN:
	case EVENT_MOUSE_RBUTTONUP:
	case EVENT_MOUSE_MBUTTONDOWN:
	case EVENT_MOUSE_MBUTTONUP:
		screenMousePoint.x = mouseX;
		screenMousePoint.y = mouseY;

		if (convertPointScreenToWindow(windowId, &screenMousePoint, &windowMousePoint) == false) {
			return false;
		}

		/* set mouse event */
		event->type = eventType;
		event->mouseEvent.windowId = windowId;
		memcpy(&event->mouseEvent.point, &windowMousePoint, sizeof(Point));
		event->mouseEvent.buttonStatus = buttonStatus;

		break;

	default:
		return false;
	}

	return true;
}

bool setWindowEvent(Event* event, qword windowId, qword eventType) {
	Rect area;

	switch (eventType) {
	case  EVENT_WINDOW_SELECT:
	case  EVENT_WINDOW_DESELECT:
	case  EVENT_WINDOW_MOVE:
	case  EVENT_WINDOW_RESIZE:
	case  EVENT_WINDOW_CLOSE:
		if (getWindowArea(windowId, &area) == false) {
			return false;
		}

		/* set window event */
		event->type = eventType;
		event->windowEvent.windowId = windowId;
		memcpy(&event->windowEvent.area, &area, sizeof(Rect));

		break;

	default:
		return false;
	}

	return true;
}

void setKeyEvent(Event* event, qword windowId, const Key* key) {
	if (key->flags & KEY_FLAGS_DOWN) {
		event->type = EVENT_KEY_DOWN;

	} else {
		event->type = EVENT_KEY_UP;
	}

	event->keyEvent.windowId = windowId;
	event->keyEvent.scanCode = key->scanCode;
	event->keyEvent.asciiCode = key->asciiCode;
	event->keyEvent.flags = key->flags;
}

bool sendMouseEventToWindow(qword windowId, qword eventType, int mouseX, int mouseY, byte buttonStatus) {
	Event event;

	if (setMouseEvent(&event, windowId, eventType, mouseX, mouseY, buttonStatus) == false) {
		return false;
	}

	return sendEventToWindow(&event, windowId);
}

bool sendWindowEventToWindow(qword windowId, qword eventType) {
	Event event;

	if (setWindowEvent(&event, windowId, eventType) == false) {
		return false;
	}

	return sendEventToWindow(&event, windowId);	
}

bool sendKeyEventToWindow(qword windowId, const Key* key) {
	Event event;

	setKeyEvent(&event, windowId, key);

	return sendEventToWindow(&event, windowId);
}
