#ifndef __CORE_WINDOW_H__
#define __CORE_WINDOW_H__

#include "types.h"
#include "2d_graphics.h"
#include "sync.h"
#include "list.h"
#include "queue.h"
#include "keyboard.h"

/**
  < GUI System Structure >
                              window manager                                           GUI task1
             ----------------------------------------------------          ------------------------------------
             |                window list                       | mouse    |  window1              task1      |
             |          (connected by z-order)                  | ,window  | ----------        -------------- |
             |           ----           ----                    | ,key     | | event  | event  |            | |
             |           |  | -> ... -> |  |                    | event    | | queue  | -----> | event      | |
  screen     |           ----           ----                    | -------> | ----------        | processing | |
  ------     |                                                  | <------- | | window | <----- | loop       | |
  |    | <-> |       window manager task                        | screen   | | buffer | screen |            | |
  |    |     | -------------------------------        --------- | update   | ---------- update -------------- |
  ------     | | manage window info          | <----- | event | | event    |                                  |
             | | send mouse/window/key event | screen | queue | |          |  window2   ----->                |
             | | update screen               | update --------- |          |            <-----                |
             | ------------------------------- event            |          |  ...                             |
             ----------------------------------------------------          ------------------------------------
                              |      |
                            -----  -----                         ------->              GUI task2
                            |   |  |   |                         <-------
                            -----  -----                                               ...
                         keyboard  mouse
*/

// etc macros
#define WINDOW_MAXCOUNT       2048 // max window count: 2048 = 1024 (max task count) * 2
#define WINDOW_MAXTITLELENGTH 40   // max title length: exclude last null character
#define WINDOW_INVALIDID      0xFFFFFFFFFFFFFFFF

// window flags
#define WINDOW_FLAGS_SHOW         0x00000001 // show flag
#define WINDOW_FLAGS_DRAWFRAME    0x00000002 // draw frame flag
#define WINDOW_FLAGS_DRAWTITLEBAR 0x00000004 // draw title bar flag
#define WINDOW_FLAGS_DEFAULT      (WINDOW_FLAGS_SHOW | WINDOW_FLAGS_DRAWFRAME | WINDOW_FLAGS_DRAWTITLEBAR)

// title bar-related macros
#define WINDOW_TITLEBAR_HEIGHT  21 // title bar height
#define WINDOW_CLOSEBUTTON_SIZE 19 // close button size

// window color
#define WINDOW_COLOR_FRAME                      RGB(109, 218, 22)
#define WINDOW_COLOR_BACKGROUND                 RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARTEXT               RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARBACKGROUNDACTIVE   RGB(79, 204, 11)   // bright green
#define WINDOW_COLOR_TITLEBARBACKGROUNDINACTIVE RGB(55, 135, 11)   // dark green
#define WINDOW_COLOR_TITLEBARBRIGHT1            RGB(183, 249, 171) // bright color
#define WINDOW_COLOR_TITLEBARBRIGHT2            RGB(150, 210, 140) // bright color
#define WINDOW_COLOR_TITLEBARDARK               RGB(46, 59, 30)    // dark color
#define WINDOW_COLOR_BUTTONBRIGHT               RGB(229, 229, 229) // bright color
#define WINDOW_COLOR_BUTTONDARK                 RGB(86, 86, 86)    // dark color
#define WINDOW_COLOR_SYSTEMBACKGROUND           RGB(232, 255, 232) // brighter green
#define WINDOW_COLOR_CLOSEBUTTONMARK            RGB(71, 199, 21)   // 'X' mark on close button

// background window title
#define WINDOW_BACKGROUNDWINDOWTITLE "SYS_BACKGROUND"

// max copied area array count
#define WINDOW_MAXCOPIEDAREAARRAYCOUNT 20

// mouse cursor width and height
#define MOUSE_CURSOR_WIDTH  20
#define MOUSE_CURSOR_HEIGHT 20

// mouse cursor color
#define MOUSE_CURSOR_COLOR_OUTERLINE RGB(0, 0, 0)       // black, represent 1 in bitmap
#define MOUSE_CURSOR_COLOR_OUTER     RGB(79, 204, 11)   // bright green, represent 2 in bitmap
#define MOUSE_CURSOR_COLOR_INNER     RGB(232, 255, 232) // brighter green, represent 3 in bitmap

// event queue-related macros
#define EVENTQUEUE_WINDOW_MAXCOUNT        100             // window event queue max count
#define EVENTQUEUE_WINDOWMANAGER_MAXCOUNT WINDOW_MAXCOUNT // window manager event queue max count

/**
  < Event Classification >
  *** window event ***
  - mouse event
  - window event
  - key event
  - user event

  *** window manager event ***
  - screen update event
*/

