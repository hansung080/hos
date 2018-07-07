#include "mp_config_table.h"
#include "console.h"
#include "util.h"

static MpConfigManager g_mpConfigManager = {0, };

bool k_findMpFloatingPointerAddr(qword* addr) {
	char* mpFloatingPointer;
	qword ebdaAddr;      // extended BIOD data area address
	qword systemBaseMem; // system basic memory size

	k_printf("====>>>> MP Floating Point Search\n");
	k_printf("1. Extended BIOS Data Area = [0x%X]\n", MP_SEARCH1_EBDA_ADDRESS);
	k_printf("2. System Base Memory      = [0x%X]\n", MP_SEARCH2_SYSEMBASEMEMORY);
	k_printf("3. BIOS ROM Area           = [0x%X~0x%X]\n", MP_SEARCH3_BIOSROM_STARTADDRESS, MP_SEARCH3_BIOSROM_ENDADDRESS);

	/* 1. search MP floating pointer: search it in the starting 1KB-sized range of extended BIOS data area */
	ebdaAddr = MP_SEARCH1_EBDA_ADDRESS;

	for (mpFloatingPointer = (char*)ebdaAddr; (qword)mpFloatingPointer <= (ebdaAddr + 1024); mpFloatingPointer++) {
		if (k_memcmp(mpFloatingPointer, MP_FLOATINGPOINTER_SIGNATURE, 4) == 0) {
			k_printf("Search Success : MP Floating Pointer is in address[0x%X] of Extended BIOS Data Area.\n\n", (qword)mpFloatingPointer);
			*addr = (qword)mpFloatingPointer;
			return true;
		}
	}

	/* 2. search MP floating pointer: search it in the ending 1KB-sized range of system basic memory */
	systemBaseMem = MP_SEARCH2_SYSEMBASEMEMORY;

	for (mpFloatingPointer = (char*)(systemBaseMem - 1024); (qword)mpFloatingPointer <= systemBaseMem; mpFloatingPointer++) {
		if (k_memcmp(mpFloatingPointer, MP_FLOATINGPOINTER_SIGNATURE, 4) == 0) {
			k_printf("Search Success : MP Floating Pointer is in address[0x%X] of System Base Memory.\n\n", (qword)mpFloatingPointer);
			*addr = (qword)mpFloatingPointer;
			return true;
		}
	}

	/* 3. search MP floating pointer: search it in BIOS ROM area */
	for (mpFloatingPointer = (char*)MP_SEARCH3_BIOSROM_STARTADDRESS; (qword)mpFloatingPointer < (qword)MP_SEARCH3_BIOSROM_ENDADDRESS; mpFloatingPointer++) {
		if (k_memcmp(mpFloatingPointer, MP_FLOATINGPOINTER_SIGNATURE, 4) == 0) {
			k_printf("Search Success : MP Floating Pointer is in address[0x%X] of BIOS ROM Area.\n\n", (qword)mpFloatingPointer);
			*addr = (qword)mpFloatingPointer;
			return true;
		}
	}

	k_printf("Search Fail\n\n");

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

	k_printf("====>>>> MP Configuration Table Analysis\n");

	// set MP floating pointer
	mpFloatingPointer = (MpFloatingPointer*)mpFloatingPointerAddr;
	g_mpConfigManager.mpFloatingPointer = mpFloatingPointer;
	mpConfigTableHeader = (MpConfigTableHeader*)((qword)mpFloatingPointer->mpConfigTableAddr & 0xFFFFFFFF);

	// set PIC mode support flag
	if (mpFloatingPointer->mpFeatureByte[1] & MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE) {
		g_mpConfigManager.usePicMode = true;
	}

	// set MP configuration table header, basic MP configuration table entry start address.
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
			k_printf("Analysis Fail : Unknown Entry Type (%d)\n\n", entryType);
			return false;
		}
	}

	k_printf("Analysis Success\n\n");

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
	k_printf("====>>>> MP Configuration Table Summary\n");

	k_printf("- MP Floating Pointer Address                     : 0x%Q\n", mpConfigManager->mpFloatingPointer);
	k_printf("- MP Configuration Table Header Address           : 0x%Q\n", mpConfigManager->mpConfigTableHeader);
	k_printf("- Base MP Configuration Table Entry Start Address : 0x%Q\n", mpConfigManager->baseEntryStartAddr);
	k_printf("- Processor/Core Count                            : %d\n", mpConfigManager->processorCount);
	k_printf("- PIC Mode Support                                : %s\n", (mpConfigManager->usePicMode == true) ? "true" : "false");
	k_printf("- ISA Bus ID                                      : %d\n", mpConfigManager->isaBusId);

	k_printf("Press any key to continue...('q' is exit):");
	if (k_getch() == 'q') {
		k_printf("\n");
		return;
	}
	k_printf("\n");

	//----------------------------------------------------------------------------------------------------
	// print MP floating pointer info.
	//----------------------------------------------------------------------------------------------------
	k_printf("\n====>>> MP Floating Pointer Info\n");

	mpFloatingPointer = mpConfigManager->mpFloatingPointer;

	k_memcpy(buffer, mpFloatingPointer->signature, sizeof(mpFloatingPointer->signature));
	buffer[sizeof(mpFloatingPointer->signature)] = '\0';
	k_printf("- Signature                      : %s\n", buffer);
	k_printf("- MP Configuration Table Address : 0x%Q\n", mpFloatingPointer->mpConfigTableAddr);
	k_printf("- Length                         : %d bytes\n", mpFloatingPointer->len * 16);
	k_printf("- Revision                       : %d\n", mpFloatingPointer->revision);
	k_printf("- Checksum                       : 0x%X\n", mpFloatingPointer->checksum);
	k_printf("- MP Feature Byte 1              : 0x%X ", mpFloatingPointer->mpFeatureByte[0]);
	if (mpFloatingPointer->mpFeatureByte[0] == MP_FLOATINGPOINTER_FEATUREBYTE1_USEMPTABLE) {
		k_printf("(Use MP Configuration Table)\n");
	} else {
		k_printf("(Use Default Configuration)\n");
	}
	k_printf("- MP Feature Byte 2              : 0x%X ", mpFloatingPointer->mpFeatureByte[1]);
	if (mpFloatingPointer->mpFeatureByte[1] & MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE) {
		k_printf("(PIC Mode Support)\n");
	} else {
		k_printf("(Virtual Wire Mode Support)\n");
	}
	k_printf("- MP Feature Byte 3              : 0x%X\n", mpFloatingPointer->mpFeatureByte[2]);
	k_printf("- MP Feature Byte 4              : 0x%X\n", mpFloatingPointer->mpFeatureByte[3]);
	k_printf("- MP Feature Byte 5              : 0x%X\n", mpFloatingPointer->mpFeatureByte[4]);

	k_printf("Press any key to continue...('q' is exit):");
	if (k_getch() == 'q') {
		k_printf("\n");
		return;
	}
	k_printf("\n");

	//----------------------------------------------------------------------------------------------------
	// print MP configuration table header info.
	//----------------------------------------------------------------------------------------------------
	k_printf("\n====>>>> MP Configuration Table Header Info\n");

	mpConfigTableHeader = mpConfigManager->mpConfigTableHeader;

	k_memcpy(buffer, mpConfigTableHeader->signature, sizeof(mpConfigTableHeader->signature));
	buffer[sizeof(mpConfigTableHeader->signature)] = '\0';
	k_printf("- Signature                           : %s\n", buffer);
	k_printf("- Base Table Length                   : %d bytes\n", mpConfigTableHeader->baseTableLen);
	k_printf("- Revision                            : %d\n", mpConfigTableHeader->revision);
	k_printf("- Checksum                            : 0x%X\n", mpConfigTableHeader->checksum);
	k_memcpy(buffer, mpConfigTableHeader->oemIdStr, sizeof(mpConfigTableHeader->oemIdStr));
	buffer[sizeof(mpConfigTableHeader->oemIdStr)] = '\0';
	k_printf("- OEM ID String                       : %s\n", buffer);
	k_memcpy(buffer, mpConfigTableHeader->productIdStr, sizeof(mpConfigTableHeader->productIdStr));
	buffer[sizeof(mpConfigTableHeader->productIdStr)] = '\0';
	k_printf("- Product ID String                   : %s\n", buffer);
	k_printf("- OEM Table Pointer Address           : 0x%X\n", mpConfigTableHeader->oemTablePointerAddr);
	k_printf("- OEM Table Size                      : %d bytes\n", mpConfigTableHeader->oemTableSize);
	k_printf("- Entry Count                         : %d\n", mpConfigTableHeader->entryCount);
	k_printf("- Memory Map IO Address Of Local APIC : 0x%X\n", mpConfigTableHeader->memMapIoAddrOfLocalApic);
	k_printf("- Extended Table Length               : %d bytes\n", mpConfigTableHeader->extendedTableLen);
	k_printf("- Extended Table Checksum             : 0x%X\n", mpConfigTableHeader->extendedTableChecksum);
	k_printf("- Reserved                            : %d\n", mpConfigTableHeader->reserved);

	k_printf("Press any key to continue...('q' is exit):");
	if (k_getch() == 'q') {
		k_printf("\n");
		return;
	}
	k_printf("\n");

	//----------------------------------------------------------------------------------------------------
	// print basic MP configuration table entry info.
	//----------------------------------------------------------------------------------------------------
	k_printf("\n====>>>> Base MP Configuration Table Entry Info (%d Entries)\n", mpConfigTableHeader->entryCount);

	baseEntryAddr = mpConfigManager->baseEntryStartAddr;

	for (i = 0; i < mpConfigTableHeader->entryCount; i++) {
		entryType = *(byte*)baseEntryAddr;

		k_printf("--------[Entry %d]----------------------------------------------------\n", i + 1);

		switch (entryType) {
		case MP_ENTRYTYPE_PROCESSOR:
			processorEntry = (ProcessorEntry*)baseEntryAddr;

			k_printf("- Entry Type         : Processor Entry\n");
			k_printf("- Local APIC ID      : %d\n", processorEntry->localApicId);
			k_printf("- Local APIC Version : 0x%X\n", processorEntry->localApicVersion);
			k_printf("- CPU Flags          : 0x%X ", processorEntry->cpuFlags);
			if (processorEntry->cpuFlags & MP_PROCESSOR_CPUFLAGS_ENABLE) {
				k_printf("(Enable, ");
			} else {
				k_printf("(Disable, ");
			}
			if (processorEntry->cpuFlags & MP_PROCESSOR_CPUFLAGS_BSP) {
				k_printf("BSP)\n");
			} else {
				k_printf("AP)\n");
			}
			k_printf("- CPU Signature      : 0x%X\n", processorEntry->cpuSignature);
			k_printf("- Feature Flags      : 0x%X\n", processorEntry->featureFlags);
			k_printf("- Reserved           : 0x%X\n", processorEntry->reserved);

			baseEntryAddr += sizeof(ProcessorEntry);
			break;

		case MP_ENTRYTYPE_BUS:
		    busEntry = (BusEntry*)baseEntryAddr;

		    k_printf("- Entry Type      : Bus Entry\n");
		    k_printf("- Bus ID          : %d\n", busEntry->busId);
		    k_memcpy(buffer, busEntry->busTypeStr, sizeof(busEntry->busTypeStr));
		    buffer[sizeof(busEntry->busTypeStr)] = '\0';
		    k_printf("- Bus Type String : %s\n", buffer);

			baseEntryAddr += sizeof(BusEntry);
			break;

		case MP_ENTRYTYPE_IOAPIC:
			ioApicEntry = (IoApicEntry*)baseEntryAddr;

			k_printf("- Entry Type            : IO APIC Entry\n");
			k_printf("- IO APIC ID            : %d\n", ioApicEntry->ioApicId);
			k_printf("- IO APIC Version       : 0x%X\n", ioApicEntry->ioApicVersion);
			k_printf("- IO APIC Flags         : 0x%X ", ioApicEntry->ioApicFlags);
			if (ioApicEntry->ioApicFlags & MP_IOAPIC_FLAGS_ENABLE) {
				k_printf("(Enable)\n");
			} else {
				k_printf("(Disable)\n");
			}
			k_printf("- Memory Map IO Address : 0x%X\n", ioApicEntry->memMapIoAddr);

			baseEntryAddr += sizeof(IoApicEntry);
			break;

		case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
			ioInterruptAssignEntry = (IoInterruptAssignEntry*)baseEntryAddr;

			k_printf("- EntryType                 : IO Interrupt Assignment Entry\n");
			k_printf("- Interrupt Type            : 0x%X ", ioInterruptAssignEntry->interruptType);
			k_printf("(%s)\n", interruptType[ioInterruptAssignEntry->interruptType]);
			k_printf("- Interrupt Flags           : 0x%X ", ioInterruptAssignEntry->interruptFlags);
			k_printf("(%s, %s)\n", interruptPolarity[ioInterruptAssignEntry->interruptFlags & 0x03]
								, interruptTrigger[(ioInterruptAssignEntry->interruptFlags >> 2) & 0x03]);
			k_printf("- Source BUS ID             : %d\n", ioInterruptAssignEntry->srcBusId);
			k_printf("- Source BUS IRQ            : %d\n", ioInterruptAssignEntry->srcBusIrq);
			k_printf("- Destination IO APIC ID    : %d\n", ioInterruptAssignEntry->destIoApicId);
			k_printf("- Destination IO APIC INTIN : %d\n", ioInterruptAssignEntry->destIoApicIntin);

			baseEntryAddr += sizeof(IoInterruptAssignEntry);
			break;

		case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
			localInterruptAssignEntry = (LocalInterruptAssignEntry*)baseEntryAddr;

			k_printf("- Entry Type                    : Local Interrupt Assignment Entry\n");
			k_printf("- Interrupt Type                : 0x%X ", localInterruptAssignEntry->interruptType);
			k_printf("(%s)\n", interruptType[localInterruptAssignEntry->interruptType]);
			k_printf("- Interrupt Flags               : 0x%X ", localInterruptAssignEntry->interruptFlags);
			k_printf("(%s, %s)\n", interruptPolarity[localInterruptAssignEntry->interruptFlags & 0x03]
								, interruptTrigger[(localInterruptAssignEntry->interruptFlags >> 2) & 0x03]);
			k_printf("- Source BUS ID                 : %d\n", localInterruptAssignEntry->srcBusId);
			k_printf("- Source BUS IRQ                : %d\n", localInterruptAssignEntry->srcBusIrq);
			k_printf("- Destination Local APIC ID     : %d\n", localInterruptAssignEntry->destLocalApicId);
			k_printf("- Destination Local APIC LINTIN : %d\n", localInterruptAssignEntry->destLocalApicLintin);

			baseEntryAddr += sizeof(LocalInterruptAssignEntry);
			break;

		default:
			k_printf("Unknown Entry Type (%d)\n", entryType);
			break;
		}

		// ask a user to print more entries, every after 3 entries are printed.
		if ((i != 0) && (((i + 1) % 3) == 0)) {
			k_printf("Press any key to continue...('q' is exit):");
			if (k_getch() == 'q') {
				k_printf("\n");
				return;
			}
			k_printf("\n");
		}
	}

	k_printf("--------[Entry End]---------------------------------------------------\n");
}

int k_getProcessorCount(void) {
	// If MP configuration table dosen't exist, return 1 as processor count.
	if (g_mpConfigManager.processorCount == 0) {
		return 1;
	}

	return g_mpConfigManager.processorCount;
}
