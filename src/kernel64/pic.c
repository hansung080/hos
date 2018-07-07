#include "pic.h"
#include "asm_util.h"

void k_initPic(void) {
	/* initialize master PIC */
	// port 0x20, command ICW1: 0x11 -> 0001 0001 -> LTIM(bit 3)=0, SNGL(bit 1)=0, IC4(bit 0)=1
	k_outPortByte(PIC_MASTER_PORT1, 0x11);

	// port 0x21, command ICW2: 0x20 -> 0010 0000 -> start position of master interrupt vector in IDT (32)
	k_outPortByte(PIC_MASTER_PORT2, PIC_IRQSTARTVECTOR);

	// port 0x21, command ICW3: 0x04 -> 0000 0100 -> pin position of master PIC which is connect to slave PIC (use bit offset)
	k_outPortByte(PIC_MASTER_PORT2, 0x04);

	// port 0x21, command ICW4: 0x01 -> 0000 0001 -> SFNM(bit 4)=0, BUF(bit 3)=0, M/S(bit 2)=0, AEOI(bit 1)=0, uPM(bit 0)=1
	k_outPortByte(PIC_MASTER_PORT2, 0x01);

	/* initialize slave PIC */
	// port 0xA0, command ICW1: 0x11 -> 0001 0001 -> LTIM(bit 3)=0, SNGL(bit 1)=0, IC4(bit 0)=1
	k_outPortByte(PIC_SLAVE_PORT1, 0x11);

	// port 0xA1, command ICW2: 0x28 -> 0010 1000 -> start position of slave interrupt vector in IDT (40)
	k_outPortByte(PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR + 8);

	// port 0xA1, command ICW3: 0x02 -> 0000 0010 -> pin position of master PIC which is connect to slave PIC (use integer)
	k_outPortByte(PIC_SLAVE_PORT2, 0x02);

	// port 0xA1, command ICW4: 0x01 -> 0000 0001 -> SFNM(bit 4)=0, BUF(bit 3)=0, M/S(bit 2)=0, AEOI(bit 1)=0, uPM(bit 0)=1
	k_outPortByte(PIC_SLAVE_PORT2, 0x01);
}

void k_maskPicInterrupt(word irqBitmask) {
	/* mask master PIC */
	// port 0x21, command OCW1: 0x?? -> ???? ???? -> mask interrupt of the pin.
	k_outPortByte(PIC_MASTER_PORT2, (byte)irqBitmask);

	/* mask slave PIC */
	// port 0xA1, command OCW1: 0x?? -> ???? ???? -> mask interrupt of the pin.
	k_outPortByte(PIC_SLAVE_PORT2, (byte)(irqBitmask >> 8));
}

void k_sendEoiToPic(int irqNumber) {
	/**
	  If interrupt occurs from master PIC, send EOI to master PIC.
	  If interrupt occurs from slave PIC, send EOI to both master PIC and slave PIC.
	 */

	/* send EOI to master PIC */
	// port 0x20, command OCW2: 0x20 -> 0010 0000 -> R(bit 7)=0, SL(bit 6)=0, EOI(bit 5)=1, L2(bit 2)=0, L1(bit 1)=0, L0(bit 0)=0
	k_outPortByte(PIC_MASTER_PORT1, 0x20);

	/* send EOI to slave PIC */
	if (irqNumber >= 8) {
		// port 0xA0, command OCW2: 0x20 -> 0010 0000 -> R(bit 7)=0, SL(bit 6)=0, EOI(bit 5)=1, L2(bit 2)=0, L1(bit 1)=0, L0(bit 0)=0
		k_outPortByte(PIC_SLAVE_PORT1, 0x20);
	}
}

