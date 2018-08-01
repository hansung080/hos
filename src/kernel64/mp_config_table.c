#include "mp_config_table.h"
#include "console.h"
#include "util.h"

static MpConfigManager g_mpConfigManager = {0, };

bool k_findMpFloatingPointerAddr(qword* addr) {
	char* mpFloatingPointer;
	qword ebdaAddr;      // extended BIOD data area address
	qword systemBaseMem; // system base memory size
	
	//k_printf("*** MP Floating Point Searching ***\n");
	//k_printf("1> extended BIOS data area: 0x%X\n", MP_SEARCH1_EBDA_ADDRESS);
	//k_printf("2> system base memory     : 0x%X\n", MP_SEARCH2_SYSEMBASEMEMORY);
	//k_printf("3> BIOS ROM area          : 0x%X~0x%X\n", MP_SEARCH3_BIOSROM_STARTADDRESS, MP_SEARCH3_BIOSROM_ENDADDRESS);
	
	/* 1. search MP floating pointer: search it in the starting 1KB-sized range of extended BIOS data area */
	ebdaAddr = MP_SEARCH1_EBDA_ADDRESS;
	
	for (mpFloatingPointer = (char*)ebdaAddr; (qword)mpFloatingPointer <= (ebdaAddr + 1024); mpFloatingPointer++) {
		if (k_memcmp(mpFloatingPointer, MP_FLOATINGPOINTER_SIGNATURE, 4) == 0) {
			//k_printf("searching success: found in address (0x%X) of extended BIOS data area.\n", (qword)mpFloatingPointer);
			*addr = (qword)mpFloatingPointer;
			return true;
		}
	}
	
	/* 2. search MP floating pointer: search it in the ending 1KB-sized range of system base memory */
	systemBaseMem = MP_SEARCH2_SYSEMBASEMEMORY;
	
	for (mpFloatingPointer = (char*)(systemBaseMem - 1024); (qword)mpFloatingPointer <= systemBaseMem; mpFloatingPointer++) {
		if (k_memcmp(mpFloatingPointer, MP_FLOATINGPOINTER_SIGNATURE, 4) == 0) {
			//k_printf("searching success: found in address (0x%X) of system base memory.\n", (qword)mpFloatingPointer);
			*addr = (qword)mpFloatingPointer;
			return true;
		}
	}
	
	/* 3. search MP floating pointer: search it in BIOS ROM area */
	for (mpFloatingPointer = (char*)MP_SEARCH3_BIOSROM_STARTADDRESS; (qword)mpFloatingPointer < (qword)MP_SEARCH3_BIOSROM_ENDADDRESS; mpFloatingPointer++) {
		if (k_memcmp(mpFloatingPointer, MP_FLOATINGPOINTER_SIGNATURE, 4) == 0) {
			//k_printf("searching success: found in address (0x%X) of BIOS ROM area.\n", (qword)mpFloatingPointer);
			*addr = (qword)mpFloatingPointer;
			return true;
		}
	}
	
	//k_printf("search failure\n");
	
	return false;
}

