#ifndef __CORE_WINDOW_H__
#define __CORE_WINDOW_H__

#include "types.h"
#include "2d_graphics.h"
#include "sync.h"
#include "../utils/list.h"
#include "../utils/queue.h"
#include "keyboard.h"
#include "widgets.h"

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
#define WINDOW_FLAGS_RESIZABLE    0x00000008 // resizable flag
#define WINDOW_FLAGS_BLOCKING     0x00000010 // blocking flag
#define WINDOW_FLAGS_HASCHILD     0x00000020 // has child flag: set in k_createWindow.
#define WINDOW_FLAGS_CHILD        0x00000040 // child flag
#define WINDOW_FLAGS_MENU         0x00000080 // menu flag: set in k_createMenu. The top menu is not a menu.
#define WINDOW_FLAGS_DEFAULT      (WINDOW_FLAGS_SHOW | WINDOW_FLAGS_DRAWFRAME | WINDOW_FLAGS_DRAWTITLEBAR)

// window size
#define WINDOW_TITLEBAR_HEIGHT  21 // title bar height
#define WINDOW_XBUTTON_SIZE     19 // close button and resize button size
#define WINDOW_MINWIDTH         (WINDOW_XBUTTON_SIZE * 2 + 30) // min window width
#define WINDOW_MINHEIGHT        (WINDOW_TITLEBAR_HEIGHT + 30)  // min window height
#define WINDOW_SYSMENU_HEIGHT   31

// window color
#define WINDOW_COLOR_BACKGROUND                 RGB(255, 255, 255)
#define WINDOW_COLOR_FRAME                      RGB(33, 147, 176)
#define WINDOW_COLOR_TITLEBARBACKGROUNDACTIVE   RGB(33, 147, 176)
#define WINDOW_COLOR_TITLEBARBACKGROUNDINACTIVE RGB(167, 173, 186)
#define WINDOW_COLOR_TITLEBARTEXTACTIVE         RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARTEXTACTIVEWITHMENU RGB(0, 255, 0)
#define WINDOW_COLOR_TITLEBARTEXTINACTIVE       RGB(255, 255, 255)
#define WINDOW_COLOR_XBUTTONBACKGROUNDACTIVE    RGB(33, 147, 176)
#define WINDOW_COLOR_XBUTTONBACKGROUNDINACTIVE  RGB(167, 173, 186)
#define WINDOW_COLOR_XBUTTONMARKACTIVE          RGB(222, 98, 98)          
#define WINDOW_COLOR_XBUTTONMARKINACTIVE        RGB(255, 255, 255)
#define WINDOW_COLOR_BUTTONDARK                 RGB(86, 86, 86)
#define WINDOW_COLOR_SYSBACKGROUND              RGB(255, 236, 210)
#define WINDOW_COLOR_SYSBACKGROUNDMARKBRIGHT    RGB(255, 255, 255)
#define WINDOW_COLOR_SYSBACKGROUNDMARKDARK      RGB(252, 182, 159)

// background window title
#define WINDOW_SYSBACKGROUND_TITLE "SYS_BACKGROUND"

// max copied area array count
#define WINDOW_MAXCOPIEDAREAARRAYCOUNT 20

// button flags
#define BUTTON_FLAGS_SHADOW 0x00000001 // 0: not draw shadow, 1: draw shadow

// mouse cursor width and height
#define MOUSE_CURSOR_WIDTH  10
#define MOUSE_CURSOR_HEIGHT 18

// mouse cursor color
#define MOUSE_CURSOR_COLOR_OUTER RGB(255, 255, 255)
#define MOUSE_CURSOR_COLOR_INNER RGB(0, 0, 0)

// resize marker-related macros
#define RESIZEMARKER_SIZE  20
#define RESIZEMARKER_THICK 4
#define RESIZEMARKER_COLOR RGB(222, 98, 98)

// event queue-related macros
#define EVENTQUEUE_WINDOW_MAXCOUNT        100             // window event queue max count
#define EVENTQUEUE_WINDOWMANAGER_MAXCOUNT WINDOW_MAXCOUNT // window manager event queue max count

