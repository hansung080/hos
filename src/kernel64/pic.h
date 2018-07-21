#ifndef __PIC_H__
#define __PIC_H__

#include "types.h"

/**
  < PIC Controller with IRQ >
   - IRQ 0 : timer (PIT)                      -> master PIC pin 0 -> interrupt vector 32 (0x20)
   - IRQ 1 : PS/2 keyboard                    -> master PIC pin 1 -> interrupt vector 33 (0x21)
   - IRQ 2 : slave PIC                        -> master PIC pin 2 -> interrupt vector 34 (0x22)
   - IRQ 3 : serial port 2 (COM port 2)       -> master PIC pin 3 -> interrupt vector 35 (0x23)
   - IRQ 4 : serial port 1 (COM port 1)       -> master PIC pin 4 -> interrupt vector 36 (0x24)
   - IRQ 5 : parallel port 2 (printer port 2) -> master PIC pin 5 -> interrupt vector 37 (0x25)
   - IRQ 6 : floppy disk                      -> master PIC pin 6 -> interrupt vector 38 (0x26)
   - IRQ 7 : parallel port 1 (printer port 1) -> master PIC pin 7 -> interrupt vector 39 (0x27)
   - IRQ 8 : RTC                              ->  slave PIC pin 0 -> interrupt vector 40 (0x28)
   - IRQ 9 : reserved                         ->  slave PIC pin 1 -> interrupt vector 41 (0x29)
   - IRQ 10 : not used                        ->  slave PIC pin 2 -> interrupt vector 42 (0x2A)
   - IRQ 11 : not used                        ->  slave PIC pin 3 -> interrupt vector 43 (0x2B)
   - IRQ 12 : PS/2 mouse                      ->  slave PIC pin 4 -> interrupt vector 44 (0x2C)
   - IRQ 13 : coprocessor                     ->  slave PIC pin 5 -> interrupt vector 45 (0x2D)
   - IRQ 14 : hard disk 1                     ->  slave PIC pin 6 -> interrupt vector 46 (0x2E)
   - IRQ 15 : hard disk 2                     ->  slave PIC pin 7 -> interrupt vector 47 (0x2F)
*/

// I/O port
#define PIC_MASTER_PORT1 0x20
#define PIC_MASTER_PORT2 0x21
#define PIC_SLAVE_PORT1  0xA0
#define PIC_SLAVE_PORT2  0xA1

// start position of interrupt vector in IDT: IRQ 0~15 -> vector 32(0x20)~47(0x2F)
#define PIC_IRQSTARTVECTOR 0x20

void k_initPic(void);
void k_maskPicInterrupt(word irqBitmask);
void k_sendEoiToPic(int irqNumber);

#endif // __PIC_H__
