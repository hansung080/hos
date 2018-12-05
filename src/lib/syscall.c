#include "syscall.h"
#include "syscall_numbers.h"
#include "userlib.h"

void setCursor(int x, int y) {
	ParamTable paramTable;

	PARAM(0) = (qword)x;
	PARAM(1) = (qword)y;

	executeSyscall(SYSCALL_SETCURSOR, &paramTable);
}

void getCursor(int* x, int* y) {
	ParamTable paramTable;

	PARAM(0) = (qword)x;
	PARAM(1) = (qword)y;

	executeSyscall(SYSCALL_GETCURSOR, &paramTable);
}

int printStr(const char* str) {
	ParamTable paramTable;

	PARAM(0) = (qword)str;

	return (int)executeSyscall(SYSCALL_PRINTSTR, &paramTable);
}

void clearScreen(void) {
	executeSyscall(SYSCALL_CLEARSCREEN, null);
}

byte getch(void) {
	return (byte)executeSyscall(SYSCALL_GETCH, null);
}

void readRtcTime(byte* hour, byte* minute, byte* second) {
	ParamTable paramTable;

	PARAM(0) = (qword)hour;
	PARAM(1) = (qword)minute;
	PARAM(2) = (qword)second;

	executeSyscall(SYSCALL_READRTCTIME, &paramTable);
}

void readRtcDate(word* year, byte* month, byte* dayOfMonth, byte* dayOfWeek) {
	ParamTable paramTable;

	PARAM(0) = (qword)year;
	PARAM(1) = (qword)month;
	PARAM(2) = (qword)dayOfMonth;
	PARAM(3) = (qword)dayOfWeek;

	executeSyscall(SYSCALL_READRTCDATE, &paramTable);
}

qword createTask(qword flags, void* memAddr, qword memSize, qword entryPointAddr, byte affinity) {
	ParamTable paramTable;

	PARAM(0) = flags;
	PARAM(1) = (qword)memAddr;
	PARAM(2) = memSize;
	PARAM(3) = entryPointAddr;
	PARAM(4) = (qword)affinity;

	return executeSyscall(SYSCALL_CREATETASK, &paramTable);
}

qword getCurrentTaskId(void) {
	return executeSyscall(SYSCALL_GETCURRENTTASKID, null);
}

bool schedule(void) {
	return (bool)executeSyscall(SYSCALL_SCHEDULE, null);
}

bool changeTaskPriority(qword taskId, byte priority) {
	ParamTable paramTable;

	PARAM(0) = taskId;
	PARAM(1) = (qword)priority;

	return (bool)executeSyscall(SYSCALL_CHANGETASKPRIORITY, &paramTable);
}

bool changeTaskAffinity(qword taskId, byte affinity) {
	ParamTable paramTable;

	PARAM(0) = taskId;
	PARAM(1) = (qword)affinity;

	return (bool)executeSyscall(SYSCALL_CHANGETASKAFFINITY, &paramTable);
}

bool waitTask(qword taskId) {
	ParamTable paramTable;

	PARAM(0) = taskId;

	return (bool)executeSyscall(SYSCALL_WAITTASK, &paramTable);
}

bool notifyTask(qword taskId) {
	ParamTable paramTable;

	PARAM(0) = taskId;

	return (bool)executeSyscall(SYSCALL_NOTIFYTASK, &paramTable);
}

bool endTask(qword taskId) {
	ParamTable paramTable;

	PARAM(0) = taskId;

	return (bool)executeSyscall(SYSCALL_ENDTASK, &paramTable);
}

void exit(int status) {
	ParamTable paramTable;

	PARAM(0) = (qword)status;

	executeSyscall(SYSCALL_EXIT, &paramTable);
}

int getTaskCount(byte apicId) {
	ParamTable paramTable;

	PARAM(0) = (qword)apicId;

	return (int)executeSyscall(SYSCALL_GETTASKCOUNT, &paramTable);
}

bool existTask(qword taskId) {
	ParamTable paramTable;

	PARAM(0) = taskId;

	return (bool)executeSyscall(SYSCALL_EXISTTASK, &paramTable);
}

qword getProcessorLoad(byte apicId) {
	ParamTable paramTable;

	PARAM(0) = (qword)apicId;

	return executeSyscall(SYSCALL_GETPROCESSORLOAD, &paramTable);
}

