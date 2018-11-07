#ifndef __TYPES_H__
#define __TYPES_H__

//----------------------------------------------------------------------------------------------------
// Macro from types.h
//----------------------------------------------------------------------------------------------------

#define byte  unsigned char
#define word  unsigned short
#define dword unsigned int
#define qword unsigned long
#define bool  unsigned char

#define false 0
#define true  1

#define null 0

#define __DEBUG__ 1 // 0: release mode, 1: debug mode

//----------------------------------------------------------------------------------------------------
// Macro from keyboard.h
//----------------------------------------------------------------------------------------------------

// key flags
#define KEY_FLAGS_UP          0x00 // up
#define KEY_FLAGS_DOWN        0x01 // down
#define KEY_FLAGS_EXTENDEDKEY 0x02 // extended key

// keys not in ASCII code (but, KEY_ENTER and KEY_TAB are in ASCII code)
#define KEY_NONE        0x00
#define KEY_ENTER       '\n'
#define KEY_TAB         '\t'
#define KEY_ESC         0x1B
#define KEY_BACKSPACE   0x08
#define KEY_CTRL        0x81
#define KEY_LSHIFT      0x82
#define KEY_RSHIFT      0x83
#define KEY_PRINTSCREEN 0x84
#define KEY_LALT        0x85
#define KEY_CAPSLOCK    0x86
#define KEY_F1          0x87
#define KEY_F2          0x88
#define KEY_F3          0x89
#define KEY_F4          0x8A
#define KEY_F5          0x8B
#define KEY_F6          0x8C
#define KEY_F7          0x8D
#define KEY_F8          0x8E
#define KEY_F9          0x8F
#define KEY_F10         0x90
#define KEY_NUMLOCK     0x91
#define KEY_SCROLLLOCK  0x92
#define KEY_HOME        0x93
#define KEY_UP          0x94
#define KEY_PAGEUP      0x95
#define KEY_LEFT        0x96
#define KEY_CENTER      0x97
#define KEY_RIGHT       0x98
#define KEY_END         0x99
#define KEY_DOWN        0x9A
#define KEY_PAGEDOWN    0x9B
#define KEY_INS         0x9C
#define KEY_DEL         0x9D
#define KEY_F11         0x9E
#define KEY_F12         0x9F
#define KEY_PAUSE       0xA0

//----------------------------------------------------------------------------------------------------
// Macro from console.h
//----------------------------------------------------------------------------------------------------

#define CONSOLE_WIDTH  80
#define CONSOLE_HEIGHT 25

//----------------------------------------------------------------------------------------------------
// Macro from task.h
//----------------------------------------------------------------------------------------------------

// invalid task ID
#define TASK_INVALIDID 0xFFFFFFFFFFFFFFFF

// task priority (low 8 bits of task flags)
#define TASK_PRIORITY_HIGHEST 0x00 // highest
#define TASK_PRIORITY_HIGH    0x01 // high
#define TASK_PRIORITY_MEDIUM  0x02 // medium
#define TASK_PRIORITY_LOW     0x03 // low
#define TASK_PRIORITY_LOWEST  0x04 // lowest
#define TASK_PRIORITY_END     0xFF // end task priority

// task flags
#define TASK_FLAGS_END     0x8000000000000000 // end task flag
#define TASK_FLAGS_SYSTEM  0x4000000000000000 // system task flag
#define TASK_FLAGS_PROCESS 0x2000000000000000 // processor flag
#define TASK_FLAGS_THREAD  0x1000000000000000 // thread flag
#define TASK_FLAGS_IDLE    0x0800000000000000 // idle task flag
#define TASK_FLAGS_GUI     0x0400000000000000 // GUI task flag: set in k_createWindow.
#define TASK_FLAGS_USER    0x0200000000000000 // user task flag

// affinity
#define TASK_AFFINITY_LOADBALANCING 0xFF // load balancing (no affinity)

/* macro functions */
#define GETTASKOFFSET(taskId)            ((taskId) & 0xFFFFFFFF)                                 // get task offset (low 32 bits) of task.link.id (64 bits).
#define GETTASKPRIORITY(flags)           ((flags) & 0xFF)                                        // get task priority (low 8 bits) of task.flags(64 bits).
#define SETTASKPRIORITY(flags, priority) ((flags) = ((flags) & 0xFFFFFFFFFFFFFF00) | (priority)) // set task priority (low 8 bits) of task.flags(64 bits).

//----------------------------------------------------------------------------------------------------
// Macro from file_system.h
//----------------------------------------------------------------------------------------------------

// max file name length
#define FS_MAXFILENAMELENGTH      24   // max file name length (including file extension and last null character)

