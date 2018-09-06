#include "vbe.h"

static VbeModeInfoBlock* g_vbeModeInfoBlock = (VbeModeInfoBlock*)VBE_MODEINFOBLOCKADDRESS;

inline VbeModeInfoBlock* k_getVbeModeInfoBlock(void) {
	return g_vbeModeInfoBlock;
}