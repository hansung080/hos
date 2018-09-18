#include "serial_Port.h"
#include "util.h"
#include "asm_util.h"

static SerialPortManager g_serialPortManager;

void k_initSerialPort(void) {
	word portBaseAddr;
	
	// initialize mutex.
	k_initMutex(&(g_serialPortManager.mutex));
	
	// select COM1 serial port.
	portBaseAddr = SERIAL_PORT_COM1;
	
	// send [0:all interrupts disable] to Interrupt Enable Register.
	k_outPortByte(portBaseAddr + SERIAL_PORT_INDEX_INTERRUPTENABLE, 0);
	
	// set communication speed [115200 Bd] using LSB/MSB Divisor Latch Register.
	k_outPortByte(portBaseAddr + SERIAL_PORT_INDEX_LINECONTROL, SERIAL_LINECONTROL_DLAB);
	k_outPortByte(portBaseAddr + SERIAL_PORT_INDEX_DIVISORLATCHLSB, SERIAL_DIVISORLATCH_115200);
	k_outPortByte(portBaseAddr + SERIAL_PORT_INDEX_DIVISORLATCHMSB, SERIAL_DIVISORLATCH_115200 >> 8);
	
	// set communication method [data bit = 8 bits, stop bit = 1 bit, not use parity bit, DLAB = 0] using Line Control Register.
	k_outPortByte(portBaseAddr + SERIAL_PORT_INDEX_LINECONTROL, SERIAL_LINECONTROL_8BIT | SERIAL_LINECONTROL_1BITSTOP | SERIAL_LINECONTROL_NOPARITY);
	
	// set FIFO-related info [enable FIFO, interrupt-occurring timing for receiving data=14bytes] using FIFO Control Register.
	// but, interrupt-occurring timing for receiving data is ignored, because currently all interrupts are disabled.
	k_outPortByte(portBaseAddr + SERIAL_PORT_INDEX_FIFOCONTROL, SERIAL_FIFOCONTROL_FIFOENABLE | SERIAL_FIFOCONTROL_14BYTEFIFO);
}

static bool k_isSerialTransmitBufferEmpty(void) {
	byte data;
	
	// [bit 5:TBE=1] of Line Control Register mean that Transmit Buffer Register is empty.
	data = k_inPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_LINESTATUS);
	if ((data & SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY) == SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY) {
		return true;
	}
	
	return false;
}

void k_sendSerialData(byte* buffer, int size) {
	int iSentByte;
	int iTempSize;
	int i;
	
	k_lock(&(g_serialPortManager.mutex));
	
	// loop until data will be sent as many as the requested byte count.
	iSentByte = 0;
	while (iSentByte < size) {
		// If Transmit Buffer Register has data, wait until it will be empty.
		while (k_isSerialTransmitBufferEmpty() == false) {
			k_sleep(0);
		}
		
		// sending byte count = MIN(remaining byte count, FIFO max size)
		iTempSize = MIN(size - iSentByte, SERIAL_FIFOMAXSIZE);
		for (i = 0; i < iTempSize; i++) {
			// send data to Transmit Buffer Register by 1 byte.
			k_outPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_TRANSMITBUFFER, buffer[iSentByte + i]);
		}
		
		iSentByte += iTempSize;
	}
	
	k_unlock(&(g_serialPortManager.mutex));
}

static bool k_isSerialReceiveBufferFull(void) {
	byte data;
	
	// [bit 0:RxRD=1] of Line Status Register mean that Receive Buffer Register has received data.
	data = k_inPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_LINESTATUS);
	if ((data & SERIAL_LINESTATUS_RECEIVEDDATAREADY) == SERIAL_LINESTATUS_RECEIVEDDATAREADY) {
		return true;
	}
	
	return false;
}

int k_recvSerialData(byte* buffer, int size) {
	int i;
	
	k_lock(&(g_serialPortManager.mutex));
	
	// loop until data will be received as many as the requested byte count.
	for (i = 0; i < size; i++) {
		// If Receive Buffer Register has no data, return.
		if (k_isSerialReceiveBufferFull() == false) {
			break;
		}
		
		// Receive Buffer Register receives data by 1 byte.
		buffer[i] = k_inPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_RECEIVEBUFFER);
	}
	
	k_unlock(&(g_serialPortManager.mutex));
	
	// return real received byte count.
	return i;
}

void k_clearSerialFifo(void) {
	k_lock(&(g_serialPortManager.mutex));
	
	// set FIFO-related info [enable FIFO, interrupt-occurring timing for receiving data=14bytes, flush Receive FIFO, flush Transmit FIFO]
	k_outPortByte(SERIAL_PORT_COM1 + SERIAL_PORT_INDEX_FIFOCONTROL, SERIAL_FIFOCONTROL_FIFOENABLE | SERIAL_FIFOCONTROL_14BYTEFIFO | SERIAL_FIFOCONTROL_CLEARRECEIVEFIFO | SERIAL_FIFOCONTROL_CLEARTRANSMITFIFO);
	
	k_unlock(&(g_serialPortManager.mutex));
}