/**
  < Event Classification >
  @ window event
  - mouse event
  - window event
  - key event
  - user event

  @ window manager event
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
#define EVENT_MOUSE_OUT         8

// window event
#define EVENT_WINDOW_SELECT   9
#define EVENT_WINDOW_DESELECT 10
#define EVENT_WINDOW_MOVE     11
#define EVENT_WINDOW_RESIZE   12
#define EVENT_WINDOW_CLOSE    13

// key event
#define EVENT_KEY_DOWN 14
#define EVENT_KEY_UP   15

// top menu event
#define EVENT_TOPMENU_CLICK 16

// screen update event
#define EVENT_SCREENUPDATE_BYID         17 // Window.area (screen coordinates)
#define EVENT_SCREENUPDATE_BYWINDOWAREA 18 // ScreenUpdateEvent.area (window coordinates)
#define EVENT_SCREENUPDATE_BYSCREENAREA 19 // ScreenUpdateEvent.area (screen coordinates)

/* macro function */
#define GETWINDOWOFFSET(windowId) ((windowId) & 0xFFFFFFFF)                             // get window offset (low 32 bits) of window.link.id (64 bits).
#define GETWINDOWFROMCHILDLINK(x) ((Window*)((qword)(x) - offsetof(Window, childLink))) // get window address from window.childLink address.

#pragma pack(push, 1)

// mouse event: window manager -> window
typedef struct k_MouseEvent {
	qword windowId;    // window ID to send event
	                   //   : window ID is not necessarily required for mouse event.
	Point point;       // mouse point (window coordinates)
	byte buttonStatus; // mouse button status
} MouseEvent;

// window event: window manager -> window
// screen update event: window -> window manager
typedef struct k_WindowEvent {
	qword windowId; // window ID to send event
	                //   : window ID is not necessarily required for window event.
	                //     But, window ID is necessarily required for screen update event.
	Rect area;      // window area: - window event (screen coordinates)
	                //              - screen update by ID event (not use this area, but use Window.area)
	                //              - screen update by window area event (window coordinates)
	                //              - screen update by screen area event (screen coordinates)
} WindowEvent, ScreenUpdateEvent;

// key event: window manager -> window
typedef struct k_KeyEvent {
	qword windowId; // window ID to send event
                    //   : window ID is not necessarily required for key event.
	byte scanCode;  // key scan code
	byte asciiCode; // key ASCII code
	byte flags;     // key flags
} KeyEvent;

// menu event: window manager -> window
typedef struct k_MenuEvent {
	qword windowId; // window ID to send event
	                //   : window ID is not necessarily required for menu event.
	int index;      // menu item index
} MenuEvent;

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
		MenuEvent menuEvent;                 // menu event
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
	//--------------------------------------------------
	// Window-related Fields
	//--------------------------------------------------
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
	                    //               bit 3 : resizable flag
						//               bit 4 : blocking flag
	                    //               bit 5 : has child flag
	                    //               bit 6 : child flag
	                    //               bit 7 : menu flag
	Queue eventQueue;   // event queue for mouse, window, key, user event
	Event* eventBuffer; // event buffer
	char title[WINDOW_MAXTITLELENGTH + 1]; // window title: include last null character
	Color backgroundColor; // background color
	Menu* topMenu;         // top menu

	//--------------------------------------------------
	// Child-related Fields
	//--------------------------------------------------
	ListLink childLink; // child link
	qword parentId;     // parent ID: Only child window has valid parent ID.
	List childList;     // child list: [NOTE] Child window has no title bar.
} Window; // Window is ListItem.

typedef struct k_WindowPoolManager {
	Mutex mutex;        // mutex
	Window* startAddr;  // start address of window pool: You can consider it as window array.
	int maxCount;       // max window count
	int usedCount;      // used window count: currently being-used window count.
	int allocatedCount; // allocated window count: It only increases when allocating window. It's for window ID to be unique.
} WindowPoolManager;

typedef struct k_WindowManager {
	Mutex mutex;            // mutex
	List windowList;        // window list: connected by z-order. 
	                        //              head -> tail == the top window -> the bottom window
	int mouseX;             // mouse x (screen coordinates): always inside screen
	int mouseY;             // mouse y (screen coordinates): always inside screen
	Rect screenArea;        // screen area (screen coordinates)
	Color* videoMem;        // video memory (screen coordinates) address
	qword backgroundId;     // system background window ID
	Queue eventQueue;       // event queue for screen update event
	Event* eventBuffer;     // event buffer
	byte* screenBitmap;     // screen bitmap
	byte prevButtonStatus;  // previous mouse button status
	qword prevUnderMouseId; // previous under mouse window ID
	bool moving;            // moving window flag
	qword movingId;         // moving window ID
	bool resizing;          // resizing window flag
	qword resizingId;       // resizing window ID
	Rect resizingArea;      // resizing window area
	qword overCloseId;      // mouse over close button window ID
	qword overResizeId;     // mouse over resize button window ID
	qword overMenuId;       // mouse over top menu window ID
} WindowManager;