bool k_analyzeMpConfigTable(void) {
	qword mpFloatingPointerAddr;
	MpFloatingPointer* mpFloatingPointer;
	MpConfigTableHeader* mpConfigTableHeader;
	byte entryType;
	word i;
	qword baseEntryAddr;
	ProcessorEntry* processorEntry;
	BusEntry* busEntry;

	k_memset(&g_mpConfigManager, 0, sizeof(MpConfigManager));
	g_mpConfigManager.isaBusId = 0xFF;

	if (k_findMpFloatingPointerAddr(&mpFloatingPointerAddr) == false) {
		return false;
	}

	// set MP floating pointer
	mpFloatingPointer = (MpFloatingPointer*)mpFloatingPointerAddr;
	g_mpConfigManager.mpFloatingPointer = mpFloatingPointer;
	mpConfigTableHeader = (MpConfigTableHeader*)((qword)mpFloatingPointer->mpConfigTableAddr & 0xFFFFFFFF);

	// set PIC mode flag
	if (mpFloatingPointer->mpFeatureByte[1] & MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE) {
		g_mpConfigManager.picMode = true;
	}

	// set MP configuration table header, base MP configuration table entry start address.
	g_mpConfigManager.mpConfigTableHeader = mpConfigTableHeader;
	g_mpConfigManager.baseEntryStartAddr = mpFloatingPointer->mpConfigTableAddr + sizeof(MpConfigTableHeader);

	// set processor count, ISA bus ID.
	baseEntryAddr = g_mpConfigManager.baseEntryStartAddr;
	for (i = 0; i < mpConfigTableHeader->entryCount; i++) {
		entryType = *(byte*)baseEntryAddr;

		switch (entryType) {
		case MP_ENTRYTYPE_PROCESSOR:
			processorEntry = (ProcessorEntry*)baseEntryAddr;
			if (processorEntry->cpuFlags & MP_PROCESSOR_CPUFLAGS_ENABLE) {
				g_mpConfigManager.processorCount++;
			}

			baseEntryAddr += sizeof(ProcessorEntry);
			break;

		case MP_ENTRYTYPE_BUS:
		    busEntry = (BusEntry*)baseEntryAddr;
		    if (k_memcmp(busEntry->busTypeStr, MP_BUS_TYPESTRING_ISA, k_strlen(MP_BUS_TYPESTRING_ISA)) == 0) {
		    	g_mpConfigManager.isaBusId = busEntry->busId;
		    }

			baseEntryAddr += sizeof(BusEntry);
			break;

		case MP_ENTRYTYPE_IOAPIC:
			baseEntryAddr += sizeof(IoApicEntry);
			break;

		case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
			baseEntryAddr += sizeof(IoInterruptAssignEntry);
			break;

		case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
			baseEntryAddr += sizeof(LocalInterruptAssignEntry);
			break;

		default:
			return false;
		}
	}
	
	return true;
}

MpConfigManager* k_getMpConfigManager(void) {
	return &g_mpConfigManager;
}

