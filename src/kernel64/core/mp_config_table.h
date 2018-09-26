#ifndef __CORE_MPCONFIGTABLE_H__
#define __CORE_MPCONFIGTABLE_H__

#include "types.h"

// MP floating pointer signature
#define MP_FLOATINGPOINTER_SIGNATURE "_MP_"

// MP floating pointer searching
#define MP_SEARCH1_EBDA_ADDRESS         (*(word*)0x040E) * 16   // extended BIOS data area address: Physical address was multiplied by 16, because it's segment start address in the address <0x040E>.
#define MP_SEARCH2_SYSEMBASEMEMORY      (*(word*)0x0413) * 1024 // system base memory size: Physical address was multiplied by 1024, because it's KByte unit in the address <0x0413>.
#define MP_SEARCH3_BIOSROM_STARTADDRESS 0x0F0000                // BIOS ROM area start address
#define MP_SEARCH3_BIOSROM_ENDADDRESS   0x0FFFFF                // BIOS ROM area end address

// MP floating pointer - MP feature byte 1~5 (1 byte * 5)
#define MP_FLOATINGPOINTER_FEATUREBYTE1_USEMPTABLE 0x00 // use MP configuration table (0:use MP configuration table, !0:use default configuration defined in MultiProcessor Specification)
#define MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE    0x80 // use PIC mode(bit 7=1: use PIC mode, bit 7=0: use virtual wire mode, bit 6~0: reserved)

// base MP configuration table entry - entry type (1 byte)
#define MP_ENTRYTYPE_PROCESSOR                0 // processor entry
#define MP_ENTRYTYPE_BUS                      1 // bus entry
#define MP_ENTRYTYPE_IOAPIC                   2 // IO APIC entry
#define MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT    3 // IO interrupt assignment entry
#define MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT 4 // local interrupt assignment entry

// processor entry - CPU flag (1 byte)
#define MP_PROCESSOR_CPUFLAGS_ENABLE 0x01 // processor enable(bit 0=1: processor enable, bit 0=0: processor disable)
#define MP_PROCESSOR_CPUFLAGS_BSP    0x02 // BSP (bit 1=1: BSP, bit 1=0: AP). BSP: Bootstrap Processor, AP: Application Processor

// bus entry - bus type string (6 bytes)
#define MP_BUS_TYPESTRING_ISA          "ISA"    // Industry Standard Architecture
#define MP_BUS_TYPESTRING_PCI          "PCI"    // Peripheral Component Interconnect
#define MP_BUS_TYPESTRING_PCMCIA       "PCMCIA" // PC Memory Card International Association
#define MP_BUS_TYPESTRING_VESALOCALBUS "VL"     // VESA Local Bus

// IO APIC entry - IO APIC flag (1 byte)
#define MP_IOAPIC_FLAGS_ENABLE 0x01 // can use IO APIC (bit 0=1: can use IO APIC, bit 0=0:can't use IO APIC)

// IO interrupt assignment entry, local interrupt assignment entry - interrupt type (1 byte)
#define MP_INTERRUPTTYPE_INT    0 // vector interrupt which is passed to local APIC through interrupt vector of IO APIC
#define MP_INTERRUPTTYPE_NMI    1 // non-maskable interrupt
#define MP_INTERRUPTTYPE_SMI    2 // system management interrupt
#define MP_INTERRUPTTYPE_EXTINT 3 // interrupt which is passed from PIC

/**
  IO interrupt assignment entry, local interrupt assignment entry - interrupt flag (2 bytes)
  - PO (polarity, 2 bits)
      bit 1=0 and bit 0=0 : depends on bus type
      bit 1=0 and bit 0=1 : active high
      bit 1=1 and bit 0=0 : reserved
      bit 1=1 and bit 0=1 : active low

  - EL (edge/level trigger, 2 bits)
      bit 3=0 and bit 2=0 : depends on bus type
      bit 3=0 and bit 2=1 : edge trigger
      bit 3=1 and bit 2=0 : reserved
      bit 3=1 and bit 2=1 : level trigger
 */
#define MP_INTERRUPTFLAGS_CONFORMPOLARITY 0x00 // set polarity depends on bus type
#define MP_INTERRUPTFLAGS_ACTIVEHIGH      0x01 // active high
#define MP_INTERRUPTFLAGS_ACTIVELOW       0x03 // active low
#define MP_INTERRUPTFLAGS_CONFORMTRIGGER  0x00 // set trigger mode depends on bus type
#define MP_INTERRUPTFLAGS_EDGETRIGGERED   0x04 // edge trigger
#define MP_INTERRUPTFLAGS_LEVELTRIGGERED  0x0C // level trigger

