#ifndef __USERLIB_H__
#define __USERLIB_H__

#include <stdarg.h>
#include "types.h"

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

/* Console Functions */
void printf(const char* format, ...);

/* Math Functions */
qword srand(qword seed);
qword rand(void);

/* 2D Graphics Functions */
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