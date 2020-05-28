#ifndef __TYPES_WINDOW_H__
#define __TYPES_WINDOW_H__

#include "types.h"
#include "2d_graphics.h"

// etc macros
#define WINDOW_MAXTITLELENGTH 23   // max title length: exclude last null character
#define WINDOW_INVALIDID      0xFFFFFFFFFFFFFFFF

// window flags
#define WINDOW_FLAGS_SHOW         0x00000001
#define WINDOW_FLAGS_DRAWFRAME    0x00000002
#define WINDOW_FLAGS_DRAWTITLEBAR 0x00000004
#define WINDOW_FLAGS_RESIZABLE    0x00000008
#define WINDOW_FLAGS_BLOCKING     0x00000010
#define WINDOW_FLAGS_HASCHILD     0x00000020
#define WINDOW_FLAGS_CHILD        0x00000040
#define WINDOW_FLAGS_VISIBLE      0x00000080
#define WINDOW_FLAGS_MENU         0x00000100
#define WINDOW_FLAGS_PANEL        0x00000200
#define WINDOW_FLAGS_DEFAULT      (WINDOW_FLAGS_SHOW | WINDOW_FLAGS_DRAWFRAME | WINDOW_FLAGS_DRAWTITLEBAR)

// window size
#define WINDOW_TITLEBAR_HEIGHT          21
#define WINDOW_XBUTTON_SIZE             19
#define WINDOW_MINWIDTH                 (WINDOW_XBUTTON_SIZE * 2 + 30)
#define WINDOW_MINHEIGHT                (WINDOW_TITLEBAR_HEIGHT + 30)
#define WINDOW_SYSMENU_HEIGHT           24
#define WINDOW_SYSBACKGROUND_LOGOWIDTH  60
#define WINDOW_SYSBACKGROUND_LOGOHEIGHT 60

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
#define WINDOW_COLOR_SYSBACKGROUNDLOGOBRIGHT    RGB(255, 255, 255)
#define WINDOW_COLOR_SYSBACKGROUNDLOGODARK      RGB(252, 182, 159)

// background window title
#define WINDOW_SYSBACKGROUND_TITLE "SYS_BACKGROUND"

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
#define GETWINDOWOFFSET(windowId) ((windowId) & 0xFFFFFFFF) // get window offset (low 32 bits) of window.link.id (64 bits).

#pragma pack(push, 1)

// mouse event: window manager -> window
typedef struct __MouseEvent {
	qword windowId;    // window ID to send event
	                   //   : window ID is not necessarily required for mouse event.
	Point point;       // mouse point (window coordinates)
	byte buttonStatus; // mouse button status
} MouseEvent;

// window event: window manager -> window
// screen update event: window -> window manager
typedef struct __WindowEvent {
	qword windowId; // window ID to send event
	                //   : window ID is not necessarily required for window event.
	                //     But, window ID is necessarily required for screen update event.
	Rect area;      // window area: - window event (screen coordinates)
	                //              - screen update by ID event (not use this area, but use Window.area)
	                //              - screen update by window area event (window coordinates)
	                //              - screen update by screen area event (screen coordinates)
} WindowEvent, ScreenUpdateEvent;

// key event: window manager -> window
typedef struct __KeyEvent {
	qword windowId; // window ID to send event
                    //   : window ID is not necessarily required for key event.
	byte scanCode;  // key scan code
	byte asciiCode; // key ASCII code
	byte flags;     // key flags
} KeyEvent;

// menu event: window manager -> window
typedef struct __MenuEvent {
	qword windowId; // window ID to send event
	                //   : window ID is not necessarily required for menu event.
	int index;      // menu item index
} MenuEvent;

// user event
typedef struct __UserEvent {
	qword data[3]; // user data
} UserEvent;

// event
typedef struct __Event {
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

#pragma pack(pop)

#endif // __TYPES_WINDOW_H__
