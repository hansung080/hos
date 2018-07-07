#include "local_apic.h"
#include "mp_config_table.h"

qword k_getLocalApicBaseAddr(void) {
	MpConfigTableHeader* mpHeader;

	mpHeader = k_getMpConfigManager()->mpConfigTableHeader;

	return mpHeader->memMapIoAddrOfLocalApic;
}

void k_enableSoftwareLocalApic(void) {
	qword localApicBaseAddr;

	localApicBaseAddr = k_getLocalApicBaseAddr();

	// set local APIC software enable/disable (bit 8) of Spurious Interrupt Vector Register (address 0xFEE000F0, 32 bits-sized) to [1:local APIC enable].
	*(dword*)(localApicBaseAddr + APIC_REGISTER_SVR) |= APIC_SOFTWARE_ENABLE;
}