void k_printMpConfigTable(void) {
	MpConfigManager* mpConfigManager;
	qword mpFloatingPointerAddr;
	MpFloatingPointer* mpFloatingPointer;
	MpConfigTableHeader* mpConfigTableHeader;
	ProcessorEntry* processorEntry;
	BusEntry* busEntry;
	IoApicEntry* ioApicEntry;
	IoInterruptAssignEntry* ioInterruptAssignEntry;
	LocalInterruptAssignEntry* localInterruptAssignEntry;
	qword baseEntryAddr;
	char buffer[20];
	word i;
	byte entryType;
	char* interruptType[4] = {"INT", "NMI", "SMI", "ExtINT"};
	char* interruptPolarity[4] = {"Conform", "Active High", "Reserved", "Active Low"};
	char* interruptTrigger[4] = {"Conform", "Edge-Trigger", "Reserved", "Level-Trigger"};

	// call MP configuration table analysis function.
	mpConfigManager = k_getMpConfigManager();
	if (mpConfigManager->baseEntryStartAddr == 0 && k_analyzeMpConfigTable() == false) {
		return;
	}

	//----------------------------------------------------------------------------------------------------
	// print MP configuration table manager info.
	//----------------------------------------------------------------------------------------------------
	k_printf("*** MP Configuration Table Summary ***\n");

	k_printf("- MP floating pointer address                     : 0x%Q\n", mpConfigManager->mpFloatingPointer);
	k_printf("- MP configuration table header address           : 0x%Q\n", mpConfigManager->mpConfigTableHeader);
	k_printf("- base MP configuration table entry start address : 0x%Q\n", mpConfigManager->baseEntryStartAddr);
	k_printf("- processor count                                 : %d\n", mpConfigManager->processorCount);
	k_printf("- PIC mode                                        : %s\n", (mpConfigManager->picMode == true) ? "true" : "false");
	k_printf("- ISA bus ID                                      : %d\n", mpConfigManager->isaBusId);

	k_printf("Press any key to continue...('q' is quit): ");
	if (k_getch() == 'q') {
		k_printf("\n");
		return;
	}
	k_printf("\n");

	//----------------------------------------------------------------------------------------------------
	// print MP floating pointer info.
	//----------------------------------------------------------------------------------------------------
	k_printf("*** MP Floating Pointer Info ***\n");

	mpFloatingPointer = mpConfigManager->mpFloatingPointer;

	k_memcpy(buffer, mpFloatingPointer->signature, sizeof(mpFloatingPointer->signature));
	buffer[sizeof(mpFloatingPointer->signature)] = '\0';
	k_printf("- signature                      : %s\n", buffer);
	k_printf("- MP configuration table address : 0x%Q\n", mpFloatingPointer->mpConfigTableAddr);
	k_printf("- length                         : %d bytes\n", mpFloatingPointer->len * 16);
	k_printf("- revision                       : %d\n", mpFloatingPointer->revision);
	k_printf("- checksum                       : 0x%X\n", mpFloatingPointer->checksum);
	k_printf("- MP feature byte 1              : 0x%X ", mpFloatingPointer->mpFeatureByte[0]);
	if (mpFloatingPointer->mpFeatureByte[0] == MP_FLOATINGPOINTER_FEATUREBYTE1_USEMPTABLE) {
		k_printf("(use MP configuration table)\n");
	} else {
		k_printf("(use default configuration)\n");
	}
	k_printf("- MP feature byte 2              : 0x%X ", mpFloatingPointer->mpFeatureByte[1]);
	if (mpFloatingPointer->mpFeatureByte[1] & MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE) {
		k_printf("(use PIC mode)\n");
	} else {
		k_printf("(use virtual wire mode)\n");
	}
	k_printf("- MP feature byte 3              : 0x%X\n", mpFloatingPointer->mpFeatureByte[2]);
	k_printf("- MP feature byte 4              : 0x%X\n", mpFloatingPointer->mpFeatureByte[3]);
	k_printf("- MP feature byte 5              : 0x%X\n", mpFloatingPointer->mpFeatureByte[4]);

	k_printf("Press any key to continue...('q' is quit): ");
	if (k_getch() == 'q') {
		k_printf("\n");
		return;
	}
	k_printf("\n");

	//----------------------------------------------------------------------------------------------------
	// print MP configuration table header info.
	//----------------------------------------------------------------------------------------------------
	k_printf("*** MP Configuration Table Header Info ***\n");

	mpConfigTableHeader = mpConfigManager->mpConfigTableHeader;

	k_memcpy(buffer, mpConfigTableHeader->signature, sizeof(mpConfigTableHeader->signature));
	buffer[sizeof(mpConfigTableHeader->signature)] = '\0';
	k_printf("- signature                           : %s\n", buffer);
	k_printf("- base table length                   : %d bytes\n", mpConfigTableHeader->baseTableLen);
	k_printf("- revision                            : %d\n", mpConfigTableHeader->revision);
	k_printf("- checksum                            : 0x%X\n", mpConfigTableHeader->checksum);
	k_memcpy(buffer, mpConfigTableHeader->oemIdStr, sizeof(mpConfigTableHeader->oemIdStr));
	buffer[sizeof(mpConfigTableHeader->oemIdStr)] = '\0';
	k_printf("- OEM ID string                       : %s\n", buffer);
	k_memcpy(buffer, mpConfigTableHeader->productIdStr, sizeof(mpConfigTableHeader->productIdStr));
	buffer[sizeof(mpConfigTableHeader->productIdStr)] = '\0';
	k_printf("- product ID string                   : %s\n", buffer);
	k_printf("- OEM table pointer address           : 0x%X\n", mpConfigTableHeader->oemTablePointerAddr);
	k_printf("- OEM table size                      : %d bytes\n", mpConfigTableHeader->oemTableSize);
	k_printf("- entry count                         : %d\n", mpConfigTableHeader->entryCount);
	k_printf("- memory map IO address of local APIC : 0x%X\n", mpConfigTableHeader->memMapIoAddrOfLocalApic);
	k_printf("- extended table length               : %d bytes\n", mpConfigTableHeader->extendedTableLen);
	k_printf("- extended table checksum             : 0x%X\n", mpConfigTableHeader->extendedTableChecksum);
	k_printf("- reserved                            : %d\n", mpConfigTableHeader->reserved);

	k_printf("Press any key to continue...('q' is quit): ");
	if (k_getch() == 'q') {
		k_printf("\n");
		return;
	}
	k_printf("\n");

	//----------------------------------------------------------------------------------------------------
	// print base MP configuration table entry info.
	//----------------------------------------------------------------------------------------------------
	k_printf("*** Base MP Configuration Table Entry Info (%d entries)\n", mpConfigTableHeader->entryCount);

	baseEntryAddr = mpConfigManager->baseEntryStartAddr;

	for (i = 0; i < mpConfigTableHeader->entryCount; i++) {
		entryType = *(byte*)baseEntryAddr;

		k_printf("  < Entry %d >\n", i + 1);

		switch (entryType) {
		case MP_ENTRYTYPE_PROCESSOR:
			processorEntry = (ProcessorEntry*)baseEntryAddr;

			k_printf("- entry type         : processor\n");
			k_printf("- local APIC ID      : %d\n", processorEntry->localApicId);
			k_printf("- local APIC version : 0x%X\n", processorEntry->localApicVersion);
			k_printf("- CPU flags          : 0x%X ", processorEntry->cpuFlags);
			if (processorEntry->cpuFlags & MP_PROCESSOR_CPUFLAGS_ENABLE) {
				k_printf("(enable, ");
			} else {
				k_printf("(disable, ");
			}
			if (processorEntry->cpuFlags & MP_PROCESSOR_CPUFLAGS_BSP) {
				k_printf("BSP)\n");
			} else {
				k_printf("AP)\n");
			}
			k_printf("- CPU signature      : 0x%X\n", processorEntry->cpuSignature);
			k_printf("- feature flags      : 0x%X\n", processorEntry->featureFlags);
			k_printf("- reserved           : 0x%X\n", processorEntry->reserved);

			baseEntryAddr += sizeof(ProcessorEntry);
			break;

		case MP_ENTRYTYPE_BUS:
		    busEntry = (BusEntry*)baseEntryAddr;

		    k_printf("- entry type      : bus\n");
		    k_printf("- bus ID          : %d\n", busEntry->busId);
		    k_memcpy(buffer, busEntry->busTypeStr, sizeof(busEntry->busTypeStr));
		    buffer[sizeof(busEntry->busTypeStr)] = '\0';
		    k_printf("- bus type string : %s\n", buffer);

			baseEntryAddr += sizeof(BusEntry);
			break;

		case MP_ENTRYTYPE_IOAPIC:
			ioApicEntry = (IoApicEntry*)baseEntryAddr;

			k_printf("- entry type            : IO APIC\n");
			k_printf("- IO APIC ID            : %d\n", ioApicEntry->ioApicId);
			k_printf("- IO APIC version       : 0x%X\n", ioApicEntry->ioApicVersion);
			k_printf("- IO APIC flags         : 0x%X ", ioApicEntry->ioApicFlags);
			if (ioApicEntry->ioApicFlags & MP_IOAPIC_FLAGS_ENABLE) {
				k_printf("(enable)\n");
			} else {
				k_printf("(disable)\n");
			}
			k_printf("- memory map IO address : 0x%X\n", ioApicEntry->memMapIoAddr);

			baseEntryAddr += sizeof(IoApicEntry);
			break;

		case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
			ioInterruptAssignEntry = (IoInterruptAssignEntry*)baseEntryAddr;

			k_printf("- entry type         : IO interrupt assignment\n");
			k_printf("- interrupt type     : 0x%X ", ioInterruptAssignEntry->interruptType);
			k_printf("(%s)\n", interruptType[ioInterruptAssignEntry->interruptType]);
			k_printf("- interrupt flags    : 0x%X ", ioInterruptAssignEntry->interruptFlags);
			k_printf("(%s, %s)\n", interruptPolarity[ioInterruptAssignEntry->interruptFlags & 0x03]
								, interruptTrigger[(ioInterruptAssignEntry->interruptFlags >> 2) & 0x03]);
			k_printf("- src bus ID         : %d\n", ioInterruptAssignEntry->srcBusId);
			k_printf("- src bus IRQ        : %d\n", ioInterruptAssignEntry->srcBusIrq);
			k_printf("- dest IO APIC ID    : %d\n", ioInterruptAssignEntry->destIoApicId);
			k_printf("- dest IO APIC INTIN : %d\n", ioInterruptAssignEntry->destIoApicIntin);

			baseEntryAddr += sizeof(IoInterruptAssignEntry);
			break;

		case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
			localInterruptAssignEntry = (LocalInterruptAssignEntry*)baseEntryAddr;

			k_printf("- entry type             : local interrupt assignment\n");
			k_printf("- interrupt type         : 0x%X ", localInterruptAssignEntry->interruptType);
			k_printf("(%s)\n", interruptType[localInterruptAssignEntry->interruptType]);
			k_printf("- interrupt flags        : 0x%X ", localInterruptAssignEntry->interruptFlags);
			k_printf("(%s, %s)\n", interruptPolarity[localInterruptAssignEntry->interruptFlags & 0x03]
								, interruptTrigger[(localInterruptAssignEntry->interruptFlags >> 2) & 0x03]);
			k_printf("- src bus ID             : %d\n", localInterruptAssignEntry->srcBusId);
			k_printf("- src bus IRQ            : %d\n", localInterruptAssignEntry->srcBusIrq);
			k_printf("- dest local APIC ID     : %d\n", localInterruptAssignEntry->destLocalApicId);
			k_printf("- dest local APIC LINTIN : %d\n", localInterruptAssignEntry->destLocalApicLintin);

			baseEntryAddr += sizeof(LocalInterruptAssignEntry);
			break;

		default:
			k_printf("invaild entry type: %d\n", entryType);
			break;
		}

		// ask a user to print more entries, every after 2 entries are printed.
		if ((i != 0) && (((i + 1) % 2) == 0)) {
			k_printf("Press any key to continue...('q' is quit): ");
			if (k_getch() == 'q') {
				k_printf("\n");
				return;
			}
			k_printf("\n");
		} else {
			k_printf("\n");
		}
	}
}

