#ifndef __PIC_H__
#define __PIC_H__

#include "types.h"

/**
  < PIC Controller with IRQ >
   - IRQ 0 : timer (PIT controller)           -> master PIC pin 0 -> interrupt vector 32 (0x20)
   - IRQ 1 : PS/2 keyboard                    -> master PIC pin 1 -> interrupt vector 33 (0x21)
   - IRQ 2 : slave PIC controller             -> master PIC pin 2 -> interrupt vector 34 (0x22)
   - IRQ 3 : serial port 2 (COM port 2)       -> master PIC pin 3 -> interrupt vector 35 (0x23)
   - IRQ 4 : serial port 1 (COM port 1)       -> master PIC pin 4 -> interrupt vector 36 (0x24)
   - IRQ 5 : parallel port 2 (printer port 2) -> master PIC pin 5 -> interrupt vector 37 (0x25)
   - IRQ 6 : floppy disk controller           -> master PIC pin 6 -> interrupt vector 38 (0x26)
   - IRQ 7 : parallel port 1 (printer port 1) -> master PIC pin 7 -> interrupt vector 39 (0x27)
   - IRQ 8 : RTC                              ->  slave PIC pin 0 -> interrupt vector 40 (0x28)
   - IRQ 9 : reserved                         ->  slave PIC pin 1 -> interrupt vector 41 (0x29)
   - IRQ 10 : not used 1                      ->  slave PIC pin 2 -> interrupt vector 42 (0x2A)
   - IRQ 11 : not used 2                      ->  slave PIC pin 3 -> interrupt vector 43 (0x2B)
   - IRQ 12 : PS/2 mouse                      ->  slave PIC pin 4 -> interrupt vector 44 (0x2C)
   - IRQ 13 : coprocessor                     ->  slave PIC pin 5 -> interrupt vector 45 (0x2D)
   - IRQ 14 : hard disk 1 (HDD1)              ->  slave PIC pin 6 -> interrupt vector 46 (0x2E)
   - IRQ 15 : hard disk 2 (HDD2)              ->  slave PIC pin 7 -> interrupt vector 47 (0x2F)
*/

// IRQ
#define IRQ_TIMER         0  // timer (PIT controller)
#define IRQ_KEYBOARD      1  // PS/2 keyboard
#define IRQ_SLAVEPIC      2  // slave PIC controller
#define IRQ_SERIALPORT2   3  // serial port 2 (COM port 2)
#define IRQ_SERIALPORT1   4  // serial port 1 (COM port 1)
#define IRQ_PARALLELPORT2 5  // parallel port 2 (printer port 2)
#define IRQ_FLOPPYDISK    6  // floppy disk controller
#define IRQ_PARALLELPORT1 7  // parallel port 1 (printer port 1)
#define IRQ_RTC           8  // RTC
#define IRQ_RESERVED      9  // reserved
#define IRQ_NOTUSED1      10 // not used 1
#define IRQ_NOTUSED2      11 // not used 2
#define IRQ_MOUSE         12 // PS/2 mouse
#define IRQ_COPROCESSOR   13 // coprocessor
#define IRQ_HDD1          14 // hard disk 1 (HDD1)
#define IRQ_HDD2          15 // hard disk 2 (HDD2)

// I/O port
#define PIC_MASTER_PORT1 0x20
#define PIC_MASTER_PORT2 0x21
#define PIC_SLAVE_PORT1  0xA0
#define PIC_SLAVE_PORT2  0xA1

// start position of interrupt vector in IDT: IRQ 0~15 -> vector 32(0x20)~47(0x2F)
#define PIC_IRQSTARTVECTOR 0x20 // 32

void k_initPic(void);
void k_maskPicInterrupt(word irqBitmask);
void k_sendEoiToPic(int irqNumber);

#endif // __PIC_H__