bool waitGroup(qword groupId, void* lock) {
	ParamTable paramTable;

	PARAM(0) = groupId;
	PARAM(1) = (qword)lock;

	return (bool)executeSyscall(SYSCALL_WAITGROUP, &paramTable);
}

bool notifyOneInWaitGroup(qword groupId) {
	ParamTable paramTable;

	PARAM(0) = groupId;

	return (bool)executeSyscall(SYSCALL_NOTIFYONEINWAITGROUP, &paramTable);
}

bool notifyAllInWaitGroup(qword groupId) {
	ParamTable paramTable;

	PARAM(0) = groupId;

	return (bool)executeSyscall(SYSCALL_NOTIFYALLINWAITGROUP, &paramTable);
}

bool joinGroup(qword* taskIds, int count) {
	ParamTable paramTable;

	PARAM(0) = (qword)taskIds;
	PARAM(1) = (qword)count;

	return (bool)executeSyscall(SYSCALL_JOINGROUP, &paramTable);
}

bool notifyOneInJoinGroup(qword groupId) {
	ParamTable paramTable;

	PARAM(0) = groupId;

	return (bool)executeSyscall(SYSCALL_NOTIFYONEINJOINGROUP, &paramTable);
}

bool notifyAllInJoinGroup(qword groupId) {
	ParamTable paramTable;

	PARAM(0) = groupId;

	return (bool)executeSyscall(SYSCALL_NOTIFYALLINJOINGROUP, &paramTable);
}

qword createThread(qword entryPointAddr, qword arg, byte affinity) {
	ParamTable paramTable;

	PARAM(0) = entryPointAddr;
	PARAM(1) = arg;
	PARAM(2) = (qword)affinity;
	PARAM(3) = (qword)exit;

	return executeSyscall(SYSCALL_CREATETHREAD, &paramTable);
}

void lock(Mutex* mutex) {
	ParamTable paramTable;

	PARAM(0) = (qword)mutex;

	executeSyscall(SYSCALL_LOCK, &paramTable);
}

void unlock(Mutex* mutex) {
	ParamTable paramTable;

	PARAM(0) = (qword)mutex;

	executeSyscall(SYSCALL_UNLOCK, &paramTable);
}

void* malloc(qword size) {
	ParamTable paramTable;

	PARAM(0) = size;

	return (void*)executeSyscall(SYSCALL_MALLOC, &paramTable);
}

bool free(void* addr) {
	ParamTable paramTable;

	PARAM(0) = (qword)addr;

	return (bool)executeSyscall(SYSCALL_FREE, &paramTable);
}

int readHddSector(bool primary, bool master, dword lba, int sectorCount, char* buffer) {
	ParamTable paramTable;

	PARAM(0) = (qword)primary;
	PARAM(1) = (qword)master;
	PARAM(2) = (qword)lba;
	PARAM(3) = (qword)sectorCount;
	PARAM(4) = (qword)buffer;

	return (int)executeSyscall(SYSCALL_READHDDSECTOR, &paramTable);
}

int writeHddSector(bool primary, bool master, dword lba, int sectorCount, char* buffer) {
	ParamTable paramTable;

	PARAM(0) = (qword)primary;
	PARAM(1) = (qword)master;
	PARAM(2) = (qword)lba;
	PARAM(3) = (qword)sectorCount;
	PARAM(4) = (qword)buffer;

	return (int)executeSyscall(SYSCALL_WRITEHDDSECTOR, &paramTable);
}

File* fopen(const char* fileName, const char* mode) {
	ParamTable paramTable;

	PARAM(0) = (qword)fileName;
	PARAM(1) = (qword)mode;

	return (File*)executeSyscall(SYSCALL_FOPEN, &paramTable);
}

dword fread(void* buffer, dword size, dword count, File* file) {
	ParamTable paramTable;

	PARAM(0) = (qword)buffer;
	PARAM(1) = (qword)size;
	PARAM(2) = (qword)count;
	PARAM(3) = (qword)file;

	return (dword)executeSyscall(SYSCALL_FREAD, &paramTable);
}

dword fwrite(const void* buffer, dword size, dword count, File* file) {
	ParamTable paramTable;

	PARAM(0) = (qword)buffer;
	PARAM(1) = (qword)size;
	PARAM(2) = (qword)count;
	PARAM(3) = (qword)file;

	return (dword)executeSyscall(SYSCALL_FWRITE, &paramTable);
}

