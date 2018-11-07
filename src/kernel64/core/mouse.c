#include "mouse.h"
#include "keyboard.h"
#include "../utils/queue.h"
#include "asm_util.h"
#include "../utils/util.h"

static MouseManager g_mouseManager = {0, };
static Queue g_mouseQueue;
static MouseData g_mouseBuffer[MOUSE_MAXQUEUECOUNT];

bool k_initMouse(void) {
	// initialize mouse queue.
	// This function must be called before activating mouse.
	k_initQueue(&g_mouseQueue, g_mouseBuffer, sizeof(MouseData), MOUSE_MAXQUEUECOUNT, false);

	// initialize spinlock
	k_initSpinlock(&g_mouseManager.spinlock);

	// activate mouse.
	if (k_activateMouse() == true) {
		// enable mouse interrupt.
		k_enableMouseInterrupt();
		return true;
	}

	return false;
}

bool k_activateMouse(void) {
	int i, j;
	bool interruptFlag;
	bool result;

	interruptFlag = k_setInterruptFlag(false);

	/* activate mouse of Keyboard Controller */

	// send [0xA8: Mouse Activation Command] to Control Register.
	k_outPortByte(0x64, 0xA8);

	/* activate mouse */

	// send [0xD4: Mouse Send Command] to Control Register in order to send data in Input Buffer to mouse.
	// If this command has not been sent, data in Input Buffer will be sent to keyboard.
	k_outPortByte(0x64, 0xD4);

	// wait until Input Buffer will be empty before sending the command.
	for (i = 0; i < 0xFFFF; i++) {
		if (k_isInputBufferFull() == false) {
			break;
		}			
	}

	// send [0xF4: Mouse Activation Command] to Input Buffer.
	k_outPortByte(0x60, 0xF4);

	// wait until ACK will be received.
	result = k_waitAckAndPutOtherScanCodes();

	k_setInterruptFlag(interruptFlag);

	return result;
}

void k_enableMouseInterrupt(void) {
	byte outPortData; // output port data (command byte)
	int i;

	/* read command byte */

	// send [0x20: Command Byte Read Command] to Control Register.
	k_outPortByte(0x64, 0x20);

	// wait until Output Buffer is full before reading command byte.
	for (i = 0; i < 0xFFFF; i++) {
		if (k_isOutputBufferFull() == true) {
			break;
		}
	}

	// read command byte from output port.
	outPortData = k_inPortByte(0x60);

	/* change command byte */

	// set Mouse Interrupt Enable Bit (bit 1) to 1.
	outPortData |= 0x02;

	/* write command byte */

	// send [0x60: Command Byte Write Command] to Control Register.
	k_outPortByte(0x64, 0x60);

	// wait until Input Buffer is empty before writing command byte.
	for (i = 0; i < 0xFFFF; i++) {
		if (k_isInputBufferFull() == false) {
			break;
		}
	}

	// write command byte to output port.
	k_outPortByte(0x60, outPortData);
}

bool k_accumulateMouseDataAndPutQueue(byte mouseData) {
	bool result;

	switch (g_mouseManager.byteCount) {
	case 0:
		g_mouseManager.currentData.flagsAndButtonStatus = mouseData;
		g_mouseManager.byteCount++;
		break;

	case 1:
		g_mouseManager.currentData.xMovement = mouseData;
		g_mouseManager.byteCount++;
		break;

	case 2:
		g_mouseManager.currentData.yMovement = mouseData;
		g_mouseManager.byteCount++;
		break;

	default:
		g_mouseManager.byteCount = 0;
		break;
	}

	if (g_mouseManager.byteCount >= 3) {
		k_lockSpin(&g_mouseManager.spinlock);
		k_putQueue(&g_mouseQueue, &g_mouseManager.currentData);
		k_unlockSpin(&g_mouseManager.spinlock);
		g_mouseManager.byteCount = 0;
	}
}

bool k_getMouseDataFromMouseQueue(byte* buttonStatus, int* relativeX, int* relativeY) {
	MouseData mouseData;
	bool result;

	if (g_mouseQueue.blocking == false && k_isQueueEmpty(&g_mouseQueue) == true) {
		return false;
	}

	k_lockSpin(&g_mouseManager.spinlock);
	result = k_getQueue(&g_mouseQueue, &mouseData, &g_mouseManager.spinlock);
	k_unlockSpin(&g_mouseManager.spinlock);
	
	if (result == false) {
		return false;
	}

	// set button status (low 3 bits).
	*buttonStatus = mouseData.flagsAndButtonStatus & 0x07;

	// set x movement.
	*relativeX = mouseData.xMovement & 0xFF;

	// If x sign (bit 4) == minus.
	if (mouseData.flagsAndButtonStatus & 0x10) {
		// keep x movement (low 8 bits) and extend sign bits by setting high 24 bits to 1.
		*relativeX |= 0xFFFFFF00;
	}

	// set y movement.
	*relativeY = mouseData.yMovement & 0xFF;

	// If y sign (bit 5) == minus.
	if (mouseData.flagsAndButtonStatus & 0x20) {
		// keep y movement (low 8 bits) and extend sign bits by setting high 24 bits to 1.
		*relativeY |= 0xFFFFFF00;
	}

	// reverse y sign, because y increasement direction of screen and that of mouse are opposite.
	// In screen, y increases as going down. But, In mouse, y increases as going up.
	*relativeY = -(*relativeY);

	return true;
}

bool k_isMouseDataInOutputBuffer(void) {
	// read Status Register to check that data in Output Buffer is mouse data or keyboard data, before reading Output Buffer.
	// If AUXB Bit (bit 5) of Status Register == 1, data in Output Buffer is mouse data.
	if (k_inPortByte(0x64) & 0x20) {
		return true;
	}

	return false;
}