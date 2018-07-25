#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "types.h"

/**
  < Registers and Port I/O Functions of Keyboard Controller >
    1. Control Register: k_outPortByte(0x64, bData)
    2. Status Register: k_inPortByte(0x64) : bData
    3. Input Buffer: k_outPortByte(0x60, bData)
    4. Output BUffer: k_inPortByte(0x60) : bData
 */

#define KEY_SKIPCOUNTFORPAUSE 2

// key status flag
#define KEY_FLAGS_UP          0x00
#define KEY_FLAGS_DOWN        0x01
#define KEY_FLAGS_EXTENDEDKEY 0x02

#define KEY_MAPPINGTABLEMAXCOUNT 89

// keys not in ASCII code (but, ENTER, TAB is in ASCII code)
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

// macros related with key queue
#define KEY_MAXQUEUECOUNT 100

#pragma pack(push, 1)

typedef struct k_KeyMappingEntry {
	byte normalCode;   // ASCII code of normal key
	byte combinedCode; // ASCII code of combined key
} KeyMappingEntry;

typedef struct k_KeyboardManager {
	// combined key info
	bool shiftDown;
	bool capslockOn;
	bool numlockOn;
	bool scrolllockOn;

	// extended key info
	bool extendedCodeIn;
	int skipCountForPause;
} KeyboardManager;

typedef struct k_Key {
	byte scanCode;  // scan code
	byte asciiCode; // ASCII code
	byte flags;     // key status flag (UP, DOWN, EXTENDEDKEY)
} Key;

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
bool k_isUseCombinedCode(byte scanCode);
void k_updateCombinationKeyStatusAndLed(byte scanCode);
bool k_convertScanCodeToAsciiCode(byte scanCode, byte* asciiCode, byte* flags);
bool k_initKeyboard(void); // initialize key queue and enable keyboard.
bool k_convertScanCodeAndPutQueue(byte scanCode); // convert scan code to ASCII code and put key to key queue.
bool k_getKeyFromKeyQueue(Key* key); // get key from key queue.
bool k_waitAckAndPutOtherScanCode(void);

#endif // __KEYBOARD_H__
