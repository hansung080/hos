#include "io_apic.h"
#include "mp_config_table.h"
#include "pic.h"
#include "util.h"
#include "console.h"
#include "multiprocessor.h"

static IoApicManager g_ioApicManager;

qword k_getIoApicBaseAddrOfIsa(void) {
	MpConfigManager* mpManager;
	IoApicEntry* ioApicEntry;
	
	if (g_ioApicManager.ioApicBaseAddrOfIsa == null) {
		ioApicEntry = k_findIoApicEntryForIsa();
		if (ioApicEntry != null) {
			g_ioApicManager.ioApicBaseAddrOfIsa = ioApicEntry->memMapIoAddr & 0xFFFFFFFF;
		}
	}
	
	return g_ioApicManager.ioApicBaseAddrOfIsa;
}

void k_setIoRedirectionTable(IoRedirectionTable* table, byte vector, byte flagsAndDeliveryMode, byte interruptMask, byte dest) {
	k_memset(table, 0, sizeof(IoRedirectionTable));
	
	table->vector = vector;
	table->flagsAndDeliveryMode = flagsAndDeliveryMode;
	table->interruptMask = interruptMask;
	table->dest = dest;
}

void k_readIoRedirectionTable(int intin, IoRedirectionTable* table) {
	qword* data;
	qword ioApicBaseAddr;
	
	ioApicBaseAddr = k_getIoApicBaseAddrOfIsa();
	data = (qword*)table;
	
	/* read high 32 bits of IO Redirection Table Register */
	// [Ref] IO Redirection Table is low-high register pairs, so its index is intin * 2.
	// write High IO Redirection Table Register index to IO Register Selector Register,
	// and read High IO Redirection Table Register data from IO Window Register.
	*(dword*)(ioApicBaseAddr + IOAPIC_REGISTER_IOREGISTERSELECTOR) = IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE + (intin * 2);
	*data = *(dword*)(ioApicBaseAddr + IOAPIC_REGISTER_IOWINDOW);
	*data = *data << 32;
	
	/* read low 32 bits of IO Redirection Table Register */
	// write Low IO Redirection Table Register index to IO Register Selector Register,
	// and read Low IO Redirection Table Register data from IO Window Register.
	*(dword*)(ioApicBaseAddr + IOAPIC_REGISTER_IOREGISTERSELECTOR) = IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE + (intin * 2);
	*data |= *(dword*)(ioApicBaseAddr + IOAPIC_REGISTER_IOWINDOW);
}

void k_writeIoRedirectionTable(int intin, const IoRedirectionTable* table) {
	qword* data;
	qword ioApicBaseAddr;
	
	ioApicBaseAddr = k_getIoApicBaseAddrOfIsa();
	data = (qword*)table;
	
	/* write high 32 bits of table data to IO Redirection Table Register */
	// write High IO Redirection Table Register index to IO Register Selector Register,
	// and write high 32 bits of table data to IO Window Register.
	*(dword*)(ioApicBaseAddr + IOAPIC_REGISTER_IOREGISTERSELECTOR) = IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE + (intin * 2);
	*(dword*)(ioApicBaseAddr + IOAPIC_REGISTER_IOWINDOW) = *data >> 32;
	
	/* write low 32 bits of table data to IO Redirection Table Register */
	// write Low IO Redirection Table Register index to IO Register Selector Register,
	// and write low 32 bits of table data to IO Window Register.
	*(dword*)(ioApicBaseAddr + IOAPIC_REGISTER_IOREGISTERSELECTOR) = IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE + (intin * 2);
	*(dword*)(ioApicBaseAddr + IOAPIC_REGISTER_IOWINDOW) = *data;
}

void k_maskAllInterruptsInIoApic(void) {
	IoRedirectionTable table;
	int i;
	
	for (i = 0; i < IOAPIC_MAXIOREDIRECTIONTABLECOUNT; i++) {
		k_readIoRedirectionTable(i, &table);
		table.interruptMask |= IOAPIC_INTERRUPT_MASK;
		k_writeIoRedirectionTable(i, &table);
	}
}