// SEEK option
#define SEEK_SET 0 // start of file
#define SEEK_CUR 1 // current file pointer offset
#define SEEK_END 2 // end of file

// redefine HansFS type names as C standard I/O type names.
#define size_t dword
#define dirent DirEntry
#define d_name fileName
#define d_size fileSize

//----------------------------------------------------------------------------------------------------
// Macro from loader.h
//----------------------------------------------------------------------------------------------------

// max argument string length
#define LOADER_MAXARGSLENGTH 1023

//----------------------------------------------------------------------------------------------------
// Macro from 2d_graphics.h
//----------------------------------------------------------------------------------------------------

// HansOS uses 16 bits color.
// A color (16 bits) in video memory represents a pixel (16 bits) in screen.
typedef word Color;

// change r, g, b (24 bits) to 16 bits color.
// - r: 0 ~ 255 (8 bits) / 8 = 0 ~ 31 (5 bits)
// - g: 0 ~ 255 (8 bits) / 4 = 0 ~ 63 (6 bits)
// - b: 0 ~ 255 (8 bits) / 8 = 0 ~ 31 (5 bits)
// Being closer to 255 represents brighter color.
#define RGB(r, g, b) ((((byte)(r) >> 3) << 11) | (((byte)(g) >> 2) << 5) | ((byte)(b) >> 3))

//----------------------------------------------------------------------------------------------------
// Macro from fonts.h
//----------------------------------------------------------------------------------------------------

/* Default Font */
#define FONT_DEFAULT_WIDTH  FONT_VERAMONO_ENG_WIDTH
#define FONT_DEFAULT_HEIGHT FONT_VERAMONO_ENG_HEIGHT

/* Bitstream Vera Sans Mono (English) */
#define FONT_VERAMONO_ENG_WIDTH  8  // 8 pixels per a character
#define FONT_VERAMONO_ENG_HEIGHT 16 // 16 pixels per a character

//----------------------------------------------------------------------------------------------------
// Macro from window.h
//----------------------------------------------------------------------------------------------------

// etc macros
#define WINDOW_MAXTITLELENGTH 40   // max title length: exclude last null character
#define WINDOW_INVALIDID      0xFFFFFFFFFFFFFFFF

// window flags
#define WINDOW_FLAGS_SHOW         0x00000001 // show flag
#define WINDOW_FLAGS_DRAWFRAME    0x00000002 // draw frame flag
#define WINDOW_FLAGS_DRAWTITLEBAR 0x00000004 // draw title bar flag
#define WINDOW_FLAGS_RESIZABLE    0x00000008 // resizable flag
#define WINDOW_FLAGS_DEFAULT      (WINDOW_FLAGS_SHOW | WINDOW_FLAGS_DRAWFRAME | WINDOW_FLAGS_DRAWTITLEBAR)

// window size
#define WINDOW_TITLEBAR_HEIGHT  21 // title bar height
#define WINDOW_XBUTTON_SIZE     19 // close button, resize button size
#define WINDOW_MINWIDTH         (WINDOW_XBUTTON_SIZE * 2 + 30) // min window width
#define WINDOW_MINHEIGHT        (WINDOW_TITLEBAR_HEIGHT + 30)  // min window height

// window color
#define WINDOW_COLOR_BACKGROUND                 RGB(255, 255, 255)
#define WINDOW_COLOR_FRAME                      RGB(109, 218, 22)
#define WINDOW_COLOR_TITLEBARTEXT               RGB(255, 255, 255)
#define WINDOW_COLOR_TITLEBARBACKGROUNDACTIVE   RGB(79, 204, 11)   // bright green
#define WINDOW_COLOR_TITLEBARBACKGROUNDINACTIVE RGB(55, 135, 11)   // dark green
#define WINDOW_COLOR_TITLEBARBRIGHT1            RGB(183, 249, 171) // bright color
#define WINDOW_COLOR_TITLEBARBRIGHT2            RGB(150, 210, 140) // bright color
#define WINDOW_COLOR_TITLEBARDARK               RGB(46, 59, 30)    // dark color
#define WINDOW_COLOR_BUTTONBRIGHT               RGB(229, 229, 229) // bright color
#define WINDOW_COLOR_BUTTONDARK                 RGB(86, 86, 86)    // dark color
#define WINDOW_COLOR_SYSTEMBACKGROUND           RGB(232, 255, 232) // brighter green
#define WINDOW_COLOR_XBUTTONMARK                RGB(71, 199, 21)   // 'X' mark on close button, '<->' mark on resize button  