int fseek(File* file, int offset, int origin) {
	ParamTable paramTable;

	PARAM(0) = (qword)file;
	PARAM(1) = (qword)offset;
	PARAM(2) = (qword)origin;

	return (int)executeSyscall(SYSCALL_FSEEK, &paramTable);
}

int fclose(File* file) {
	ParamTable paramTable;

	PARAM(0) = (qword)file;

	return (int)executeSyscall(SYSCALL_FCLOSE, &paramTable);
}

int remove(const char* fileName) {
	ParamTable paramTable;

	PARAM(0) = (qword)fileName;

	return (int)executeSyscall(SYSCALL_REMOVE, &paramTable);
}

Dir* opendir(const char* dirName) {
	ParamTable paramTable;

	PARAM(0) = (qword)dirName;

	return (Dir*)executeSyscall(SYSCALL_OPENDIR, &paramTable);
}

dirent* readdir(Dir* dir) {
	ParamTable paramTable;

	PARAM(0) = (qword)dir;

	return (dirent*)executeSyscall(SYSCALL_READDIR, &paramTable);
}

void rewinddir(Dir* dir) {
	ParamTable paramTable;

	PARAM(0) = (qword)dir;

	executeSyscall(SYSCALL_REWINDDIR, &paramTable);
}

int closedir(Dir* dir) {
	ParamTable paramTable;

	PARAM(0) = (qword)dir;

	return (int)executeSyscall(SYSCALL_CLOSEDIR, &paramTable);
}

bool isfopen(const dirent* entry) {
	ParamTable paramTable;

	PARAM(0) = (qword)entry;

	return (bool)executeSyscall(SYSCALL_ISFOPEN, &paramTable);
}

void sendSerialData(byte* buffer, int size) {
	ParamTable paramTable;

	PARAM(0) = (qword)buffer;
	PARAM(1) = (qword)size;

	executeSyscall(SYSCALL_SENDSERIALDATA, &paramTable);
}

int recvSerialData(byte* buffer, int size) {
	ParamTable paramTable;

	PARAM(0) = (qword)buffer;
	PARAM(1) = (qword)size;

	return (int)executeSyscall(SYSCALL_RECVSERIALDATA, &paramTable);
}

void clearSerialFifo(void) {
	executeSyscall(SYSCALL_CLEARSERIALFIFO, null);
}

int k_getProcessorCount(void) {
	return (int)executeSyscall(SYSCALL_GETPROCESSORCOUNT, null);
}

byte k_getApicId(void) {
	return (byte)executeSyscall(SYSCALL_GETAPICID, null);
}

qword getBackgroundWindowId(void) {
	return executeSyscall(SYSCALL_GETBACKGROUNDWINDOWID, null);
}

void getScreenArea(Rect* screenArea) {
	ParamTable paramTable;

	PARAM(0) = (qword)screenArea;

	executeSyscall(SYSCALL_GETSCREENAREA, &paramTable);
}

qword createWindow(int x, int y, int width, int height, dword flags, const char* title, Color backgroundColor, Menu* topMenu, void* widget, qword parentId) {
	ParamTable paramTable;

	PARAM(0) = (qword)x;
	PARAM(1) = (qword)y;
	PARAM(2) = (qword)width;
	PARAM(3) = (qword)height;
	PARAM(4) = (qword)flags;
	PARAM(5) = (qword)title;
	PARAM(6) = (qword)backgroundColor;
	PARAM(7) = (qword)topMenu;
	PARAM(8) = (qword)widget;
	PARAM(9) = parentId;

	return executeSyscall(SYSCALL_CREATEWINDOW, &paramTable);
}

bool deleteWindow(qword windowId) {
	ParamTable paramTable;

	PARAM(0) = windowId;

	return (bool)executeSyscall(SYSCALL_DELETEWINDOW, &paramTable);
}

bool isWindowShown(qword windowId) {
	ParamTable paramTable;

	PARAM(0) = windowId;

	return (bool)executeSyscall(SYSCALL_ISWINDOWSHOWN, &paramTable);
}

bool showWindow(qword windowId, bool show) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)show;

	return (bool)executeSyscall(SYSCALL_SHOWWINDOW, &paramTable);
}

