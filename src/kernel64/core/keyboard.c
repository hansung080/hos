#include "types.h"
#include "asm_util.h"
#include "keyboard.h"
#include "../utils/queue.h"
#include "../utils/util.h"
#include "sync.h"
#include "mouse.h"

bool k_isOutputBufferFull(void) {
	
	// check OUTB(bit 0) of Status Register.
	if (k_inPortByte(0x64) & 0x01) {
		return true;
	}
	
	return false;
}

bool k_isInputBufferFull(void) {
	
	// check INPB(bit 1) of Status Register.
	if (k_inPortByte(0x64) & 0x02) {
		return true;
	}
	
	return false;
}

bool k_activateKeyboard(void) {
	int i, j;
	bool result;
	bool interruptFlag;
	
	interruptFlag = k_setInterruptFlag(false);
	
	/* activate keyboard of Keyboard Controller */

	// send [0xAE: Keyboard Activation Command] to Control Register.
	k_outPortByte(0x64, 0xAE);
	
	/* activate keyboard */

	// wait until Input Buffer will be empty before sending the command.
	for (i = 0; i < 0xFFFF; i++) {
		if (k_isInputBufferFull() == false) {
			break;
		}
	}
	
	// send [0xF4: Keyboard Activation Command] to Input Buffer.
	k_outPortByte(0x60, 0xF4);
	
	// wait until ACK will be received.
	result = k_waitAckAndPutOtherScanCodes();
	
	k_setInterruptFlag(interruptFlag);
	
	return result;
}

byte k_getKeyboardScanCode(void) {
	while (k_isOutputBufferFull() == false) {
		;
	}
	
	// return data reading from Output Buffer.
	// The data might be keyboard data (scan code) or mouse data.
	// Thus, you need to check what kind of data it is using k_isMouseDataInOutputBuffer before calling this function.
	return k_inPortByte(0x60);
}

bool k_changeKeyboardLed(bool capslockOn, bool numlockOn, bool scrolllockOn) {
	int i, j;
	bool result;
	bool interruptFlag;
	
	interruptFlag = k_setInterruptFlag(false);
	
	for (i = 0; i < 0xFFFF; i++) {
		if (k_isInputBufferFull() == false) {
			break;
		}
	}
	
	// send Keyboard LED Status Change Command: send [0xED: Keyboard LED Status Change Command] to Input Buffer.
	k_outPortByte(0x60, 0xED);
	
	for (i = 0; i < 0xFFFF; i++) {
		if (k_isInputBufferFull() == false) {
			break;
		}
	}
	
	// wait until ACK will be received.
	result = k_waitAckAndPutOtherScanCodes();
	
	if (result == false) {
		k_setInterruptFlag(interruptFlag);
		return false;
	}
	
	// send Keyboard LED Status Change Data: send [caps lock (bit 2) | num lock (bit 1) | scroll lock (bit 0)] to Input Buffer.
	k_outPortByte(0x60, (capslockOn << 2) | (numlockOn << 1) | (scrolllockOn));
	
	for (i = 0; i < 0xFFFF; i++) {
		if (k_isInputBufferFull() == false) {
			break;
		}
	}
	
	// wait until ACK will be received.
	result = k_waitAckAndPutOtherScanCodes();
	
	k_setInterruptFlag(interruptFlag);
	
	return result;
}

void k_enableA20gate(void) {
	byte outPortData; // output port data
	int i;
	
	/* read output port data */

	// send [0xD0: Output Port Read Command] to Control Register.
	k_outPortByte(0x64, 0xD0);
	
	// wait until Output Buffer is full before reading data.
	for (i = 0; i < 0xFFFF; i++) {
		if (k_isOutputBufferFull() == true) {
			break;
		}
	}
	
	// read data from output port.
	outPortData = k_inPortByte(0x60);
	
	/* change output port data */
	
	// set A20 Gate Enable Bit(bit 1) to 1.
	outPortData |= 0x02;
	
	/* write output port data */

	// wait until Input Buffer is empty before sending the command.
	for (i = 0; i < 0xFFFF; i++) {
		if (k_isInputBufferFull() == false) {
			break;
		}
	}
	
	// send [0xD1: Output Port Write Command] to Control Register.
	k_outPortByte(0x64, 0xD1);
	
	// write data to output port.
	k_outPortByte(0x60, outPortData);
}

