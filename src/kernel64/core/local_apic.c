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
	*(dword*)(localApicBaseAddr + LAPIC_REGISTER_SVR) |= LAPIC_SOFTWARE_ENABLE;
}

void k_sendEoiToLocalApic(void) {
	qword localApicBaseAddr;
	
	localApicBaseAddr = k_getLocalApicBaseAddr();
	
	// set 0x00 (EOI) to EOI Register.
	*(dword*)(localApicBaseAddr + LAPIC_REGISTER_EOI) = 0x00;
}

void k_setInterruptPriority(byte priority) {
	qword localApicBaseAddr;
	
	localApicBaseAddr = k_getLocalApicBaseAddr();
	
	/**
	  set priority to Task Priority Register
	  - interrupt priority ranges 0 ~ 15 which is interrupt vector 0 ~ 255 divided by 16.
	  - The bigger number means the higher priority.
	  - set 0 priority to Task Priority Register in order to accept all priorities of interrupts.
	*/
	*(dword*)(localApicBaseAddr + LAPIC_REGISTER_TASKPRIORITY) = priority;
}

void k_initLocalVectorTable(void) {
	qword localApicBaseAddr;
	
	localApicBaseAddr = k_getLocalApicBaseAddr();
	
	// set mask interrupt to LVT Timer Register.
	*(dword*)(localApicBaseAddr + LAPIC_REGISTER_TIMER) |= LAPIC_INTERRUPT_MASK;
	
	// set mask interrupt to LVT Thermal Sensor Register.
	*(dword*)(localApicBaseAddr + LAPIC_REGISTER_THERMALSENSOR) |= LAPIC_INTERRUPT_MASK;
	
	// set mask interrupt to LVT Performance Monitering Counter Register.
	*(dword*)(localApicBaseAddr + LAPIC_REGISTER_PERFORMANCEMONITERINGCOUNTER) |= LAPIC_INTERRUPT_MASK;
	
	// set mask interrupt to LVT LINT0 Register.
	*(dword*)(localApicBaseAddr + LAPIC_REGISTER_LINT0) |= LAPIC_INTERRUPT_MASK;
	
	// set NMI to LVT LINT1 Register.
	*(dword*)(localApicBaseAddr + LAPIC_REGISTER_LINT1) = LAPIC_TRIGGERMODE_EDGE | LAPIC_POLARITY_ACTIVEHIGH | LAPIC_DELIVERYMODE_NMI;
	
	// set mask interrupt to LVT Error Register.
	*(dword*)(localApicBaseAddr + LAPIC_REGISTER_ERROR) |= LAPIC_INTERRUPT_MASK;
}