/* event type */
// unknown event
#define EVENT_UNKNOWN 0

// mouse event
#define EVENT_MOUSE_MOVE        1
#define EVENT_MOUSE_LBUTTONDOWN 2
#define EVENT_MOUSE_LBUTTONUP   3
#define EVENT_MOUSE_RBUTTONDOWN 4
#define EVENT_MOUSE_RBUTTONUP   5
#define EVENT_MOUSE_MBUTTONDOWN 6
#define EVENT_MOUSE_MBUTTONUP   7

// window event
#define EVENT_WINDOW_SELECT   8
#define EVENT_WINDOW_DESELECT 9
#define EVENT_WINDOW_MOVE     10
#define EVENT_WINDOW_RESIZE   11
#define EVENT_WINDOW_CLOSE    12

// key event
#define EVENT_KEY_DOWN 13
#define EVENT_KEY_UP   14

// screen update event
#define EVENT_SCREENUPDATE_BYID         15 // Window.area (screen coordinates)
#define EVENT_SCREENUPDATE_BYWINDOWAREA 16 // ScreenUpdateEvent.area (window coordinates)
#define EVENT_SCREENUPDATE_BYSCREENAREA 17 // ScreenUpdateEvent.area (screen coordinates)

/* macro function */
#define GETWINDOWOFFSET(windowId) ((windowId) & 0xFFFFFFFF) // get low 32 bits of window.link.id (64 bits)

#pragma pack(push, 1)

// mouse event: window manager -> window
typedef struct k_MouseEvent {
	qword windowId;    // window ID to send event
	                   //     : window ID is not necessarily required to declare here, because window has mouse event.
	                   //       But, window ID is declared here for the management.
	Point point;       // mouse point (window coordinates)
	byte buttonStatus; // mouse button status
} MouseEvent;

// window event: window manager -> window
// screen update event: window -> window manager
typedef struct k_WindowEvent {
	qword windowId; // window ID to send event
	                //     : window ID is not necessarily required for window event.
	                //       But, window ID is necessarily required for screen update event.
	Rect area;      // window area: - window event (screen coordinates)
	                //              - screen update by ID event (not use this area, but use Window.area)
	                //              - screen update by window area event (window coordinates)
	                //              - screen update by screen area event (screen coordinates)
} WindowEvent, ScreenUpdateEvent;

// key event: window manager -> window
typedef struct k_KeyEvent {
	qword windowId; // window ID to send event
                    //     : window ID is not necessarily required to declare here, because window has key event.
                    //       But, window ID is declared here for the management.
	byte scanCode;  // key scan code
	byte asciiCode; // key ASCII code
	byte flags;     // key flags
} KeyEvent;

// user event
typedef struct k_UserEvent {
	qword data[3]; // user data
} UserEvent;

// event
typedef struct k_Event {
	qword type; // event type
	union {
		MouseEvent mouseEvent;               // mouse event
		WindowEvent windowEvent;             // window event
		KeyEvent keyEvent;                   // key event
		ScreenUpdateEvent screenUpdateEvent; // screen update event
		UserEvent userEvent;                 // user event
	};
} Event;

// screen bitmap (screen update bitmap)
typedef struct k_ScreenBitmap {
	Rect area;    // update area (screen coordinates)
	byte* bitmap; // bitmap: A bit in bitmap represents a pixel in update area.
	              //         - 1: on (to update)
	              //         - 0: off (updated)
} ScreenBitmap;

typedef struct k_Window {
	ListLink link;      // window link: It consists of next window address (link.next) and window ID (link.id).
	                    //              window ID consists of allocated window count (high 32 bits) and window offset (low 32 bits).
	                    //              [NOTE] ListLink must be the first field.
	Mutex mutex;        // mutex
	Rect area;          // window area (screen coordinates)
	Color* buffer;      // window buffer (window coordinates) address
	qword taskId;       // window-creating task ID
	dword flags;        // window flags: bit 0 : show flag
	                    //               bit 1 : draw frame flag
	                    //               bit 2 : draw title bar flag
	Queue eventQueue;   // event queue for mouse, window, key, user event
	Event* eventBuffer; // event buffer
	char title[WINDOW_MAXTITLELENGTH + 1]; // window title: include last null character
} Window; // Window is ListItem.

typedef struct k_WindowPoolManager {
	Mutex mutex;        // mutex
	Window* startAddr;  // start address of window pool: You can consider it as window array.
	int maxCount;       // max window count
	int usedCount;      // used window count: currently being-used window count.
	int allocatedCount; // allocated window count: It only increases when allocating window. It's for window ID to be unique.
} WindowPoolManager;