void k_initIoRedirectionTable(void) {
	MpConfigManager* mpManager;
	MpConfigTableHeader* mpHeader;
	IoInterruptAssignEntry* ioInterruptAssignEntry;
	IoRedirectionTable ioRedirectionTable;
	qword entryAddr;
	byte entryType;
	byte dest;
	int i;
	
	/* initialize IO APIC manager */
	k_memset(&g_ioApicManager, 0, sizeof(g_ioApicManager));
	k_getIoApicBaseAddrOfIsa();
	for (i = 0; i < IOAPIC_MAXIRQTOINTINMAPCOUNT; i++) {
		g_ioApicManager.irqToIntinMap[i] = 0xFF; // 0xFF is invalid intin.
	}
	
	/* initialize IO Redirection Table Register */
	// mask all interrupts during initialization of IO Redirection Table Register.
	// because mis-delivery of interrupt can cause problems before this initialization completes.
	k_maskAllInterruptsInIoApic();
	
	mpManager = k_getMpConfigManager();
	mpHeader = mpManager->mpConfigTableHeader;
	entryAddr = mpManager->baseEntryStartAddr;
	
	// search all IO interrupt assignment entries, and initialize IO Redirection Table Register using that entries info.
	for (i = 0; i < mpHeader->entryCount; i++) {
		entryType = *(byte*)entryAddr;
		switch (entryType) {
		case MP_ENTRYTYPE_PROCESSOR:
			entryAddr += sizeof(ProcessorEntry);
			break;
			
		case MP_ENTRYTYPE_BUS:
			entryAddr += sizeof(BusEntry);
			break;
			
		case MP_ENTRYTYPE_IOAPIC:
			entryAddr += sizeof(IoApicEntry);
			break;
			
		case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
			ioInterruptAssignEntry = (IoInterruptAssignEntry*)entryAddr;
			if (ioInterruptAssignEntry->srcBusId == mpManager->isaBusId && ioInterruptAssignEntry->interruptType == MP_INTERRUPTTYPE_INT) {
				// If it's IRQ 0 (timer), broadcast interrupt to all Local APICs in order to use it to scheduler.
				// If it's not, send interrupt to BSP APIC.
				if (ioInterruptAssignEntry->srcBusIrq == IRQ_TIMER) {
					dest = APICID_BROADCAST; // broadcast
					
				} else {
					dest = APICID_BSP; // BSP APIC ID
				}
				
				/**
				  set IO Redirection Table
				  - interrupt vector: IRQ + 32 (0x20)
				  - trigger mode: edge trigger
				  - remote IRR: - (read only)
				  - interrupt input pin polarity: active high
				  - delivery status: - (read only)
				  - destination mode: physical destination
				  - delivery mode: fixed
				  - interrupt mask: not mask interrupt
				  - destination: dest
				*/
				k_setIoRedirectionTable(&ioRedirectionTable
									   ,ioInterruptAssignEntry->srcBusIrq + PIC_IRQSTARTVECTOR
									   ,IOAPIC_TRIGGERMODE_EDGE | IOAPIC_POLARITY_ACTIVEHIGH | IOAPIC_DESTINATIONMODE_PHYSICAL | IOAPIC_DELIVERYMODE_FIXED
									   ,IOAPIC_INTERRUPT_NOTMASK
									   ,dest);
				
				// write IO Redirection Table Register
				k_writeIoRedirectionTable(ioInterruptAssignEntry->destIoApicIntin, &ioRedirectionTable);
				
				// set IRQ to INTIN map
				g_ioApicManager.irqToIntinMap[ioInterruptAssignEntry->srcBusIrq] = ioInterruptAssignEntry->destIoApicIntin;
			}
			
			entryAddr += sizeof(IoInterruptAssignEntry);
			break;
			
		case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
			entryAddr += sizeof(LocalInterruptAssignEntry);
			break;
			
		default:
			return;
		}
	}
}

void k_printIrqToIntinMap(void) {
	int i;
	
	k_printf("*** IRQ to INTIN Map ***\n");
	for (i = 0; i < IOAPIC_MAXIRQTOINTINMAPCOUNT; i++) {
		k_printf("- IRQ %d : INTIN %d\n", i, g_ioApicManager.irqToIntinMap[i]);
	}
}

void k_routeIrqToApic(int irq, byte apicId) {
	int i;
	IoRedirectionTable table;
	
	if (irq >= IOAPIC_MAXIRQTOINTINMAPCOUNT) {
		return;
	}
	
	k_readIoRedirectionTable(g_ioApicManager.irqToIntinMap[irq], &table);
	table.dest = apicId;
	k_writeIoRedirectionTable(g_ioApicManager.irqToIntinMap[irq], &table);
}