void k_rebootSystem(void) {
	int i;
	
	for (i = 0; i < 0xFFFF; i++) {
		if (k_isInputBufferFull() == false) {
			break;
		}
	}
	
	// send [0xD1: Output Port Write Command] to Control Register.
	k_outPortByte(0x64, 0xD1);
	
	// set Processor Reset Bit (bit 0) to 0.
	k_outPortByte(0x60, 0x00);
	
	while (true) {
		;
	}
}

static KeyboardManager g_keyboardManager = {0, };
static Queue g_keyQueue;
static Key g_keyBuffer[KEY_MAXQUEUECOUNT];

// table to convert from scan code to ASCII code
static KeyMappingEntry g_keyMappingTable[KEY_MAPPINGTABLEMAXCOUNT] = {
		//----------------------------------------------------------------
		//    Scan Code    |       ASCII Code
		//----------------------------------------------------------------
		//    Down  Up     | Normal          Combined
		//----------------------------------------------------------------
		/*  0:0x00, 0x80 */ {KEY_NONE,       KEY_NONE},
		/*  1:0x01, 0x81 */ {KEY_ESC,        KEY_ESC},
		/*  2:0x02, 0x82 */ {'1',            '!'},
		/*  3:0x03, 0x83 */ {'2',            '@'},
		/*  4:0x04, 0x84 */ {'3',            '#'},
		/*  5:0x05, 0x85 */ {'4',            '$'},
		/*  6:0x06, 0x86 */ {'5',            '%'},
		/*  7:0x07, 0x87 */ {'6',            '^'},
		/*  8:0x08, 0x88 */ {'7',            '&'},
		/*  9:0x09, 0x89 */ {'8',            '*'},
		/* 10:0x0A, 0x8A */ {'9',            '('},
		/* 11:0x0B, 0x8B */ {'0',            ')'},
		/* 12:0x0C, 0x8C */ {'-',            '_'},
		/* 13:0x0D, 0x8D */ {'=',            '+'},
		/* 14:0x0E, 0x8E */ {KEY_BACKSPACE,  KEY_BACKSPACE},
		/* 15:0x0F, 0x8F */ {KEY_TAB,        KEY_TAB},
		/* 16:0x10, 0x90 */ {'q',            'Q'},
		/* 17:0x11, 0x91 */ {'w',            'W'},
		/* 18:0x12, 0x92 */ {'e',            'E'},
		/* 19:0x13, 0x93 */ {'r',            'R'},
		/* 20:0x14, 0x94 */ {'t',            'T'},
		/* 21:0x15, 0x95 */ {'y',            'Y'},
		/* 22:0x16, 0x96 */ {'u',            'U'},
		/* 23:0x17, 0x97 */ {'i',            'I'},
		/* 24:0x18, 0x98 */ {'o',            'O'},
		/* 25:0x19, 0x99 */ {'p',            'P'},
		/* 26:0x1A, 0x9A */ {'[',            '{'},
		/* 27:0x1B, 0x9B */ {']',            '}'},
		/* 28:0x1C, 0x9C */ {KEY_ENTER,      KEY_ENTER},
		/* 29:0x1D, 0x9D */ {KEY_CTRL,       KEY_CTRL},
		/* 30:0x1E, 0x9E */ {'a',            'A'},
		/* 31:0x1F, 0x9F */ {'s',            'S'},
		/* 32:0x20, 0xA0 */ {'d',            'D'},
		/* 33:0x21, 0xA1 */ {'f',            'F'},
		/* 34:0x22, 0xA2 */ {'g',            'G'},
		/* 35:0x23, 0xA3 */ {'h',            'H'},
		/* 36:0x24, 0xA4 */ {'j',            'J'},
		/* 37:0x25, 0xA5 */ {'k',            'K'},
		/* 38:0x26, 0xA6 */ {'l',            'L'},
		/* 39:0x27, 0xA7 */ {';',            ':'},
		/* 40:0x28, 0xA8 */ {'\'',           '\"'},
		/* 41:0x29, 0xA9 */ {'`',            '~'},
		/* 42:0x2A, 0xAA */ {KEY_LSHIFT,     KEY_LSHIFT},
		/* 43:0x2B, 0xAB */ {'\\',           '|'},
		/* 44:0x2C, 0xAC */ {'z',            'Z'},
		/* 45:0x2D, 0xAD */ {'x',            'X'},
		/* 46:0x2E, 0xAE */ {'c',            'C'},
		/* 47:0x2F, 0xAF */ {'v',            'V'},
		/* 48:0x30, 0xB0 */ {'b',            'B'},
		/* 49:0x31, 0xB1 */ {'n',            'N'},
		/* 50:0x32, 0xB2 */ {'m',            'M'},
		/* 51:0x33, 0xB3 */ {',',            '<'},
		/* 52:0x34, 0xB4 */ {'.',            '>'},
		/* 53:0x35, 0xB5 */ {'/',            '?'},
		/* 54:0x36, 0xB6 */ {KEY_RSHIFT,     KEY_RSHIFT},
		/* 55:0x37, 0xB7 */ {'*',            '*'},
		/* 56:0x38, 0xB8 */ {KEY_LALT,       KEY_LALT},
		/* 57:0x39, 0xB9 */ {' ',            ' '},
		/* 58:0x3A, 0xBA */ {KEY_CAPSLOCK,   KEY_CAPSLOCK},
		/* 59:0x3B, 0xBB */ {KEY_F1,         KEY_F1},
		/* 60:0x3C, 0xBC */ {KEY_F2,         KEY_F2},
		/* 61:0x3D, 0xBD */ {KEY_F3,         KEY_F3},
		/* 62:0x3E, 0xBE */ {KEY_F4,         KEY_F4},
		/* 63:0x3F, 0xBF */ {KEY_F5,         KEY_F5},
		/* 64:0x40, 0xC0 */ {KEY_F6,         KEY_F6},
		/* 65:0x41, 0xC1 */ {KEY_F7,         KEY_F7},
		/* 66:0x42, 0xC2 */ {KEY_F8,         KEY_F8},
		/* 67:0x43, 0xC3 */ {KEY_F9,         KEY_F9},
		/* 68:0x44, 0xC4 */ {KEY_F10,        KEY_F10},
		/* 69:0x45, 0xC5 */ {KEY_NUMLOCK,    KEY_NUMLOCK},
		/* 70:0x46, 0xC6 */ {KEY_SCROLLLOCK, KEY_SCROLLLOCK},
		/* 71:0x47, 0xC7 */ {KEY_HOME,       '7'},
		/* 72:0x48, 0xC8 */ {KEY_UP,         '8'},
		/* 73:0x49, 0xC9 */ {KEY_PAGEUP,     '9'},
		/* 74:0x4A, 0xCA */ {'-',            '-'},
		/* 75:0x4B, 0xCB */ {KEY_LEFT,       '4'},
		/* 76:0x4C, 0xCC */ {KEY_CENTER,     '5'},
		/* 77:0x4D, 0xCD */ {KEY_RIGHT,      '6'},
		/* 78:0x4E, 0xCE */ {'+',            '+'},
		/* 79:0x4F, 0xCF */ {KEY_END,        '1'},
		/* 80:0x50, 0xD0 */ {KEY_DOWN,       '2'},
		/* 81:0x51, 0xD1 */ {KEY_PAGEDOWN,   '3'},
		/* 82:0x52, 0xD2 */ {KEY_INS,        '0'},
		/* 83:0x53, 0xD3 */ {KEY_DEL,        '.'},
		/* 84:0x54, 0xD4 */ {KEY_NONE,       KEY_NONE},
		/* 85:0x55, 0xD5 */ {KEY_NONE,       KEY_NONE},
		/* 86:0x56, 0xD6 */ {KEY_NONE,       KEY_NONE},
		/* 87:0x57, 0xD7 */ {KEY_F11,        KEY_F11},
		/* 88:0x58, 0xD8 */ {KEY_F12,        KEY_F12}
};

