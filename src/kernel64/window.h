#ifndef __WINDOW_H__
#define __WINDOW_H__

#include "types.h"
#include "sync.h"
#include "2d_graphics.h"
#include "list.h"

/**
  < GUI System Structure >
                              window manager                                           process1
             ---------------------------------------------------          ------------------------------------
             |                window list                      | keyboard |  window1              task1      |
             |          (connected by z-order)                 | ,mouse   | ----------        -------------- |
             |           ----           ----                   | ,window  | | event  | event  |            | |
             |           |  | -> ... -> |  |                   | event    | | queue  | -----> | event      | |
  screen     |           ----           ----                   | -------> | ----------        | processing | |
  ------     |                                                 | <------- | | window | <----- | loop       | |
  |    | <-> |       window manager task                       | update   | | buffer | update |            | |
  |    |     |  -----------------------------        --------- | screen   | ---------- screen -------------- |
  ------     |  | manage window info        | <----- | event | | event    |                                  |
             |  | send keyboard/mouse event | update | queue | |          |  window2   ----->                |
             |  | update screen             | screen --------- |          |            <-----                |
             |  -----------------------------                  |          |  ...                             |
             ---------------------------------------------------          ------------------------------------
                              |      |
                            -----  -----                         ------->              process2
                            |   |  |   |                         <-------
                            -----  -----                                               ...
                         keyboard  mouse
*/

// etc macros
#define WINDOW_MAXCOUNT       2048
#define WINDOW_MAXTITLELENGTH 40 // exclude last null character
#define WINDOW_INVALIDID      0xFFFFFFFFFFFFFFFF

// window flags
#define WINDOW_FLAGS_SHOW         0x00000001
#define WINDOW_FLAGS_DRAWFRAME    0x00000002
#define WINDOW_FLAGS_DRAWTITLEBAR 0x00000004
#define WINDOW_FLAGS_DEFAULT      (WINDOW_FLAGS_SHOW | WINDOW_FLAGS_DRAWFRAME | WINDOW_FLAGS_DRAWTITLEBAR)

// title bar-related macros
#define WINDOW_TITLEBAR_HEIGHT 21 // title bar height
#define WINDOW_XBUTTON_SIZE    19 // close button size

// window color
#define WINDOW_COLOR_FRAME              RGB(109, 218, 22)
#define WINDOW_COLOR_BACKGROUND         RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARTEXT       RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARBACKGROUND RGB(79, 204, 11)
#define WINDOW_COLOR_TITLEBARBRIGHT1    RGB(183, 249, 171) // bright color
#define WINDOW_COLOR_TITLEBARBRIGHT2    RGB(150, 210, 140) // bright color
#define WINDOW_COLOR_TITLEBARUNDERLINE  RGB(46, 59, 30)    // dark color
#define WINDOW_COLOR_BUTTONBRIGHT       RGB(229, 229, 229) // bright color
#define WINDOW_COLOR_BUTTONDARK         RGB(86, 86, 86)    // dark color
#define WINDOW_COLOR_SYSTEMBACKGROUND   RGB(232, 255, 232)
#define WINDOW_COLOR_XBUTTONLINE        RGB(71, 199, 21)

// background window tile
#define WINDOW_BACKGROUNDWINDOWTILE "SYS_BACKGROUND"

// mouse cursor width and height
#define MOUSE_CURSOR_WIDTH  20
#define MOUSE_CURSOR_HEIGHT 20

// mouse cursor color
#define MOUSE_CURSOR_COLOR_OUTERLINE RGB(0, 0, 0)       // black color, represent 1 in bitmap
#define MOUSE_CURSOR_COLOR_OUTER     RGB(79, 204, 11)   // dark green color, represent 2 in bitmap
#define MOUSE_CURSOR_COLOR_INNER     RGB(232, 255, 232) // bright color, represent 3 in bitmap

// macro function
#define GETWINDOWOFFSET(windowId) ((windowId) & 0xFFFFFFFF) // get low 32 bits of window.link.id (64 bits)

#pragma pack(push, 1)

typedef struct k_Window {
	ListLink link; // window link: It consists of next window address (link.next) and window ID (link.id).
	               //              window ID consists of window allocate count (high 32 bits) and window offset (low 32 bits).
	               //              [Note] ListLink must be the first field.
	Mutex mutex;   // mutex
	Rect area;     // window area (screen coordinates)
	Color* buffer; // window buffer (window coordinates) address
	qword taskId;  // window-creating task ID
	dword flags;   // window flags: bit 0 is show flag.
	               //               bit 1 is draw frame flag.
	               //               bit 2 is draw title bar flag.
	char title[WINDOW_MAXTITLELENGTH + 1]; // window title: include last null character
} Window; // Window is ListItem.

typedef struct k_WindowPoolManager {
	Mutex mutex;       // mutex
	Window* startAddr; // start address of window pool: You can consider it as window array.
	int maxCount;      // window max count
	int useCount;      // window use count: currently being-used window count.
	int allocCount;    // window allocate count: It only increases when allocating window. It's for window ID to be unique.
} WindowPoolManager;

typedef struct k_WindowManager {
	Mutex mutex;     // mutex
	List windowList; // window list: connected by z-order. 
	                 //              Getting closer to the tail in window list means getting closer to the top in screen.
	                 //              (head -> tail == the bottom window -> the top window)
	int mouseX;      // mouse x (screen coordinates)
	int mouseY;      // mouse y (screen coordinates)
	Rect screenArea; // screen area (screen coordinates)
	Color* videoMem; // video memory (screen coordinates) address
	qword backgroundWindowId; // background window ID
} WindowManager;

#pragma pack(pop)

/* Window Pool-related Functions */
static void k_initWindowPool(void);
static Window* k_allocWindow(void);
static void k_freeWindow(qword windowId);

/* Window and Window Manager-related Functions */
void k_initGui(void);
WindowManager* k_getWindowManager(void);
qword k_getBackgroundWindowId(void);
void k_getScreenArea(Rect* screenArea);
qword k_createWindow(int x, int y, int width, int height, dword flags, const char* title);
bool k_deleteWindow(qword windowId);
bool k_deleteAllWindowsByTask(qword taskId);
Window* k_getWindow(qword windowId);
Window* k_getWindowWithLock(qword windowId);
bool k_showWindow(qword windowId, bool show);
bool k_redrawWindowByArea(const Rect* area);
static void k_copyWindowBufferToVideoMem(const Window* window, const Rect* copyArea);

/* Window Drawing Functions: draw objects in window buffer using window coordinates */
bool k_drawWindowFrame(qword windowId);
bool k_drawWindowBackground(qword windowId);
bool k_drawWindowTitleBar(qword windowId, const char* title);
bool k_drawButton(qword windowId, const Rect* buttonArea, Color backgroundColor, const char* text, Color textColor);
bool k_drawPixel(qword windowId, int x, int y, Color color);
bool k_drawLine(qword windowId, int x1, int y1, int x2, int y2, Color color);
bool k_drawRect(qword windowId, int x1, int y1, int x2, int y2, Color color, bool fill);
bool k_drawCircle(qword windowId, int x, int y, int radius, Color color, bool fill);
bool k_drawText(qword windowId, int x, int y, Color textColor, Color backgroundColor, const char* str);

/* Mouse Cursor-related Functions */
static void k_drawMouseCursor(int x, int y);
void k_moveMouseCursor(int x, int y);
void k_getMouseCursorPos(int* x, int* y);

#endif // __WINDOW_H__