#ifndef __CORE_PIT_H__
#define __CORE_PIT_H__

#include "types.h"

// macros related with PIT
#define PIT_FREQUENCY 1193182                    // 1.193182 Mhz
#define MSTOCOUNT(x) (PIT_FREQUENCY*(x)/1000)    // MilliSecond to Count
#define USTOCOUNT(x) (PIT_FREQUENCY*(x)/1000000) // MicroSecond to Count

// PIT I/O port
#define PIT_PORT_CONTROL  0x43 // Control Register (1 byte)
#define PIT_PORT_COUNTER0 0x40 // Counter-0 Register (2 bytes)
#define PIT_PORT_COUNTER1 0x41 // Counter-1 Register (2 bytes)
#define PIT_PORT_COUNTER2 0x42 // Counter-2 Register (2 bytes)

// fields of PIT Control Register (1 byte)
#define PIT_CONTROL_COUNTER0      0x00 // SC(bit 7,6)=[00:Counter-0]
#define PIT_CONTROL_COUNTER1      0x40 // SC(bit 7,6)=[01:Counter-1]
#define PIT_CONTROL_COUNTER2      0x80 // SC(bit 7,6)=[10:Counter-2]
#define PIT_CONTROL_LSBMSBRW      0x30 // RW(bit 5,4)=[11:read/write I/O port from low byte to high byte of Counter continuously, send 2 bytes]
#define PIT_CONTROL_LATCH         0x00 // RW(bit 5,4)=[00:read current value of Counter, send 2 bytes (If set bit 00, set bit 3~0 to 0, and read I/O port from low byte to high byte continuously.)]
#define PIT_CONTROL_MODE0         0x00 // Mode(bit 3,2,1)=[000:mode 0(Interrupt during counting, signal occurs once)]
#define PIT_CONTROL_MODE2         0x04 // Mode(bit 3,2,1)=[010:mode 2(Clock rate generator, signal occurs periodically)]
#define PIT_CONTROL_BINARYCOUNTER 0x00 // BCD(bit 0)=[0:set Counter value using binary format]
#define PIT_CONTROL_BSDCOUNTER    0x01 // BCD(bit 1)=[0:set Counter value using BSD format]

// useful macros
#define PIT_COUNTER0_ONCE     (PIT_CONTROL_COUNTER0 | PIT_CONTROL_LSBMSBRW | PIT_CONTROL_MODE0 | PIT_CONTROL_BINARYCOUNTER)
#define PIT_COUNTER0_PERIODIC (PIT_CONTROL_COUNTER0 | PIT_CONTROL_LSBMSBRW | PIT_CONTROL_MODE2 | PIT_CONTROL_BINARYCOUNTER)
#define PIT_COUNTER0_LATCH    (PIT_CONTROL_COUNTER0 | PIT_CONTROL_LATCH)

void k_initPit(word count, bool periodic);
word k_readCounter0(void);
void k_waitUsingDirectPit(word count);

#endif // __CORE_PIT_H__