#ifndef __LOCAL_APIC_H__
#define __LOCAL_APIC_H__

#include "types.h"

// Local APIC Register offset
#define APIC_REGISTER_APICID                       0x000020 // Local APIC ID Register
#define APIC_REGISTER_TASKPRIORITY                 0x000080 // Task Priority Register
#define APIC_REGISTER_EOI                          0x0000B0 // EOI Register
#define APIC_REGISTER_SVR                          0x0000F0 // Spurious Interrupt Vector Register
#define APIC_REGISTER_ICRLOWER                     0x000300 // Lower Interrupt Command Register
#define APIC_REGISTER_ICRUPPER                     0x000310 // Upper Interrupt Command Register
#define APIC_REGISTER_TIMER                        0x000320 // LVT Timer Register
#define APIC_REGISTER_THERMALSENSOR                0x000330 // LVT Thermal Sensor Register
#define APIC_REGISTER_PERFORMANCEMONITERINGCOUNTER 0x000340 // LVT Performance Monitering Counter Register
#define APIC_REGISTER_LINT0                        0x000350 // LVT LINT0 Register
#define APIC_REGISTER_LINT1                        0x000360 // LVT LINT1 Register
#define APIC_REGISTER_ERROR                        0x000370 // LVT Error Register

// Spurious Interrupt Vector Register (32 bits) - local APIC software enable/disable (bit 8)
#define APIC_SOFTWARE_DISABLE 0x000 // 0 : local APIC disable
#define APIC_SOFTWARE_ENABLE  0x100 // 1 : local APIC enable

// Interrupt Command Register (64 bits) - delivery mode (bit 8~10)
#define APIC_DELIVERYMODE_FIXED          0x000000 // 000 : fixed
#define APIC_DELIVERYMODE_LOWESTPRIORITY 0x000100 // 001 : lowest priority
#define APIC_DELIVERYMODE_SMI            0x000200 // 010 : system management interrupt
#define APIC_DELIVERYMODE_RESERVED       0x000300 // 011 : reserved
#define APIC_DELIVERYMODE_NMI            0x000400 // 100 : non-maskable interrupt
#define APIC_DELIVERYMODE_INIT           0x000500 // 101 : initialization
#define APIC_DELIVERYMODE_STARTUP        0x000600 // 110 : start up
#define APIC_DELIVERYMODE_EXTINT         0x000700 // 111 : reserved

// Interrupt Command Register (64 bits) - destination mode (bit 11)
#define APIC_DESTINATIONMODE_PHYSICAL 0x000000 // 0 : physical destination
#define APIC_DESTINATIONMODE_LOGICAL  0x000800 // 1 : logical destination

// Interrupt Command Register (64 bits) - delivery status (bit 12)
#define APIC_DELIVERYSTATUS_IDLE    0x000000 // 0 : idle
#define APIC_DELIVERYSTATUS_PENDING 0x001000 // 1 : pending

// Interrupt Command Register (64 bits) - level (bit 14)
#define APIC_LEVEL_DEASSERT 0x000000 // 0 : deassert
#define APIC_LEVEL_ASSERT   0x004000 // 1 : assert

// Interrupt Command Register (64 bits) - trigger mode (bit 15)
#define APIC_TRIGGERMODE_EDGE  0x000000 // 0 : edge trigger
#define APIC_TRIGGERMODE_LEVEL 0x008000 // 1 : level trigger

// Interrupt Command Register (64 bits) - destination shorthand (bit 18~19)
#define APIC_DESTINATIONSHORTHAND_NOSHORTHAND      0x000000 // 00 : no shorthand
#define APIC_DESTINATIONSHORTHAND_SELF             0x040000 // 01 : self
#define APIC_DESTINATIONSHORTHAND_ALLINCLUDINGSELF 0x080000 // 10 : all local APICs including self
#define APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF 0x0C0000 // 11 : all local APICs excluding self

// Interrupt Command Register (64 bits) - vector (bit 0~7)
#define APIC_VECTOR_KERNEL32STARTADDRESS 0x10 // 0x10=0x10000/4KB : start address of kernel32

qword k_getLocalApicBaseAddr(void);
void k_enableSoftwareLocalApic(void);

#endif // __LOCAL_APIC_H__
