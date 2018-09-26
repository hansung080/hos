#ifndef __CORE_CONSOLE_H__
#define __CORE_CONSOLE_H__

#include "types.h"
#include "sync.h"
#include "queue.h"
#include "keyboard.h"

/**
  < Console Output Process >
                            text mode
                            video memory (0xB8000)     monitor
                            --------------- video      -----
      shell task            |h#e#l#l#o#...| controller |   |
  --------------------- --> |.............| ---------> |   |
  | - k_printf()      |     ---------------            -----
  | - k_printStr()    |   
  | - k_printStrXy()  |     graphic mode               GUI shell            graphic mode        
  | - k_clearScreen() |     console screen buffer      window buffer        video memory     monitor
  |                   |     --------------- GUI shell  ----- window manager ----- video      -----
  ---------------------     |h#e#l#l#o#...| task       |   | task           |   | controller |   |
                        --> |.............| ---------> |   | -------------> |   | ---------> |   |
                            ---------------            -----                -----            -----
                            - h: char (1 byte)
                            - #: attr (1 byte)
                            - h#: Char (2 bytes)
                            - screen: 80 * 25 Chars
  
  < Console Input Process >
                      kernel                                                                shell task
  keyboard            key queue   text mode                                               ---------------
  ----- keyboard      -----  -----------------------------------------------------------> | - k_getch() | 
  |   | device driver |   |                                                   console     |             |
  |   | ------------> |   |  graphic mode             key event               key queue   |             |
  -----               -----  --------> window manager --------> GUI shell --> -----  ---> |             |
                                       task                     task          |   |       ---------------
                                                                              |   |
                                                                              -----
*/

/**
 < About Color >
 - 3 Primary Colors of Light (RGB): red, green, blue. It's for screen.
 - 4 Primary Colors of Dye (CMYK): cyan, magenta, yellow, key. It's for print.
   cyan: bright blue
   magenta: red mixed with purple
   key: black
*/

// console attribute
#define CONSOLE_BACKGROUND_BLACK         0x00
#define CONSOLE_BACKGROUND_BLUE          0x10
#define CONSOLE_BACKGROUND_GREEN         0x20
#define CONSOLE_BACKGROUND_CYAN          0x30
#define CONSOLE_BACKGROUND_RED           0x40
#define CONSOLE_BACKGROUND_MAGENTA       0x50
#define CONSOLE_BACKGROUND_YELLOW        0x60
#define CONSOLE_BACKGROUND_WHITE         0x70
#define CONSOLE_BACKGROUND_BLINK         0x80 // Blink Bit of Attribute Mode Control Register of Video Controller: [1:blink], [0:highlight background color].
#define CONSOLE_FOREGROUND_DARKBLACK     0x00
#define CONSOLE_FOREGROUND_DARKBLUE      0x01
#define CONSOLE_FOREGROUND_DARKGREEN     0x02
#define CONSOLE_FOREGROUND_DARKCYAN      0x03
#define CONSOLE_FOREGROUND_DARKRED       0x04
#define CONSOLE_FOREGROUND_DARKMAGENTA   0x05
#define CONSOLE_FOREGROUND_DARKYELLOW    0x06
#define CONSOLE_FOREGROUND_DARKWHITE     0x07
#define CONSOLE_FOREGROUND_BRIGHTBLACK   0x08
#define CONSOLE_FOREGROUND_BRIGHTBLUE    0x09
#define CONSOLE_FOREGROUND_BRIGHTGREEN   0x0A
#define CONSOLE_FOREGROUND_BRIGHTCYAN    0x0B
#define CONSOLE_FOREGROUND_BRIGHTRED     0x0C
#define CONSOLE_FOREGROUND_BRIGHTMAGENTA 0x0D
#define CONSOLE_FOREGROUND_BRIGHTYELLOW  0x0E
#define CONSOLE_FOREGROUND_BRIGHTWHITE   0x0F

// console text attribute
#define CONSOLE_TEXT_ATTRIBUTE (CONSOLE_BACKGROUND_BLACK | CONSOLE_FOREGROUND_BRIGHTGREEN)

// etc macros
#define CONSOLE_WIDTH              80
#define CONSOLE_HEIGHT             25
#define CONSOLE_VIDEOMEMORYADDRESS 0xB8000 // video memory address in text mode

// console key queue-related macro
#define CONSOLE_KEYQUEUE_MAXCOUNT 100

// console cursor-related macros
#define CONSOLE_CURSOR_CHARACTER '_'
#define CONSOLE_CURSOR_ATTRIBUTE 0x00 // black

// macros related with Video Controller
#define VGA_PORT_INDEX        0x3D4 // CRTC Control Address Register
#define VGA_PORT_DATA         0x3D5 // CRTC Control Data Register
#define VGA_INDEX_UPPERCURSOR 0x0E  // High Cursor Position Register Select Command
#define VGA_INDEX_LOWERCURSOR 0x0F  // Low Cursor Position Register Select Command

#pragma pack(push, 1)

typedef struct k_ConsoleManager {
	int currentPrintOffset; // current print offset
	Char* screenBuffer;     // console screen buffer
	Mutex mutex;            // mutex
	Queue keyQueue;         // console key queue
	volatile bool exit;     // shell exit flag
} ConsoleManager;

#pragma pack(pop)

void k_initConsole(int x, int y);
void k_setCursor(int x, int y);
void k_getCursor(int* x, int* y);
void k_printf(const char* format, ...); // print format at current cursor position.
int k_printStr(const char* str); // print string at current cursor position.
void k_printStrXy(int x, int y, const char* str); // print string at (x, y).
void k_clearScreen(void);
byte k_getch(void);
ConsoleManager* k_getConsoleManager(void);
bool k_putKeyToConsoleKeyQueue(const Key* key);
bool k_getKeyFromConsoleKeyQueue(Key* key);
void k_setShellExitFlag(bool exit);

#endif // __CORE_CONSOLE_H__