// background window title
#define WINDOW_BACKGROUNDWINDOWTITLE "SYS_BACKGROUND"

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
#define GETWINDOWOFFSET(windowId) ((windowId) & 0xFFFFFFFF) // get window offset (low 32 bits) of window.link.id (64 bits).

//----------------------------------------------------------------------------------------------------
// Macro from util.h ***/
//----------------------------------------------------------------------------------------------------

/* Macro Functions */
#define MIN(x, y)     (((x) < (y)) ? (x) : (y))
#define MAX(x, y)     (((x) > (y)) ? (x) : (y))
#define ABS(x)        (((x) >= 0) ? (x) : -(x))
#define SWAP(x, y, t) ((t) = (x)), ((x) = (y)), ((y) = (t))

#pragma pack(push, 1)
//----------------------------------------------------------------------------------------------------
// Struct from keyboard.h
//----------------------------------------------------------------------------------------------------

typedef struct __Key {
	byte scanCode;  // scan code
	byte asciiCode; // ASCII code
	byte flags;     // key flags
} Key;

//----------------------------------------------------------------------------------------------------
// Struct from sync.h
//----------------------------------------------------------------------------------------------------
typedef struct __Mutex {
	volatile qword taskId;    // lock-executing task ID
	volatile dword lockCount; // lock count: Mutex allows duplicated lock.
	volatile bool lockFlag;   // lock flag
	byte padding[3];          // padding bytes: align structure size with 8 bytes.
} Mutex;

//----------------------------------------------------------------------------------------------------
// Struct from file_system.h
//----------------------------------------------------------------------------------------------------

typedef struct __DirEntry {
	char fileName[FS_MAXFILENAMELENGTH]; // [byte 0~23]  : file name (including file extension and last null character)
	dword fileSize;                      // [byte 24~27] : file size (byte-level)
	dword startClusterIndex;             // [byte 28~31] : start cluster index (0x00:free directory entry)
} DirEntry; // 32 bytes-sized

typedef struct __FileHandle {
	int dirEntryOffset;        // directory entry offset (directory entry index matching file name)
	dword fileSize;            // file size (byte-level)
	dword startClusterIndex;   // start cluster index
	dword currentClusterIndex; // current cluster index (cluster index which current I/O is working)
	dword prevClusterIndex;    // previous cluster index
	dword currentOffset;       // current file pointer offset (byte-level)
} FileHandle;

typedef struct __DirHandle {
	DirEntry* dirBuffer; // root directory buffer
	int currentOffset;   // current directory pointer offset
} DirHandle;

typedef struct __FileDirHandle {
	byte type; // handle type: free handle, file handle, directory handle	
	union {
		FileHandle fileHandle; // file handle
		DirHandle dirHandle;   // directory handle
	};
} File, Dir;

//----------------------------------------------------------------------------------------------------
// Struct from 2d_graphics.h
//----------------------------------------------------------------------------------------------------

typedef struct k_Point {
	int x; // x of point
	int y; // y of point
} Point;

typedef struct k_Rect {
	int x1; // x of start point (top-left)
	int y1; // y of start point (top-left)
	int x2; // x of end point (bottom-right)
	int y2; // y of end point (bottom-right)
} Rect;

//----------------------------------------------------------------------------------------------------
// Struct from window.h
//----------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------
// Struct from jpeg.h
//----------------------------------------------------------------------------------------------------
// huffman table
typedef struct k_Huff {
	int elem; // element count
	unsigned short code[256];
	unsigned char size[256];
	unsigned char value[256];
} Huff;

// JPEG struct
typedef struct k_Jpeg {
	// SOF: start of frame
	int width;
	int height;
	
	// MCU: minimum coded unit
	int mcu_width;
	int mcu_height;
	
	int max_h, max_v;
	int compo_count;
	int compo_id[3];
	int compo_sample[3];
	int compo_h[3];
	int compo_v[3];
	int compo_qt[3];
	
	// SOS: start of scan
	int scan_count;
	int scan_id[3];
	int scan_ac[3];
	int scan_dc[3];
	int scan_h[3];  // sampling element count
	int scan_v[3];  // sampling element count
	int scan_qt[3]; // quantization table index
	
	// DRI: data restart interval
	int interval;
	
	int mcu_buf[32 * 32 * 4]; // buffer
	int *mcu_yuv[4];
	int mcu_preDC[3];
	
	// DQT: define quantization table
	int dqt[3][64];
	int n_dqt;
	
	// DHT: define huffman table
	Huff huff[2][3];
	
	// I/O: input/output
	const unsigned char* data;
	int data_index;
	int data_size;
	
	unsigned long bit_buff;
	int bit_remain;
} Jpeg;

#pragma pack(pop)

#endif // __TYPES_H__