int k_getProcessorCount(void) {
	// If MP configuration table dosen't exist, return 1 as processor count.
	if (g_mpConfigManager.processorCount == 0) {
		return 1;
	}

	return g_mpConfigManager.processorCount;
}

IoApicEntry* k_findIoApicEntryForIsa(void) {
	MpConfigTableHeader* mpHeader;
	IoInterruptAssignEntry* ioInterruptAssignEntry;
	IoApicEntry* ioApicEntry;
	qword entryAddr;
	byte entryType;
	bool found = false;
	int i;
	
	mpHeader = g_mpConfigManager.mpConfigTableHeader;
	entryAddr = g_mpConfigManager.baseEntryStartAddr;
	
	/* find IO interrupt assignment entry connected with ISA bus */
	for (i = 0; i < mpHeader->entryCount && found == false; i++) {
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
			if (ioInterruptAssignEntry->srcBusId == g_mpConfigManager.isaBusId) {
				found = true;
			}
			
			entryAddr += sizeof(IoInterruptAssignEntry);
			break;
			
		case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
			entryAddr += sizeof(LocalInterruptAssignEntry);
			break;
			
		default:
			return null;
		}
	}
	
	if (found == false) {
		return null;
	}
	
	/* find IO APIC entry connected with IO interrupt assignment entry found above */
	entryAddr = g_mpConfigManager.baseEntryStartAddr;
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
			ioApicEntry = (IoApicEntry*)entryAddr;
			if (ioApicEntry->ioApicId == ioInterruptAssignEntry->destIoApicId) {
				return ioApicEntry;
			}
			
			entryAddr += sizeof(IoApicEntry);
			break;
			
		case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
			entryAddr += sizeof(IoInterruptAssignEntry);
			break;
			
		case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
			entryAddr += sizeof(LocalInterruptAssignEntry);
			break;
			
		default:
			return null;
		}
	}
	
	return null;
}

