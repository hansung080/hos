#ifndef __IO_APIC_H__
#define __IO_APIC_H__

#include "types.h"

// IO APIC registers offset/index: IO APIC registers use memory map IO.
// -> IO Redirection Table Register is 64 bit-sized,
//    and 24 (0~23) these registers exist which is the same count as the interrupt input pin count of IO APIC,
//    and low register index is even number out of [0x10 ~ 0x3F],
//    and high register index is odd number out of [0x10 ~ 0x3F].
#define IOAPIC_REGISTER_IOREGISTERSELECTOR          0x00 // offset of IO Register Selector Register: 32 bits
#define IOAPIC_REGISTER_IOWINDOW                    0x10 // offset of IO Window Register: 32 bits
#define IOAPIC_REGISTERINDEX_IOAPICID               0x00 // index of IO APIC ID Register: 32 bits
#define IOAPIC_REGISTERINDEX_IOAPICVERSION          0x01 // index of IO APIC Version Register: 32 bits
#define IOAPIC_REGISTERINDEX_IOAPICARBID            0x02 // index of IO APIC Arbitration ID Register: 32 bits
#define IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE  0x10 // index of Low IO Redirection Table Register: 32 bits, index is even number out of [0x10 ~ 0x3F]
#define IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE 0x11 // index of High IO Redirection Table Register: 32 bits, index is odd number out of [0x10 ~ 0x3F]

// IO Redirection Table consists of 24 IO Redirection Table Registers.
#define IOAPIC_MAXIOREDIRECTIONTABLECOUNT 24

// IO Redirection Table Register (64 bits) - delivery mode (bit 10~8)
#define IOAPIC_DELIVERYMODE_FIXED          0x00 // 000 : fixed: use interrupt vector
#define IOAPIC_DELIVERYMODE_LOWESTPRIORITY 0x01 // 001 : lowest priority
#define IOAPIC_DELIVERYMODE_SMI            0x02 // 010 : system management interrupt
#define IOAPIC_DELIVERYMODE_RESERVED1      0x03 // 011 : reserved
#define IOAPIC_DELIVERYMODE_NMI            0x04 // 100 : non-maskable interrupt
#define IOAPIC_DELIVERYMODE_INIT           0x05 // 101 : initialization
#define IOAPIC_DELIVERYMODE_RESERVED2      0x06 // 110 : reserved
#define IOAPIC_DELIVERYMODE_EXTINT         0x07 // 111 : external interrupt from PIC

// IO Redirection Table Register (64 bits) - destination mode (bit 11)
#define IOAPIC_DESTINATIONMODE_PHYSICAL 0x00 // 0 : physical destination
#define IOAPIC_DESTINATIONMODE_LOGICAL  0x08 // 1 : logical destination

// IO Redirection Table Register (64 bits) - delivery status (bit 12): read only
#define IOAPIC_DELIVERYSTATUS_IDLE    0x00 // 0 : IO APIC is idle.
#define IOAPIC_DELIVERYSTATUS_PENDING 0x10 // 1 : IO APIC is pending to send interrupt.

// IO Redirection Table Register (64 bits) - interrupt input pin polarity (bit 13)
#define IOAPIC_POLARITY_ACTIVEHIGH 0x00 // 0 : active high
#define IOAPIC_POLARITY_ACTIVELOW  0x20 // 1 : active low

// IO Redirection Table Register (64 bits) - remote IRR (bit 14): read only
#define IOAPIC_REMOTEIRR_EOI      0x00 // 0 : Local APIC sent EOI, or interrupt has not occurred.
#define IOAPIC_REMOTEIRR_ACCEPTED 0x40 // 1 : Local APIC accepted level trigger interrupt.

// IO Redirection Table Register (64 bits) - trigger mode (bit 15)
#define IOAPIC_TRIGGERMODE_EDGE  0x00 // 0 : edge trigger
#define IOAPIC_TRIGGERMODE_LEVEL 0x80 // 1 : level trigger

// IO Redirection Table Register (64 bits) - interrupt mask (bit 16)
#define IOAPIC_INTERRUPT_NOTMASK 0x00 // 0 : not mask interrupt
#define IOAPIC_INTERRUPT_MASK    0x01 // 1 : mask interrupt

// IRQ to INTIN map max count
#define IOAPIC_MAXIRQTOINTINMAPCOUNT 16

#pragma pack(push, 1)

// IO Redirection Table Register (64 bits)
// -> IO Redirection Table Register is 64 bit-sized,
//    and 24 (0~23) these registers exist which is the same count as the interrupt input pin count of IO APIC.
// -> This structure is actually not table but entry of table.
typedef struct k_IoRedirectionTable {
	byte vector;               // interrupt vector (bit 7~0)
	byte flagsAndDeliveryMode; // trigger mode (bit 15), remote IRR (bit 14), interrupt input pin polarity (bit 13), delivery status (bit 12), destination mode (bit 11), delivery mode (bit 10~8)
	byte interruptMask;        // interrupt mask (bit 16)
	byte reserved[4];          // reserved area (bit 55~17)
	byte dest;                 // destination (bit 63~56): Local APIC ID to deliver interrupt
} IoRedirectionTable;

// IO APIC manager
typedef struct k_IoApicManager {
	qword ioApicBaseAddrOfIsa;                        // IO APIC base address of ISA: memory map IO address of IO APIC connected with ISA bus
	byte irqToIntinMap[IOAPIC_MAXIRQTOINTINMAPCOUNT]; // IRQ to INTIN map: mapping table between IRQ and interrupt input pin of IO APIC
} IoApicManager;

#pragma pack(pop)

qword k_getIoApicBaseAddrOfIsa(void);
void k_setIoRedirectionTable(IoRedirectionTable* table, byte vector, byte flagsAndDeliveryMode, byte interruptMask, byte dest);
void k_readIoRedirectionTable(int intin, IoRedirectionTable* table);
void k_writeIoRedirectionTable(int intin, const IoRedirectionTable* table);
void k_maskAllInterruptsInIoApic(void);
void k_initIoRedirectionTable(void);
void k_printIrqToIntinMap(void);

#endif // __IO_APIC_H__
