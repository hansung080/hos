#ifndef __PIC_H__
#define __PIC_H__

#include "types.h"

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
