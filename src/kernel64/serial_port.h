#ifndef __SERIAL_PORT_H__
#define __SERIAL_PORT_H__

#include "types.h"
#include "sync.h"

// I/O port base address of Serial Port Controller
#define SERIAL_PORT_COM1 0x3F8 // COM1 serial port (IRQ4)
#define SERIAL_PORT_COM2 0x2F8 // COM2 serial port (IRQ3)
#define SERIAL_PORT_COM3 0x3E8 // COM3 serial port (IRQ4)
#define SERIAL_PORT_COM4 0x2E8 // COM4 serial port (IRQ3)

// I/O port offset of register (all 1 byte)
#define SERIAL_PORT_INDEX_RECEIVEBUFFER           0x00 // Receive Buffer Register (read)
#define SERIAL_PORT_INDEX_TRANSMITBUFFER          0x00 // Transmit Buffer Register (write)
#define SERIAL_PORT_INDEX_INTERRUPTENABLE         0x01 // Interrupt Enable Register (read/write)
#define SERIAL_PORT_INDEX_DIVISORLATCHLSB         0x00 // LSB Divisor Latch Register (read/write)
#define SERIAL_PORT_INDEX_DIVISORLATCHMSB         0x01 // MSB Divisor Latch Register (read/write)
#define SERIAL_PORT_INDEX_INTERRUPTIDENTIFICATION 0x02 // Interrupt Identification Register (read)
#define SERIAL_PORT_INDEX_FIFOCONTROL             0x02 // FIFO Control Register (write)
#define SERIAL_PORT_INDEX_LINECONTROL             0x03 // Line Control Register (read/write)
#define SERIAL_PORT_INDEX_MODEMCONTROL            0x04 // Modem Control Register (read/write)
#define SERIAL_PORT_INDEX_LINESTATUS              0x05 // Line Status Register (read/write)
#define SERIAL_PORT_INDEX_MODEMSTATUS             0x06 // Modem Status Register (read/write)
#define SERIAL_PORT_INDEX_SCRATCHPAD              0x07 // Scratch Pad Register (read/write)

// fields of Interrupt Enable Register (1 byte)
#define SERIAL_INTERRUPTENABLE_RECEIVEBUFFERFULL   0x01 // [bit 0:RxRD] Receive Data Ready, set whether causing interrupt, when Receive Buffer Register is full.
#define SERIAL_INTERRUPTENABLE_TRANSMITBUFFEREMPTY 0x02 // [bit 1:TBE] Transmit Buffer Empty, set whether causing interrupt, when Transmit Buffer Register is empty.
#define SERIAL_INTERRUPTENABLE_LINESTATUS          0x04 // [bit 2:EBRK] Error & Break, set whether causing interrupt, when error occurs in sending/receiving data.
#define SERIAL_INTERRUPTENABLE_DELTASTATUS         0x08 // [bit 3:SINP] Serial Input, set whether causing interrupt, when modem status changes.
                                                        // [bit 4~7:None] not used.

// fields of FIFO Control Register (1 byte)
#define SERIAL_FIFOCONTROL_FIFOENABLE        0x01 // [bit 0:FIFO] set whether using FIFO
#define SERIAL_FIFOCONTROL_CLEARRECEIVEFIFO  0x02 // [bit 1:CLRF] Clear Receive FIFO, set to clear Receive FIFO.
#define SERIAL_FIFOCONTROL_CLEARTRANSMITFIFO 0x04 // [bit 2:CLTF] Clear Transmit FIFO, set to clear Transmit FIFO.
#define SERIAL_FIFOCONTROL_ENABLEDMA         0x08 // [bit 3:DMA] set whether using DMA
#define SERIAL_FIFOCONTROL_1BYTEFIFO         0x00 // [bit 6~7:ITL] Interrupt Trigger Level, set data size of Receive FIFO for causing interrupt, 1 byte
#define SERIAL_FIFOCONTROL_4BYTEFIFO         0x40 // [bit 6~7:ITL] Interrupt Trigger Level, set data size of Receive FIFO for causing interrupt, 4 bytes
#define SERIAL_FIFOCONTROL_8BYTEFIFO         0x80 // [bit 6~7:ITL] Interrupt Trigger Level, set data size of Receive FIFO for causing interrupt, 8 bytes
#define SERIAL_FIFOCONTROL_14BYTEFIFO        0xC0 // [bit 6~7:ITL] Interrupt Trigger Level, set data size of Receive FIFO for causing interrupt, 14 bytes
                                                  // [bit 4~5:None] not used