#pragma pack(pop)

/* Window Pool Functions */
static void k_initWindowPool(void);
static Window* k_allocWindow(void);
static void k_freeWindow(qword windowId);

/* Window and Window Manager Functions */
void k_initGuiSystem(void);
WindowManager* k_getWindowManager(void);
qword k_getBackgroundWindowId(void);
void k_getScreenArea(Rect* screenArea);
qword k_createWindow(int x, int y, int width, int height, dword flags, const char* title, Color backgroundColor, Menu* topMenu, qword parentId);
bool k_deleteWindow(qword windowId);
bool k_deleteWindowsByTask(qword taskId);
bool k_closeWindowsByTask(qword taskId);
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
bool k_isTopWindow(qword windowId);
bool k_isTopWindowWithChild(qword windowId);
bool k_moveWindowToTop(qword windowId);
bool k_isPointInTitleBar(qword windowId, int x, int y);
bool k_isPointInCloseButton(qword windowId, int x, int y);
bool k_isPointInResizeButton(qword windowId, int x, int y);
bool k_isPointInTopMenu(qword windowId, int x, int y);
static bool k_updateWindowTitleBar(qword windowId, bool selected);
bool k_updateCloseButton(qword windowId, bool mouseOver);
bool k_updateResizeButton(qword windowId, bool mouseOver);
bool k_moveWindow(qword windowId, int x, int y);
bool k_resizeWindow(qword windowId, int x, int y, int width, int height);
void k_moveChildWindows(qword windowId, int moveX, int moveY);
void k_showChildWindows(qword windowId, bool show, dword flags);
void k_deleteChildWindows(qword windowId);

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
bool k_setMenuEvent(Event* event, qword windowId, qword eventType, int mouseX, int mouseY);
bool k_sendEventToWindow(const Event* event, qword windowId);
bool k_recvEventFromWindow(Event* event, qword windowId);
bool k_sendEventToWindowManager(const Event* event);
bool k_recvEventFromWindowManager(Event* event);
bool k_sendMouseEventToWindow(qword windowId, qword eventType, int mouseX, int mouseY, byte buttonStatus);
bool k_sendWindowEventToWindow(qword windowId, qword eventType);
bool k_sendKeyEventToWindow(qword windowId, const Key* key);
bool k_sendMenuEventToWindow(qword windowId, qword eventType, int mouseX, int mouseY);

/* Screen Update Functions */
bool k_updateScreenById(qword windowId); // screen coordinates
bool k_updateScreenByWindowArea(qword windowId, const Rect* area); // window coordinates
bool k_updateScreenByScreenArea(const Rect* area); // screen coordinates

/* Window Drawing Functions: draw objects in window buffer using window coordinates */
bool k_drawWindowBackground(qword windowId);
bool k_drawWindowFrame(qword windowId);
bool k_drawWindowTitleBar(qword windowId, bool selected);
static bool k_drawCloseButton(qword windowId, bool selected, bool mouseOver);
static bool k_drawResizeButton(qword windowId, bool selected, bool mouseOver);
bool k_drawButton(qword windowId, const Rect* buttonArea, Color textColor, Color backgroundColor, const char* text, dword flags);
bool k_drawPixel(qword windowId, int x, int y, Color color);
bool k_drawLine(qword windowId, int x1, int y1, int x2, int y2, Color color);
bool k_drawRect(qword windowId, int x1, int y1, int x2, int y2, Color color, bool fill);
bool k_drawCircle(qword windowId, int x, int y, int radius, Color color, bool fill);
bool k_drawText(qword windowId, int x, int y, Color textColor, Color backgroundColor, const char* str, int len);
bool k_bitblt(qword windowId, int x, int y, const Color* buffer, int width, int height);
static void k_drawBackgroundMark(void);
static void k_drawBackgroundImage(void);

/* Mouse Cursor Functions */
static void k_drawMouseCursor(int x, int y); // draw mouse cursor in video memory using screen coordinates.  
void k_moveMouseCursor(int x, int y); // screen coordinates
void k_getMouseCursorPos(int* x, int* y); // screen coordinates

/* Resize Marker Functions */
void k_drawResizeMarker(const Rect* area, bool show); // draw resize marker in video memory using screen coordinates.

#endif // __CORE_WINDOW_H__