bool k_isAlphabetScanCode(byte downScanCode) {
	// If ASCII code converted from scan code == [a~z], it's alphabets.
	if (('a' <= g_keyMappingTable[downScanCode].normalCode) && (g_keyMappingTable[downScanCode].normalCode <= 'z')) {
		return true;
	}
	
	return false;
}

bool k_isNumberOrSymbolScanCode(byte downScanCode) {
	// If it's not alphabet in excluding number pad and extended keys (scan code 2~53), it must be numbers or symbols.
	if ((2 <= downScanCode) && (downScanCode <= 53) && (k_isAlphabetScanCode(downScanCode) == false)) {
		return true;
	}
	
	return false;
}

bool k_isNumberPadScanCode(byte downScanCode) {
	// Scan code 71~83 is number pad.
	if ((71 <= downScanCode) && (downScanCode <= 83)) {
		return true;
	}
	
	return false;
}

bool k_isCombinedKeyUsing(byte scanCode) {
	byte downScanCode;
	bool combinedKey = false;
	
	downScanCode = scanCode & 0x7F;
	
	// Alphabet keys get affected by shift or caps lock key.
	if (k_isAlphabetScanCode(downScanCode) == true) {
		if (g_keyboardManager.shiftDown ^ g_keyboardManager.capslockOn) {
			combinedKey = true;
		
		} else {
			combinedKey = false;
		}
	
	// Number or Symbol keys get affected by shift key.
	} else if (k_isNumberOrSymbolScanCode(downScanCode) == true) {
		if(g_keyboardManager.shiftDown == true){
			combinedKey = true;
			
		} else {
			combinedKey = false;
		}
		
	// Number pad keys get affected by num lock key.
	// and process only when extended key codes are not received, because extended key codes and number pad key codes are duplicated except 0xE0.
	} else if ((k_isNumberPadScanCode(downScanCode) == true) && (g_keyboardManager.extendedCodeIn == false)) {
		if(g_keyboardManager.numlockOn == true){
			combinedKey = true;
			
		} else {
			combinedKey = false;
		}
	}
	
	return combinedKey;
}