// fields of Line Control Register (1 byte)
#define SERIAL_LINECONTROL_5BIT        0x00 // [bit 0~1:D-Bit] Data-Bit, set data bit length which is saved in the sending/receiving frame, 5 bits
#define SERIAL_LINECONTROL_6BIT        0x01 // [bit 0~1:D-Bit] Data-Bit, set data bit length which is saved in the sending/receiving frame, 6 bits
#define SERIAL_LINECONTROL_7BIT        0x02 // [bit 0~1:D-Bit] Data-Bit, set data bit length which is saved in the sending/receiving frame, 7 bits
#define SERIAL_LINECONTROL_8BIT        0x03 // [bit 0~1:D-Bit] Data-Bit, set data bit length which is saved in the sending/receiving frame, 8 bits (generally use 8 bits)
#define SERIAL_LINECONTROL_1BITSTOP    0x00 // [bit 2:STOP] set stop bit length, 1 bit (generally use 1 bit)
#define SERIAL_LINECONTROL_2BITSTOP    0x04 // [bit 2:STOP] set stop bit length, 2 bits or 1.5 bits
#define SERIAL_LINECONTROL_NOPARITY    0x00 // [bit 3~5:Parity] set whether using parity, not use parity.
#define SERIAL_LINECONTROL_ODDPARITY   0x08 // [bit 3~5:Parity] set whether using parity, use odd parity.
#define SERIAL_LINECONTROL_EVENPARITY  0x18 // [bit 3~5:Parity] set whether using parity, use even parity.
#define SERIAL_LINECONTROL_MARKPARITY  0x28 // [bit 3~5:Parity] set whether using parity, set parity bit to 1 always.
#define SERIAL_LINECONTROL_SPACEPARITY 0x38 // [bit 3~5:Parity] set whether using parity, set parity bit to 0 always.
#define SERIAL_LINECONTROL_BREAK       0x40 // [bit 6:BRK] Break, set whether sending Break signal which stops sending/receiving data.
#define SERIAL_LINECONTROL_DLAB        0x80 // [bit 7:DLAB] Divisor Latch Access Bit, set whether accessing to Divisor Latch Register.

// fields of Line Status Register (1 byte)
#define SERIAL_LINESTATUS_RECEIVEDDATAREADY      0x01 // [bit 0:RxRD] Receive Data Ready, mean that Receiver Buffer Register has received data.
#define SERIAL_LINESTATUS_OVERRUNERROR           0x02 // [bit 1:OVRE] Overrun Error, mean that Receive Buffer Register is totally full, so no more data to receive.
#define SERIAL_LINESTATUS_PARITYERROR            0x04 // [bit 2:PARE] Parity Error, mean that parity error has occurred in the received data.
#define SERIAL_LINESTATUS_FRAMINGERROR           0x08 // [bit 3:FRME] Frame Error, mean that frame error has occurred in the received data.
#define SERIAL_LINESTATUS_BREAKINDICATOR         0x10 // [bit 4:BREK] Break, mean that Break signal has been detected.
#define SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY    0x20 // [bit 5:TBE] Transmit Buffer Empty, mean that Transmit Buffer Register is empty.
#define SERIAL_LINESTATUS_TRANSMITEMPTY          0x40 // [bit 6:TXE] Transmit Empty, mean that currently no data is being sent.
#define SERIAL_LINESTATUS_RECEIVEDCHARACTERERROR 0x80 // [bit 7:FIFOE] FIFO Error, mean that error exists in the data saved in Receive FIFO.

// fields of Divisor Latch Register (MSB=1 byte, LSB=1 byte)
#define SERIAL_DIVISORLATCH_115200 1  // [MSB=0, LSB=1] set serial communication speed, 115200 Bd or Bps, send 14KB per 1 second, used in HansOS.
#define SERIAL_DIVISORLATCH_57600  2  // [MSB=0, LSB=2] set serial communication speed,  57600 Bd or Bps
#define SERIAL_DIVISORLATCH_38400  3  // [MSB=0, LSB=3] set serial communication speed,  38400 Bd or Bps
#define SERIAL_DIVISORLATCH_19200  6  // [MSB=0, LSB=6] set serial communication speed,  19200 Bd or Bps
#define SERIAL_DIVISORLATCH_9600   12 // [MSB=0, LSB=12] set serial communication speed,   9600 Bd or Bps
#define SERIAL_DIVISORLATCH_4800   24 // [MSB=0, LSB=24] set serial communication speed,   4800 Bd or Bps
#define SERIAL_DIVISORLATCH_2400   48 // [MSB=0, LSB=48] set serial communication speed,   2400 Bd or Bps [Bd = Baud Rate, Bps = Bit Per Second]

// etc macros
#define SERIAL_FIFOMAXSIZE 16 // FIFO max size (16 bytes)

#pragma pack(push, 1)

typedef struct k_SerialPortManager {
	Mutex mutex;
} SerialPortManager;

#pragma pack(pop)

void k_initSerialPort(void);
void k_sendSerialData(byte* buffer, int size);
int k_recvSerialData(byte* buffer, int size);
void k_clearSerialFifo(void);
static bool k_isSerialTransmitBufferEmpty(void);
static bool k_isSerialReceiveBufferFull(void);

#endif // __SERIAL_PORT_H__