qword findWindowByPoint(int x, int y) {
	ParamTable paramTable;

	PARAM(0) = (qword)x;
	PARAM(1) = (qword)y;

	return executeSyscall(SYSCALL_FINDWINDOWBYPOINT, &paramTable);
}

qword findWindowByTitle(const char* title) {
	ParamTable paramTable;

	PARAM(0) = (qword)title;

	return executeSyscall(SYSCALL_FINDWINDOWBYTITLE, &paramTable);
}

bool existWindow(qword windowId) {
	ParamTable paramTable;

	PARAM(0) = windowId;

	return (bool)executeSyscall(SYSCALL_EXISTWINDOW, &paramTable);
}

qword getTopWindowId(void) {
	return executeSyscall(SYSCALL_GETTOPWINDOWID, null);
}

bool isTopWindow(qword windowId) {
	ParamTable paramTable;

	PARAM(0) = windowId;

	return (bool)executeSyscall(SYSCALL_ISTOPWINDOW, &paramTable);
}

bool isTopWindowWithChild(qword windowId) {
	ParamTable paramTable;

	PARAM(0) = windowId;

	return (bool)executeSyscall(SYSCALL_ISTOPWINDOWWITHCHILD, &paramTable);	
}

bool moveWindowToTop(qword windowId) {
	ParamTable paramTable;

	PARAM(0) = windowId;

	return (bool)executeSyscall(SYSCALL_MOVEWINDOWTOTOP, &paramTable);
}

bool isPointInTitleBar(qword windowId, int x, int y) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)x;
	PARAM(2) = (qword)y;

	return (bool)executeSyscall(SYSCALL_ISPOINTINTITLEBAR, &paramTable);
}

bool isPointInCloseButton(qword windowId, int x, int y) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)x;
	PARAM(2) = (qword)y;

	return (bool)executeSyscall(SYSCALL_ISPOINTINCLOSEBUTTON, &paramTable);
}

bool isPointInResizeButton(qword windowId, int x, int y) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)x;
	PARAM(2) = (qword)y;

	return (bool)executeSyscall(SYSCALL_ISPOINTINRESIZEBUTTON, &paramTable);	
}

bool isPointInTopMenu(qword windowId, int x, int y) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)x;
	PARAM(2) = (qword)y;

	return (bool)executeSyscall(SYSCALL_ISPOINTINTOPMENU, &paramTable);
}

bool moveWindow(qword windowId, int x, int y) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)x;
	PARAM(2) = (qword)y;

	return (bool)executeSyscall(SYSCALL_MOVEWINDOW, &paramTable);
}

bool resizeWindow(qword windowId, int x, int y, int width, int height) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)x;
	PARAM(2) = (qword)y;
	PARAM(3) = (qword)width;
	PARAM(4) = (qword)height;

	return (bool)executeSyscall(SYSCALL_RESIZEWINDOW, &paramTable);
}

void moveChildWindows(qword windowId, int moveX, int moveY) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)moveX;
	PARAM(2) = (qword)moveY;

	executeSyscall(SYSCALL_MOVECHILDWINDOWS, &paramTable);
}

void showChildWindows(qword windowId, bool show, dword flags, bool parentToTop) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)show;
	PARAM(2) = (qword)flags;
	PARAM(3) = (qword)parentToTop;

	executeSyscall(SYSCALL_SHOWCHILDWINDOWS, &paramTable);
}

void deleteChildWindows(qword windowId) {
	ParamTable paramTable;

	PARAM(0) = windowId;

	executeSyscall(SYSCALL_DELETECHILDWINDOWS, &paramTable);
}

void* k_getNthChildWidget(qword windowId, int n) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)n;

	return (void*)executeSyscall(SYSCALL_GETNTHCHILDWIDGET, &paramTable);
}

bool getWindowArea(qword windowId, Rect* area) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)area;

	return (bool)executeSyscall(SYSCALL_GETWINDOWAREA, &paramTable);
}

bool sendEventToWindow(const Event* event, qword windowId) {
	ParamTable paramTable;

	PARAM(0) = (qword)event;
	PARAM(1) = (qword)windowId;

	return (bool)executeSyscall(SYSCALL_SENDEVENTTOWINDOW, &paramTable);
}

