#ifndef __LOCAL_APIC_H__
#define __LOCAL_APIC_H__

#include "types.h"

// Local APIC registers offset: Local APIC registers use memory map IO.
// LVT means Local Vector Table
#define LAPIC_REGISTER_APICID                       0x000020 // offset of Local APIC ID Register: 32 bits
#define LAPIC_REGISTER_TASKPRIORITY                 0x000080 // offset of Task Priority Register: 32 bits
#define LAPIC_REGISTER_EOI                          0x0000B0 // offset of EOI Register: 32 bits
#define LAPIC_REGISTER_SVR                          0x0000F0 // offset of Spurious Interrupt Vector Register: 32 bits
#define LAPIC_REGISTER_ICRLOWER                     0x000300 // offset of Lower Interrupt Command Register: 32 bits
#define LAPIC_REGISTER_ICRUPPER                     0x000310 // offset of Upper Interrupt Command Register: 32 bits
#define LAPIC_REGISTER_TIMER                        0x000320 // offset of LVT Timer Register: 32 bits
#define LAPIC_REGISTER_THERMALSENSOR                0x000330 // offset of LVT Thermal Sensor Register: 32 bits
#define LAPIC_REGISTER_PERFORMANCEMONITERINGCOUNTER 0x000340 // offset of LVT Performance Monitering Counter Register: 32 bits
#define LAPIC_REGISTER_LINT0                        0x000350 // offset of LVT LINT0 Register: 32 bits
#define LAPIC_REGISTER_LINT1                        0x000360 // offset of LVT LINT1 Register: 32 bits
#define LAPIC_REGISTER_ERROR                        0x000370 // offset of LVT Error Register: 32 bits

// Spurious Interrupt Vector Register (32 bits) - local APIC software enable/disable (bit 8)
#define LAPIC_SOFTWARE_DISABLE 0x000 // 0 : local APIC disable
#define LAPIC_SOFTWARE_ENABLE  0x100 // 1 : local APIC enable

// Interrupt Command Register (64 bits) - interrupt vector (bit 7~0)
#define LAPIC_VECTOR_KERNEL32STARTADDRESS 0x10 // 0x10=0x10000/4KB : start address of kernel32

// Interrupt Command Register (64 bits) - delivery mode (bit 10~8)
#define LAPIC_DELIVERYMODE_FIXED          0x000000 // 000 : fixed: use interrupt vector
#define LAPIC_DELIVERYMODE_LOWESTPRIORITY 0x000100 // 001 : lowest priority
#define LAPIC_DELIVERYMODE_SMI            0x000200 // 010 : system management interrupt
#define LAPIC_DELIVERYMODE_RESERVED       0x000300 // 011 : reserved
#define LAPIC_DELIVERYMODE_NMI            0x000400 // 100 : non-maskable interrupt
#define LAPIC_DELIVERYMODE_INIT           0x000500 // 101 : initialization
#define LAPIC_DELIVERYMODE_STARTUP        0x000600 // 110 : start up
#define LAPIC_DELIVERYMODE_EXTINT         0x000700 // 111 : external interrupt from PIC

// Interrupt Command Register (64 bits) - destination mode (bit 11)
#define LAPIC_DESTINATIONMODE_PHYSICAL 0x000000 // 0 : physical destination
#define LAPIC_DESTINATIONMODE_LOGICAL  0x000800 // 1 : logical destination

// Interrupt Command Register (64 bits) - delivery status (bit 12)
#define LAPIC_DELIVERYSTATUS_IDLE    0x000000 // 0 : Local APIC is idle.
#define LAPIC_DELIVERYSTATUS_PENDING 0x001000 // 1 : Local APIC is pending to send IPI.

// Interrupt Command Register (64 bits) - level (bit 14)
#define LAPIC_LEVEL_DEASSERT 0x000000 // 0 : deassert
#define LAPIC_LEVEL_ASSERT   0x004000 // 1 : assert

// Interrupt Command Register (64 bits) - trigger mode (bit 15)
#define LAPIC_TRIGGERMODE_EDGE  0x000000 // 0 : edge trigger
#define LAPIC_TRIGGERMODE_LEVEL 0x008000 // 1 : level trigger

// Interrupt Command Register (64 bits) - destination shorthand (bit 19~18)
#define LAPIC_DESTINATIONSHORTHAND_NOSHORTHAND      0x000000 // 00 : no shorthand
#define LAPIC_DESTINATIONSHORTHAND_SELF             0x040000 // 01 : self
#define LAPIC_DESTINATIONSHORTHAND_ALLINCLUDINGSELF 0x080000 // 10 : all local APICs including self
#define LAPIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF 0x0C0000 // 11 : all local APICs excluding self

// Local Vector Table Register (32 bits) - interrupt input pin polarity (bit 13)
#define LAPIC_POLARITY_ACTIVEHIGH 0x000000 // 0 : active high
#define LAPIC_POLARITY_ACTIVELOW  0x002000 // 1 : active low

// Local Vector Table Register (32 bits) - interrupt mask (bit 16)
#define LAPIC_INTERRUPT_NOTMASK 0x000000 // 0 : not mask interrupt
#define LAPIC_INTERRUPT_MASK    0x010000 // 1 : mask interrupt

// LVT Timer Register (32 bits) - timer mode (bit 17)
#define LAPIC_TIMERMODE_ONCE     0x000000 // 0 : once
#define LAPIC_TIMERMODE_PERIODIC 0x020000 // 1 : periodic

qword k_getLocalApicBaseAddr(void);
void k_enableSoftwareLocalApic(void);
void k_sendEoiToLocalApic(void);
void k_setInterruptPriority(byte priority);
void k_initLocalVectorTable(void);

#endif // __LOCAL_APIC_H__
