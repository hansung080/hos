#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "types.h"
#include "sync.h"

/**
  < Keyboard Controller's Registers >
    - Control Register : port 0x64, write only, control Keyboard Controller.
                         k_outPortByte(0x64, data)

    - Status Register  : port 0x64, read only, show status of Keyboard Controller.
                         k_inPortByte(0x64) : data

    - Input Buffer     : port 0x60, write only, save command/data from processor to keyboard/mouse/output port.
                         k_outPortByte(0x60, data)

    - Output Buffer    : port 0x60, read only, save data from keyboard/mouse/output port to processor.
                         k_inPortByte(0x60) : data

      [Ref] Output port is inside Keyboard Controller.
            To read/write data from/to output port, It requires to send the command to Control Register before.
 */

#define KEY_SKIPCOUNTFORPAUSE 2

// key flags
#define KEY_FLAGS_UP          0x00 // up
#define KEY_FLAGS_DOWN        0x01 // down
#define KEY_FLAGS_EXTENDEDKEY 0x02 // extended key

#define KEY_MAPPINGTABLEMAXCOUNT 89

// keys not in ASCII code (but, ENTER and TAB are in ASCII code)
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

// key queue-related macro
#define KEY_MAXQUEUECOUNT 100

#pragma pack(push, 1)

typedef struct k_KeyMappingEntry {
	byte normalCode;   // ASCII code of normal key
	byte combinedCode; // ASCII code of combined key
} KeyMappingEntry;

typedef struct k_Key {
	byte scanCode;  // scan code
	byte asciiCode; // ASCII code
	byte flags;     // key flags
} Key;

typedef struct k_KeyboardManager {
	Spinlock spinlock;
	
	// combined key info
	bool shiftDown;
	bool capslockOn;
	bool numlockOn;
	bool scrolllockOn;

	// extended key info
	bool extendedCodeIn;
	int skipCountForPause;
} KeyboardManager;

#pragma pack(pop)

bool k_isOutputBufferFull(void);
bool k_isInputBufferFull(void);
bool k_activateKeyboard(void);
byte k_getKeyboardScanCode(void); // get scan code
bool k_changeKeyboardLed(bool capslockOn, bool numlockOn, bool scrolllockOn);
void k_enableA20gate(void);
void k_rebootSystem(void);
bool k_isAlphabetScanCode(byte downScanCode);
bool k_isNumberOrSymbolScanCode(byte downScanCode);
bool k_isNumberPadScanCode(byte downScanCode);
bool k_isCombinedKeyUsing(byte scanCode);
void k_updateCombinedKeyStatusAndLed(byte scanCode);
bool k_convertScanCodeToAsciiCode(byte scanCode, byte* asciiCode, byte* flags);
bool k_initKeyboard(void); // initialize key queue and enable keyboard.
bool k_convertScanCodeAndPutQueue(byte scanCode); // convert scan code to ASCII code and put key to key queue.
bool k_getKeyFromKeyQueue(Key* key); // get key from key queue.
bool k_waitAckAndPutOtherScanCodes(void);

#endif // __KEYBOARD_H__