#pragma pack(push, 1)

// MP floating pointer (16 bytes)
typedef struct k_MpFloatingPointer {
	char signature[4];       // signature (_MP_)
	dword mpConfigTableAddr; // MP configuration table address
	byte len;                // MP floating pointer length (16 bytes)
	byte revision;           // MultiProcessor Specification version
	byte checksum;           // checksum
	byte mpFeatureByte[5];   // MP feature byte 1~5
} MpFloatingPointer;

// MP configuration table header (44 bytes)
typedef struct k_MpConfigTableHeader {
	char signature[4];             // signature (PCMP)
	word baseTableLen;             // base table length
	byte revision;                 // MultiProcessor Specification version
	byte checksum;                 // checksum
	char oemIdStr[8];              // OEM ID string
	char productIdStr[12];         // PRODUCT ID string
	dword oemTablePointerAddr;     // OEM table pointer
	word oemTableSize;             // OEM table size
	word entryCount;               // base MP configuration table entry count
	dword memMapIoAddrOfLocalApic; // memory map IO address of local APIC
	word extendedTableLen;         // extended table length
	byte extendedTableChecksum;    // extended table checksum
	byte reserved;                 // reserved area
} MpConfigTableHeader;

// base MP configuration table entry - processor entry (20 bytes)
typedef struct k_ProcessorEntry {
	byte entryType;        // entry type (0)
	byte localApicId;      // local APIC ID
	byte localApicVersion; // local APIC version
	byte cpuFlags;         // CPU flag
	byte cpuSignature[4];  // CPU signature
	dword featureFlags;    // feature flag
	dword reserved[2];     // reserved area
} ProcessorEntry;

// base MP configuration table entry - bus entry (8 bytes)
typedef struct k_BusEntry {
	byte entryType;     // entry type (1)
	byte busId;         // bus ID
	char busTypeStr[6]; // bus type string
} BusEntry;

// base MP configuration table entry - IO APIC entry (8 bytes)
typedef struct k_IoApicEntry {
	byte entryType;     // entry type (2)
	byte ioApicId;      // IO APIC ID
	byte ioApicVersion; // IO APIC version
	byte ioApicFlags;   // IO APIC flag
	dword memMapIoAddr; // memory map IO address of IO APIC
} IoApicEntry;

// base MP configuration table entry - IO interrupt assignment entry (8 bytes)
typedef struct k_IoInterruptAssignEntry {
	byte entryType;       // entry type (3)
	byte interruptType;   // interrupt type
	word interruptFlags;  // interrupt flag
	byte srcBusId;        // source bus ID
	byte srcBusIrq;       // source bus IRQ
	byte destIoApicId;    // destination IO APIC ID
	byte destIoApicIntin; // destination IO APIC INTIN
} IoInterruptAssignEntry;

// base MP configuration table entry - local interrupt assignment entry (8 bytes)
typedef struct k_LocalInterruptAssignEntry {
	byte entryType;           // entry type (4)
	byte interruptType;       // interrupt type
	word interruptFlags;      // interrupt flag
	byte srcBusId;            // source bus ID
	byte srcBusIrq;           // source bus IRQ
	byte destLocalApicId;     // destination local APIC ID
	byte destLocalApicLintin; // destination local APIC LINTIN
} LocalInterruptAssignEntry;

// MP configuration table manager
typedef struct k_MpConfigManager {
	MpFloatingPointer* mpFloatingPointer;     // MP floating pointer address
	MpConfigTableHeader* mpConfigTableHeader; // MP configuration table header address
	qword baseEntryStartAddr;                 // base MP configuration table entry start address
	int processorCount;                       // processor count: [REF] processor means processor of multiprocessor or processor's core of multi-core processor.
	bool picMode;                             // PIC mode flag
	byte isaBusId;                            // ISA bus ID
} MpConfigManager;

#pragma pack(pop)

bool k_findMpFloatingPointerAddr(qword* addr);
bool k_analyzeMpConfigTable(void);
MpConfigManager* k_getMpConfigManager(void);
void k_printMpConfigTable(void);
int k_getProcessorCount(void);
IoApicEntry* k_findIoApicEntryForIsa(void);

#endif // __CORE_MPCONFIGTABLE_H__