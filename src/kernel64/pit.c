#include "pit.h"
#include "asm_util.h"

void k_initPit(word count, bool periodic) {
	
	// send Once Command to Control Register.
	k_outPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);
	
	if (periodic == true) {
		// send Periodic Command to Control Register.
		k_outPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
	}
	
	// write initialization value to Counter-0 ordered by LSB to MSB
	k_outPortByte(PIT_PORT_COUNTER0, count);
	k_outPortByte(PIT_PORT_COUNTER0, count >> 8);
}

word k_readCounter0(void) {
	byte highByte, lowByte;
	word temp = 0;

	// send Latch Command to Control Register in order to read current value of Counter-0.
	k_outPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

	// read current value from Counter-0 ordered by LSB to MSB.
	lowByte = k_inPortByte(PIT_PORT_COUNTER0);
	highByte = k_inPortByte(PIT_PORT_COUNTER0);

	temp = highByte;
	temp = (temp << 8) | lowByte;

	return temp;
}

/**
  wait for a while in setting Counter-0 directly.
  - If this function is called, PIT Controller changes, so it has to re-set PIT Controller later.
  - It's recommended to disable interrupt before calling this function in order to guarantee accuracy.
  - can measure up to 54.93 milliseconds
 */
void k_waitUsingDirectPit(word count) {
	word lastCounter0;
	word currentCounter0;

	// set PIT Controller in oder to repeat counting from 0 to 0xFFFF.
	k_initPit(0, true);

	// wait until it exceeds count.
	lastCounter0 = k_readCounter0();
	while (true) {
		currentCounter0 = k_readCounter0();
		if(((currentCounter0 - lastCounter0) & 0xFFFF) >= count){
			break;
		}
	}
}