void k_updateCombinedKeyStatusAndLed(byte scanCode) {
	bool down = false;
	byte downScanCode;
	bool ledStatusChanged = false;
	
	// If the highest bit (bit 7) of scan code == 1, it's up code.
	// If the highest bit (bit 7) of scan code == 0, it's down code.
	if (scanCode & 0x80) {
		down = false;
		downScanCode = scanCode & 0x7F;
		
	} else {
		down = true;
		downScanCode = scanCode;
	}
	
	// If [42: left shift] or [54: right shift].
	if ((downScanCode == 42) || (downScanCode == 54)) {
		g_keyboardManager.shiftDown = down;
		
	// If [58: caps lock] and down code.
	} else if ((downScanCode == 58) && (down == true)) {
		g_keyboardManager.capslockOn ^= true;
		ledStatusChanged = true;
		
	// If [69: num lock] and down code.
	} else if ((downScanCode == 69) && (down == true)) {
		g_keyboardManager.numlockOn ^= true;
		ledStatusChanged = true;
		
	// If [70: scroll lock] and down code.
	} else if ((downScanCode == 70) && (down == true)) {
		g_keyboardManager.scrolllockOn ^= true;
		ledStatusChanged = true;
	}
	
	// change keyboard LED status.
	if (ledStatusChanged == true) {
		k_changeKeyboardLed(g_keyboardManager.capslockOn, g_keyboardManager.numlockOn, g_keyboardManager.scrolllockOn);
	}
}