bool recvEventFromWindow(Event* event, qword windowId) {
	ParamTable paramTable;

	PARAM(0) = (qword)event;
	PARAM(1) = windowId;

	return (bool)executeSyscall(SYSCALL_RECVEVENTFROMWINDOW, &paramTable);
}

bool updateScreenById(qword windowId) {
	ParamTable paramTable;

	PARAM(0) = windowId;

	return (bool)executeSyscall(SYSCALL_UPDATESCREENBYID, &paramTable);
}

bool updateScreenByWindowArea(qword windowId, const Rect* area) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)area;

	return (bool)executeSyscall(SYSCALL_UPDATESCREENBYWINDOWAREA, &paramTable);
}

bool updateScreenByScreenArea(const Rect* area) {
	ParamTable paramTable;

	PARAM(0) = (qword)area;

	return (bool)executeSyscall(SYSCALL_UPDATESCREENBYSCREENAREA, &paramTable);
}

bool drawWindowBackground(qword windowId) {
	ParamTable paramTable;

	PARAM(0) = windowId;

	return (bool)executeSyscall(SYSCALL_DRAWWINDOWBACKGROUND, &paramTable);
}

bool drawWindowFrame(qword windowId) {
	ParamTable paramTable;

	PARAM(0) = windowId;

	return (bool)executeSyscall(SYSCALL_DRAWWINDOWFRAME, &paramTable);
}

bool drawWindowTitleBar(qword windowId, bool selected) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)selected;

	return (bool)executeSyscall(SYSCALL_DRAWWINDOWTITLEBAR, &paramTable);
}

bool drawPixel(qword windowId, int x, int y, Color color) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)x;
	PARAM(2) = (qword)y;
	PARAM(3) = (qword)color;

	return (bool)executeSyscall(SYSCALL_DRAWPIXEL, &paramTable);
}

bool drawLine(qword windowId, int x1, int y1, int x2, int y2, Color color) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)x1;
	PARAM(2) = (qword)y1;
	PARAM(3) = (qword)x2;
	PARAM(4) = (qword)y2;
	PARAM(5) = (qword)color;

	return (bool)executeSyscall(SYSCALL_DRAWLINE, &paramTable);
}

bool drawRect(qword windowId, int x1, int y1, int x2, int y2, Color color, bool fill) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)x1;
	PARAM(2) = (qword)y1;
	PARAM(3) = (qword)x2;
	PARAM(4) = (qword)y2;
	PARAM(5) = (qword)color;
	PARAM(6) = (qword)fill;

	return (bool)executeSyscall(SYSCALL_DRAWRECT, &paramTable);
}

bool drawCircle(qword windowId, int x, int y, int radius, Color color, bool fill) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)x;
	PARAM(2) = (qword)y;
	PARAM(3) = (qword)radius;
	PARAM(4) = (qword)color;
	PARAM(5) = (qword)fill;

	return (bool)executeSyscall(SYSCALL_DRAWCIRCLE, &paramTable);
}

bool drawText(qword windowId, int x, int y, Color textColor, Color backgroundColor, const char* str, int len) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)x;
	PARAM(2) = (qword)y;
	PARAM(3) = (qword)textColor;
	PARAM(4) = (qword)backgroundColor;
	PARAM(5) = (qword)str;
	PARAM(6) = (qword)len;

	return (bool)executeSyscall(SYSCALL_DRAWTEXT, &paramTable);
}

bool bitblt(qword windowId, int x, int y, const Color* buffer, int width, int height) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)x;
	PARAM(2) = (qword)y;
	PARAM(3) = (qword)buffer;
	PARAM(4) = (qword)width;
	PARAM(5) = (qword)height;

	return (bool)executeSyscall(SYSCALL_BITBLT, &paramTable);
}

void moveMouseCursor(int x, int y) {
	ParamTable paramTable;

	PARAM(0) = (qword)x;
	PARAM(1) = (qword)y;

	executeSyscall(SYSCALL_MOVEMOUSECURSOR, &paramTable);
}

void getMouseCursorPos(int* x, int* y) {
	ParamTable paramTable;

	PARAM(0) = (qword)x;
	PARAM(1) = (qword)y;

	executeSyscall(SYSCALL_GETMOUSECURSORPOS, &paramTable);
}


bool initJpeg(Jpeg* jpeg, const byte* fileBuffer, dword fileSize) {
	ParamTable paramTable;

	PARAM(0) = (qword)jpeg;
	PARAM(1) = (qword)fileBuffer;
	PARAM(2) = (qword)fileSize;

	return (bool)executeSyscall(SYSCALL_INITJPEG, &paramTable);
}

