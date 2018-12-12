#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "types.h"

#define SC_MAXPARAMCOUNT 10

/* Macro Function */
#define PARAM(x) (paramTable.values[(x)])

#pragma pack(push, 1)

typedef struct __ParamTable {
	qword values[SC_MAXPARAMCOUNT];
} ParamTable;

#pragma pack(pop)

/****** Functions defined in syscall_asm.asm ******/
qword executeSyscall(qword syscallNumber, const ParamTable* paramTable);

/****** Functions defined in syscall.c ******/
/*** Syscall from console.h ***/
void setCursor(int x, int y);
void getCursor(int* x, int* y);
int printStr(const char* str);
void clearScreen(void);
byte getch(void);

/*** Syscall from rtc.h ***/
void readRtcTime(byte* hour, byte* minute, byte* second);
void readRtcDate(word* year, byte* month, byte* dayOfMonth, byte* dayOfWeek);

/*** Syscall from task.h ***/
qword createTask(qword flags, void* memAddr, qword memSize, qword entryPointAddr, byte affinity);
qword getCurrentTaskId(void);
bool schedule(void);
bool changeTaskPriority(qword taskId, byte priority);
bool changeTaskAffinity(qword taskId, byte affinity);
bool waitTask(qword taskId);
bool notifyTask(qword taskId);
bool endTask(qword taskId);
void exit(int status);
int getTaskCount(byte apicId);
bool existTask(qword taskId);
qword getProcessorLoad(byte apicId);
bool waitGroup(qword groupId, void* lock);
bool notifyOneInWaitGroup(qword groupId);
bool notifyAllInWaitGroup(qword groupId);
bool joinGroup(qword* taskIds, int count);
bool notifyOneInJoinGroup(qword groupId);
bool notifyAllInJoinGroup(qword groupId);
qword createThread(qword entryPointAddr, qword arg, byte affinity);

/*** Syscall from sync.h ***/
void lock(Mutex* mutex);
void unlock(Mutex* mutex);

/*** Syscall from dynamic_mem.h ***/
void* malloc(qword size);
bool free(void* addr);

/*** Syscall from hdd.h ***/
int readHddSector(bool primary, bool master, dword lba, int sectorCount, char* buffer);
int writeHddSector(bool primary, bool master, dword lba, int sectorCount, char* buffer);

/*** Syscall from file_system.h ***/
File* fopen(const char* fileName, const char* mode);
dword fread(void* buffer, dword size, dword count, File* file);
dword fwrite(const void* buffer, dword size, dword count, File* file);
int fseek(File* file, int offset, int origin);
int fclose(File* file);
int remove(const char* fileName);
Dir* opendir(const char* dirName);
dirent* readdir(Dir* dir);
void rewinddir(Dir* dir);
int closedir(Dir* dir);
bool isfopen(const dirent* entry);

/*** Syscall from serial_port.h ***/
void sendSerialData(byte* buffer, int size);
int recvSerialData(byte* buffer, int size);
void clearSerialFifo(void);

/*** Syscall from mp_config_table.h ***/
int k_getProcessorCount(void);

/*** Syscall from multiprocessor.h ***/
byte k_getApicId(void);

/*** Syscall from window.h ***/
qword getBackgroundWindowId(void);
void getScreenArea(Rect* screenArea);
qword createWindow(int x, int y, int width, int height, dword flags, const char* title, Color backgroundColor, Menu* topMenu, void* widget, qword parentId);
bool deleteWindow(qword windowId);
bool isWindowShown(qword windowId);
bool showWindow(qword windowId, bool show);
qword findWindowByPoint(int x, int y);
qword findWindowByTitle(const char* title);
bool existWindow(qword windowId);
qword getTopWindowId(void);
bool isTopWindow(qword windowId);
bool isTopWindowWithChild(qword windowId);
bool moveWindowToTop(qword windowId);
bool isPointInTitleBar(qword windowId, int x, int y);
bool isPointInCloseButton(qword windowId, int x, int y);
bool isPointInResizeButton(qword windowId, int x, int y);
bool isPointInTopMenu(qword windowId, int x, int y);
bool moveWindow(qword windowId, int x, int y);
bool resizeWindow(qword windowId, int x, int y, int width, int height);
void moveChildWindows(qword windowId, int moveX, int moveY);
void showChildWindows(qword windowId, bool show, dword flags, bool parentToTop);
void deleteChildWindows(qword windowId);
void* getNthChildWidget(qword windowId, int n);
bool getWindowArea(qword windowId, Rect* area);
bool sendEventToWindow(const Event* event, qword windowId);
bool recvEventFromWindow(Event* event, qword windowId);
bool updateScreenById(qword windowId);
bool updateScreenByWindowArea(qword windowId, const Rect* area);
bool updateScreenByScreenArea(const Rect* area);
bool drawWindowBackground(qword windowId);
bool drawWindowFrame(qword windowId);
bool drawWindowTitleBar(qword windowId, bool selected);
bool drawPixel(qword windowId, int x, int y, Color color);
bool drawLine(qword windowId, int x1, int y1, int x2, int y2, Color color);
bool drawRect(qword windowId, int x1, int y1, int x2, int y2, Color color, bool fill);
bool drawCircle(qword windowId, int x, int y, int radius, Color color, bool fill);
bool drawText(qword windowId, int x, int y, Color textColor, Color backgroundColor, const char* str, int len);
bool bitblt(qword windowId, int x, int y, const Color* buffer, int width, int height);
void moveMouseCursor(int x, int y);
void getMouseCursorPos(int* x, int* y);

/*** Syscall from jpeg.h ***/
bool initJpeg(Jpeg* jpeg, const byte* fileBuffer, dword fileSize);
bool decodeJpeg(Jpeg* jpeg, Color* imageBuffer);

/*** Syscall from util.h ***/
qword getTotalRamSize(void);
qword getTickCount(void);
void sleep(qword millisecond);
bool isGraphicMode(void);

/*** Syscall from app_manager.h ***/
qword executeApp(const char* fileName, const char* args, byte affinity);

/*** Syscall from widgets.h ***/
bool createMenu(Menu* menu, int x, int y, int itemHeight, Color* colors, qword parentId, Menu* top, dword flags);
bool processMenuEvent(Menu* menu);
bool drawHansLogo(qword windowId, int x, int y, int width, int height, Color brightColor, Color darkColor);
bool drawButton(qword windowId, const Rect* buttonArea, Color textColor, Color backgroundColor, const char* text, dword flags);
void setClock(Clock* clock, qword windowId, int x, int y, Color textColor, Color backgroundColor, byte format, bool reset);
void addClock(Clock* clock);
Clock* removeClock(qword clockId);

/*** Syscall from kid.h ***/
qword allocKid(void);
void freeKid(qword id);

#endif // __SYSCALL_H__