bool k_convertScanCodeToAsciiCode(byte scanCode, byte* asciiCode, byte* flags) {
	bool combinedKey = false;
	
	if (g_keyboardManager.skipCountForPause > 0) {
		g_keyboardManager.skipCountForPause--;
		return false;
	}
	
	// If [0xE1: pause key].
	if (scanCode == 0xE1) {
		*asciiCode = KEY_PAUSE;
		*flags = KEY_FLAGS_DOWN;
		g_keyboardManager.skipCountForPause = KEY_SKIPCOUNTFORPAUSE;
		return true;
		
	// If [0xE0: extended key].
	} else if (scanCode == 0xE0) {
		g_keyboardManager.extendedCodeIn = true;
		return false;
	}
	
	combinedKey = k_isCombinedKeyUsing(scanCode);
	
	if (combinedKey == true) {
		*asciiCode = g_keyMappingTable[scanCode & 0x7F].combinedCode;
		
	} else {
		*asciiCode = g_keyMappingTable[scanCode & 0x7F].normalCode;
	}
	
	if (g_keyboardManager.extendedCodeIn == true) {
		*flags = KEY_FLAGS_EXTENDEDKEY;
		g_keyboardManager.extendedCodeIn = false;
		
	} else {
		*flags = 0;
	}
	
	if ((scanCode & 0x80) == 0) {
		*flags |= KEY_FLAGS_DOWN;
	}
	
	k_updateCombinedKeyStatusAndLed(scanCode);
	
	return true;
}

bool k_initKeyboard(void) {
	// initialize key queue.
	k_initQueue(&g_keyQueue, g_keyBuffer, sizeof(Key), KEY_MAXQUEUECOUNT);
	
	// initialize spinlock.
	k_initSpinlock(&(g_keyboardManager.spinlock));
	
	// activate keyboard.
	return k_activateKeyboard();
}

bool k_convertScanCodeAndPutQueue(byte scanCode) {
	Key key;
	bool result = false;
	
	key.scanCode = scanCode;
	
	// convert scan code to ASCII code.
	if (k_convertScanCodeToAsciiCode(scanCode, &(key.asciiCode), &(key.flags)) == true) {
		
		k_lockSpin(&(g_keyboardManager.spinlock));
		
		// put data to key queue.
		result = k_putQueue(&g_keyQueue, &key);
		
		k_unlockSpin(&(g_keyboardManager.spinlock));
	}
	
	return result;
}

bool k_getKeyFromKeyQueue(Key* key) {
	bool result = false;
	
	if (k_isQueueEmpty(&g_keyQueue) == true) {
		return false;
	}

	k_lockSpin(&(g_keyboardManager.spinlock));
	
	// get data from key queue.
	result = k_getQueue(&g_keyQueue, key);
	
	k_unlockSpin(&(g_keyboardManager.spinlock));
	
	return result;
}

bool k_waitAckAndPutOtherScanCodes(void) {
	int i, j;
	byte data;
	bool result = false;
	bool mouseData;
	
	// It is possible that data already exists in Output Buffer before receiving ACK from keyboard/mouse.
	// Thus, check data as much as 100 times.
	for (i = 0; i < 100; i++) {
		for (j = 0; j < 0xFFFF; j++) {
			if (k_isOutputBufferFull() == true) {
				break;
			}
		}
		
		if (k_isMouseDataInOutputBuffer() == true) {
			mouseData = true;

		} else {
			mouseData = false;
		}

		// check if data reading from Output Buffer == [0xFA: ACK].
		data = k_inPortByte(0x60);
		if (data == 0xFA) {
			result = true;
			break;
			
		} else {
			if (mouseData == false) {
				k_convertScanCodeAndPutQueue(data);

			} else {
				k_accumulateMouseDataAndPutQueue(data);
			}
		}
	}
	
	return result;
}