bool decodeJpeg(Jpeg* jpeg, Color* imageBuffer) {
	ParamTable paramTable;

	PARAM(0) = (qword)jpeg;
	PARAM(1) = (qword)imageBuffer;

	return (bool)executeSyscall(SYSCALL_DECODEJPEG, &paramTable);
}

qword getTotalRamSize(void) {
	return executeSyscall(SYSCALL_GETTOTALRAMSIZE, null);
}

qword getTickCount(void) {
	return executeSyscall(SYSCALL_GETTICKCOUNT, null);
}

void sleep(qword millisecond) {
	ParamTable paramTable;

	PARAM(0) = millisecond;

	executeSyscall(SYSCALL_SLEEP, &paramTable);
}

bool isGraphicMode(void) {
	return (bool)executeSyscall(SYSCALL_ISGRAPHICMODE, null);
}

qword executeApp(const char* fileName, const char* args, byte affinity) {
	ParamTable paramTable;

	PARAM(0) = (qword)fileName;
	PARAM(1) = (qword)args;
	PARAM(2) = (qword)affinity;

	return executeSyscall(SYSCALL_EXECUTEAPP, &paramTable);
}

bool createMenu(Menu* menu, int x, int y, int itemHeight, Color* colors, qword parentId, Menu* top, dword flags) {
	ParamTable paramTable;

	PARAM(0) = (qword)menu;
	PARAM(1) = (qword)x;
	PARAM(2) = (qword)y;
	PARAM(3) = (qword)itemHeight;
	PARAM(4) = (qword)colors;
	PARAM(5) = parentId;
	PARAM(6) = (qword)top;
	PARAM(7) = (qword)flags;

	return (bool)executeSyscall(SYSCALL_CREATEMENU, &paramTable);
}

bool processMenuEvent(Menu* menu) {
	ParamTable paramTable;

	PARAM(0) = (qword)menu;

	return (bool)executeSyscall(SYSCALL_PROCESSMENUEVENT, &paramTable);
}

bool drawHansLogo(qword windowId, int x, int y, int width, int height, Color brightColor, Color darkColor) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = x;
	PARAM(2) = y;
	PARAM(3) = width;
	PARAM(4) = height;
	PARAM(5) = brightColor;
	PARAM(6) = darkColor;

	return (bool)executeSyscall(SYSCALL_DRAWHANSLOGO, &paramTable);
}

bool drawButton(qword windowId, const Rect* buttonArea, Color textColor, Color backgroundColor, const char* text, dword flags) {
	ParamTable paramTable;

	PARAM(0) = windowId;
	PARAM(1) = (qword)buttonArea;
	PARAM(2) = (qword)backgroundColor;
	PARAM(3) = (qword)text;
	PARAM(4) = (qword)textColor;
	PARAM(5) = (qword)flags;

	return (bool)executeSyscall(SYSCALL_DRAWBUTTON, &paramTable);
}

void setClock(Clock* clock, qword windowId, int x, int y, Color textColor, Color backgroundColor, byte format, bool reset) {
	ParamTable paramTable;

	PARAM(0) = (qword)clock;
	PARAM(1) = windowId;
	PARAM(2) = (qword)x;
	PARAM(3) = (qword)y;
	PARAM(4) = (qword)textColor;
	PARAM(5) = (qword)backgroundColor;
	PARAM(6) = (qword)format;
	PARAM(7) = (qword)reset;

	executeSyscall(SYSCALL_SETCLOCK, &paramTable);
}

void addClock(Clock* clock) {
	ParamTable paramTable;

	PARAM(0) = (qword)clock;

	executeSyscall(SYSCALL_ADDCLOCK, &paramTable);
}

Clock* removeClock(qword clockId) {
	ParamTable paramTable;

	PARAM(0) = clockId;

	return (Clock*)executeSyscall(SYSCALL_REMOVECLOCK, &paramTable);
}

qword allocKid(void) {
	return executeSyscall(SYSCALL_ALLOCKID, null);
}

void freeKid(qword id) {
	ParamTable paramTable;

	PARAM(0) = id;

	executeSyscall(SYSCALL_FREEKID, &paramTable);
}
