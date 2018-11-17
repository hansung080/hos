#ifndef __USERLIB_H__
#define __USERLIB_H__

#include <stdarg.h>
#include "types.h"

// argument-related macros
#define ARG_MAXLENGTH              30 // It's including the last null character, so the max argument length user can input is 29.
#define ARG_ERROR_TOOLONGARGLENGTH -1 // too long argument length error

#pragma pack(push, 1)

typedef struct __ArgList {
	const char* args; // argument string
	int len;          // argument string length
	int currentIndex; // current index in argument string
} ArgList;

#pragma pack(pop)

/* Argument Functions */
void initArgs(ArgList* list, const char* args);
int getNextArg(ArgList* list, char* arg);

/* Memory Functions */
void memset(void* dest, byte data, int size);
int memcpy(void* dest, const void* src, int size);
int memcmp(const void* dest, const void* src, int size);

/* String Functions */
int strcpy(char* dest, const char* src);
int strcmp(const char* dest, const char* src);
int strncpy(char* dest, const char* src, int size);
int strncmp(const char* dest, const char* src, int size);
int strlen(const char* str);
void reverseStr(char* str);
int atoi10(const char* str);
dword atoi16(const char* str);
long atol10(const char* str);
qword atol16(const char* str);
int itoa10(int value, char* str);
int itoa16(dword value, char* str);
int ltoa10(long value, char* str);
int ltoa16(qword value, char* str);
int sprintf(char* str, const char* format, ...);
int vsprintf(char* str, const char* format, va_list ap);
int findChar(const char* str, char ch);
bool addFileExtension(char* fileName, const char* extension);

/* Math Functions */
qword srand(qword seed);
qword rand(void);

/* Console Functions */
void printf(const char* format, ...);

/* Synchronization Functions */
void k_initMutex(Mutex* mutex);

/* 2D Graphics Functions */
Color changeColorBrightness(Color color, int r, int g, int b);
Color changeColorBrightness2(Color color, int r, int g, int b);
void setRect(Rect* rect, int x1, int y1, int x2, int y2);
int getRectWidth(const Rect* rect);
int getRectHeight(const Rect* rect);
bool isPointInRect(const Rect* rect, int x, int y);
bool isRectOverlapped(const Rect* rect1, const Rect* rect2);
bool getOverlappedRect(const Rect* rect1, const Rect* rect2, Rect* overRect);

/* Window Functions */
bool convertPointScreenToWindow(qword windowId, const Point* screenPoint, Point* windowPoint);
bool convertPointWindowToScreen(qword windowId, const Point* windowPoint, Point* screenPoint);
bool convertRectScreenToWindow(qword windowId, const Rect* screenRect, Rect* windowRect);
bool convertRectWindowToScreen(qword windowId, const Rect* windowRect, Rect* screenRect);
bool setMouseEvent(Event* event, qword windowId, qword eventType, int mouseX, int mouseY, byte buttonStatus);
bool setWindowEvent(Event* event, qword windowId, qword eventType);
void setKeyEvent(Event* event, qword windowId, const Key* key);
bool sendMouseEventToWindow(qword windowId, qword eventType, int mouseX, int mouseY, byte buttonStatus);
bool sendWindowEventToWindow(qword windowId, qword eventType);
bool sendKeyEventToWindow(qword windowId, const Key* key);

#endif // __USERLIB_H__