typedef struct k_WindowManager {
	Mutex mutex;              // mutex
	List windowList;          // window list: connected by z-order. 
	                          //              head -> tail == the top window -> the bottom window
	int mouseX;               // mouse x (screen coordinates): always inside screen
	int mouseY;               // mouse y (screen coordinates): always inside screen
	Rect screenArea;          // screen area (screen coordinates)
	Color* videoMem;          // video memory (screen coordinates) address
	qword backgroundWindowId; // background window ID
	Queue eventQueue;         // event queue for screen update event
	Event* eventBuffer;       // event buffer
	byte prevButtonStatus;    // previous mouse button status
	bool windowMoving;        // window moving flag
	qword movingWindowId;     // moving window ID
	byte* screenBitmap;       // screen bitmap
} WindowManager;

#pragma pack(pop)

/* Window Pool Functions */
static void k_initWindowPool(void);
static Window* k_allocWindow(void);
static void k_freeWindow(qword windowId);

/* Window and Window Manager Functions */
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
bool k_redrawWindowByArea(qword windowId, const Rect* area); // screen coordinates
static void k_copyWindowBufferToVideoMem(const Window* window, ScreenBitmap* bitmap);
bool k_createScreenBitmap(ScreenBitmap* bitmap, const Rect* area);
static bool k_fillScreenBitmap(const ScreenBitmap* bitmap, const Rect* area, bool on);
bool k_getStartOffsetInScreenBitmap(const ScreenBitmap* bitmap, int x, int y, int* byteOffset, int* bitOffset);
bool k_isScreenBitmapAllOff(const ScreenBitmap* bitmap);
qword k_findWindowByPoint(int x, int y);
qword k_findWindowByTitle(const char* title);
bool k_existWindow(qword windowId);
qword k_getTopWindowId(void);
bool k_moveWindowToTop(qword windowId);
bool k_isPointInTitleBar(qword windowId, int x, int y);
bool k_isPointInCloseButton(qword windowId, int x, int y);
bool k_moveWindow(qword windowId, int x, int y);
static bool k_updateWindowTitleBar(qword windowId, bool selected);

/* Coordinates Conversion Functions */
bool k_getWindowArea(qword windowId, Rect* area);
bool k_convertPointScreenToWindow(qword windowId, const Point* screenPoint, Point* windowPoint);
bool k_convertPointWindowToScreen(qword windowId, const Point* windowPoint, Point* screenPoint);
bool k_convertRectScreenToWindow(qword windowId, const Rect* screenRect, Rect* windowRect);
bool k_convertRectWindowToScreen(qword windowId, const Rect* windowRect, Rect* screenRect);

/* Event Functions */
bool k_setMouseEvent(Event* event, qword windowId, qword eventType, int mouseX, int mouseY, byte buttonStatus);
bool k_setWindowEvent(Event* event, qword windowId, qword eventType);
void k_setKeyEvent(Event* event, qword windowId, const Key* key);
bool k_sendEventToWindow(const Event* event, qword windowId);
bool k_recvEventFromWindow(Event* event, qword windowId);
bool k_sendEventToWindowManager(const Event* event);
bool k_recvEventFromWindowManager(Event* event);
bool k_sendMouseEventToWindow(qword windowId, qword eventType, int mouseX, int mouseY, byte buttonStatus);
bool k_sendWindowEventToWindow(qword windowId, qword eventType);
bool k_sendKeyEventToWindow(qword windowId, const Key* key);

/* Screen Update Functions */
bool k_updateScreenById(qword windowId); // screen coordinates
bool k_updateScreenByWindowArea(qword windowId, const Rect* area); // window coordinates 
bool k_updateScreenByScreenArea(const Rect* area); // screen coordinates

/* Window Drawing Functions: draw objects in window buffer using window coordinates */
bool k_drawWindowFrame(qword windowId);
bool k_drawWindowBackground(qword windowId);
bool k_drawWindowTitleBar(qword windowId, const char* title, bool selected);
bool k_drawButton(qword windowId, const Rect* buttonArea, Color backgroundColor, const char* text, Color textColor);
bool k_drawPixel(qword windowId, int x, int y, Color color);
bool k_drawLine(qword windowId, int x1, int y1, int x2, int y2, Color color);
bool k_drawRect(qword windowId, int x1, int y1, int x2, int y2, Color color, bool fill);
bool k_drawCircle(qword windowId, int x, int y, int radius, Color color, bool fill);
bool k_drawText(qword windowId, int x, int y, Color textColor, Color backgroundColor, const char* str);

/* Mouse Cursor Functions */
static void k_drawMouseCursor(int x, int y);
void k_moveMouseCursor(int x, int y);
void k_getMouseCursorPos(int* x, int* y);

#endif // __CORE_WINDOW_H__