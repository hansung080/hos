#include "hdd.h"
#include "asm_util.h"
#include "../utils/util.h"
#include "console.h"

static HddManager g_hddManager;

bool k_initHdd(void) {
	
	// initialize mutex.
	k_initMutex(&(g_hddManager.mutex));
	
	// initialize interrupt flag.
	g_hddManager.primaryInterruptOccurred = false;
	g_hddManager.secondaryInterruptOccurred = false;
	
	// set 0 to Digital Output Resister in order to enable interrupt.
	k_outPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);
	k_outPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);
	
	// read hard disk info.
	if (k_readHddInfo(true, true, &(g_hddManager.hddInfo)) == false) {
		g_hddManager.hddDetected = false;
		g_hddManager.writable = false;
		return false;
	}
	
	// If hard disk is detected, can write to hard disk only on QEMU.
	// It must be careful to write to real hard disk, because important data might be lost in a mistake.
	g_hddManager.hddDetected = true;
	if (k_memcmp(g_hddManager.hddInfo.modelNumber, "QEMU", 4) == 0) {
		g_hddManager.writable = true;
		
	} else {
		g_hddManager.writable = false;
	}
	
	return true;
}

static byte k_readHddStatus(bool primary) {
	if (primary == true) {
		// read Status Register of first PATA port.
		return k_inPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_STATUS);
	}
	
	// read Status Register of second PATA port.
	return k_inPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_STATUS);
}

static bool k_waitHddNoBusy(bool primary) {
	qword startTickCount;
	byte status;
	
	startTickCount = k_getTickCount();
	
	// wait until hard disk status becomes [No Busy], or limit time expires
	while ((k_getTickCount() - startTickCount) <= HDD_WAITTIME) {
		status = k_readHddStatus(primary);
		
		// If BSY(bit 7) of Status Register == 0, return true.
		if ((status & HDD_STATUS_BUSY) != HDD_STATUS_BUSY) {
			return true;
		}
		
		k_sleep(1);
	}
	
	return false;
}

static bool k_waitHddReady(bool primary) {
	qword startTickCount;
	byte status;
	
	startTickCount = k_getTickCount();
	
	// wait until hard disk status becomes [Device Ready], or limit time expires
	while ((k_getTickCount() - startTickCount) <= HDD_WAITTIME) {
		status = k_readHddStatus(primary);
		
		// If DRDY(bit 6) of Status Register == 1, return true.
		if ((status & HDD_STATUS_READY) == HDD_STATUS_READY) {
			return true;
		}
		
		k_sleep(1);
	}
	
	return false;
}

void k_setHddInterruptFlag(bool primary, bool flag) {
	if (primary == true) {
		g_hddManager.primaryInterruptOccurred = flag;
		
	} else {
		g_hddManager.secondaryInterruptOccurred = flag;
	}
}

static bool k_waitHddInterrupt(bool primary) {
	qword startTickCount;
	
	startTickCount = k_getTickCount();
	
	// wait until interrupt occurs, or limit time expires
	while ((k_getTickCount() - startTickCount) <= HDD_WAITTIME) {
		// If first interrupt occurs, return true.
		if ((primary == true) && (g_hddManager.primaryInterruptOccurred == true)) {
			return true;
			
		// If second interrupt occurs, return true.
		} else if ((primary == false) && (g_hddManager.secondaryInterruptOccurred == true)) {
			return true;
		}
	}
	
	return false;
}

bool k_readHddInfo(bool primary, bool master, HddInfo* hddInfo) {
	word portBase;
	byte status;
	byte driveFlag;
	int i;
	bool waitResult;
	
	if (primary == true) {
		portBase = HDD_PORT_PRIMARYBASE;
		
	} else {
		portBase = HDD_PORT_SECONDARYBASE;
	}
	
	k_lock(&(g_hddManager.mutex));
	
	// wait for a wile If hard disk already has executing commands.
	if (k_waitHddNoBusy(primary) == false) {
		k_unlock(&(g_hddManager.mutex));
		return false;
	}
	
	//----------------------------------------------------------------------------------------------------
	// send configuration value (LBA mode, drive number) to Drive/Head Register.
	//----------------------------------------------------------------------------------------------------
	
	if (master) {
		driveFlag = HDD_DRIVEANDHEAD_LBA;
		
	} else {
		driveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}
	
	// send configuration value to Drive/Head Register.
	k_outPortByte(portBase + HDD_PORT_INDEX_DRIVEANDHEAD, driveFlag);
	
	//----------------------------------------------------------------------------------------------------
	// send drive recognition command, and wait for interrupt.
	//----------------------------------------------------------------------------------------------------
	
	// wait until hard disk is ready to receive commands, or limit time expires
	if (k_waitHddReady(primary) == false) {
		k_unlock(&(g_hddManager.mutex));
		return false;
	}
	
	k_setHddInterruptFlag(primary, false);
	
	// send drive recognition command to Command Register.
	k_outPortByte(portBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_IDENTIFY);
	
	// wait until command processing finishes and interrupt occurs, or limit time expires
	waitResult = k_waitHddInterrupt(primary);
	
	// If interrupt dosen't occur or error occurs in processing, return
	status = k_readHddStatus(primary);
	if ((waitResult == false) || ((status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR)) {
		k_unlock(&(g_hddManager.mutex));
		return false;
	}
	
	//----------------------------------------------------------------------------------------------------
	// receive data
	//----------------------------------------------------------------------------------------------------
	
	// read 1 sector-sized data from Data Register.
	for (i = 0; i < (512 / 2); i++) {
		((word*)hddInfo)[i] = k_inPortWord(portBase + HDD_PORT_INDEX_DATA);
	}
	
	// change byte order of string.
	k_swapByteInWord(hddInfo->modelNumber, sizeof(hddInfo->modelNumber) / 2);
	k_swapByteInWord(hddInfo->serialNumber, sizeof(hddInfo->serialNumber) / 2);
	
	k_unlock(&(g_hddManager.mutex));
	return true;
}

static void k_swapByteInWord(word* data, int wordCount) {
	int i;
	word temp;
	
	for (i = 0; i < wordCount; i++) {
		temp = data[i];
		data[i] = (temp >> 8) | (temp << 8);
	}
}

int k_readHddSector(bool primary, bool master, dword lba, int sectorCount, char* buffer) {
	word portBase;
	int i, j;
	byte driveFlag;
	byte status;
	long readCount = 0;
	bool waitResult;
	
	// check the range of reading sector count (1 ~ 256 sectors).
	if ((g_hddManager.hddDetected == false) || (sectorCount <= 0) || (sectorCount > 256) || ((lba + sectorCount) >= g_hddManager.hddInfo.totalSectors)) {
		return 0;
	}
	
	if (primary == true) {
		portBase = HDD_PORT_PRIMARYBASE;
		
	} else {
		portBase = HDD_PORT_SECONDARYBASE;
	}
	
	k_lock(&(g_hddManager.mutex));
	
	// wait until hard disk already has executing commands, or limit time expires
	if (k_waitHddNoBusy(primary) == false) {
		k_unlock(&(g_hddManager.mutex));
		return false;
	}
	
	//----------------------------------------------------------------------------------------------------
	// send reading sector count, sector offset to many registers,
	// and send configuration value (LBA mode, drive number) to Drive/Head Register.
	// save [bit 0~7: sector number], [bit 8~15: cylinder number LSB], [bit 16~23: cylinder number MSB], [bit 24~27: head number] of LBA address (28 bits).
	//----------------------------------------------------------------------------------------------------
	
	// send sector count to Sector Count Register.
	k_outPortByte(portBase + HDD_PORT_INDEX_SECTORCOUNT, sectorCount);
	
	// send sector offset (LBA bit 0~7) to Sector Number Register.
	k_outPortByte(portBase + HDD_PORT_INDEX_SECTORNUMBER, lba);
	
	// send sector offset (LBA bit 8~15) to Cylinder LSB Register.
	k_outPortByte(portBase + HDD_PORT_INDEX_CYLINDERLSB, lba >> 8);
	
	// send sector offset (LBA bit 16~23) to Cylinder MSB Register.
	k_outPortByte(portBase + HDD_PORT_INDEX_CYLINDERMSB, lba >> 16);
	
	if (master == true) {
		driveFlag = HDD_DRIVEANDHEAD_LBA;
		
	} else {
		driveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}
	
	// send sector offset (LBA bit 24~27) and configuration value (LBA mode, drive number) to Drive/Head Register.
	k_outPortByte(portBase + HDD_PORT_INDEX_DRIVEANDHEAD, driveFlag | ((lba >> 24) & 0x0F));
	
	//----------------------------------------------------------------------------------------------------
	// send Sector Read Command.
	//----------------------------------------------------------------------------------------------------
	
	// wait until hard disk is ready to receive commands, or limit time expires
	if (k_waitHddReady(primary) == false) {
		k_unlock(&(g_hddManager.mutex));
		return false;
	}
	
	k_setHddInterruptFlag(primary, false);
	
	// send Sector Read Command to Command Register.
	k_outPortByte(portBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_READ);
	
	//----------------------------------------------------------------------------------------------------
	// after waiting for interrupts, receive data.
	//----------------------------------------------------------------------------------------------------
	
	// read data as many as sector count from Data Register.
	for (i = 0; i < sectorCount; i++) {
		// If error occurs in processing, return.
		status = k_readHddStatus(primary);
		if ((status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
			k_printf("HDD error: Error has occured while reading HDD sector.\n");
			k_unlock(&(g_hddManager.mutex));
			return i; // return real read sector count.
		}
		
		// wait until it finishes receiving data, or limit time expires
		if ((status & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST) {
			
			// wait until it finishes receiving data and interrupt occurs, or limit time expires
			waitResult = k_waitHddInterrupt(primary);
			
			k_setHddInterruptFlag(primary, false);
			
			// If interrupt dosen't occurs, return.
			if (waitResult == false) {
				k_printf("HDD error: Interrupt has not occured while reading HDD sector.\n");
				k_unlock(&(g_hddManager.mutex));
				return false;
			}
		}
		
		// read data as many as 1 sector from Data Register.
		for (j = 0; j < (512 / 2); j++) {
			((word*)buffer)[readCount++] = k_inPortWord(portBase + HDD_PORT_INDEX_DATA);
		}
	}
	
	k_unlock(&(g_hddManager.mutex));
	return i; // return real read sector count.
}

int k_writeHddSector(bool primary, bool master, dword lba, int sectorCount, char* buffer) {
	word portBase;
	int i, j;
	byte driveFlag;
	byte status;
	long writeCount = 0;
	bool waitResult;
	
	// check the range of writing sector count (1~256 sectors).
	if ((g_hddManager.writable == false) || (sectorCount <= 0) || (sectorCount > 256) || ((lba + sectorCount) >= g_hddManager.hddInfo.totalSectors)) {
		return 0;
	}
	
	if (primary == true) {
		portBase = HDD_PORT_PRIMARYBASE;
		
	} else {
		portBase = HDD_PORT_SECONDARYBASE;
	}
	
	// wait until hard disk already has executing commands, or limit time expires.
	if (k_waitHddNoBusy(primary) == false) {
		return false;
	}
	
	k_lock(&(g_hddManager.mutex));
	
	//----------------------------------------------------------------------------------------------------
	// send writing sector count, sector offset to many registers,
	// and send configuration value (LBA mode, drive number) to Drive/Head Register.
	// save [bit 0~7: sector number], [bit 8~15: cylinder number LSB], [bit 16~23: cylinder number MSB], [bit 24~27: head number] of LBA address (28 bits).
	//----------------------------------------------------------------------------------------------------
	
	// send sector count to Sector Count Register.
	k_outPortByte(portBase + HDD_PORT_INDEX_SECTORCOUNT, sectorCount);
	
	// send sector offset (LBA bit 0~7) to Sector Number Register.
	k_outPortByte(portBase + HDD_PORT_INDEX_SECTORNUMBER, lba);
	
	// send sector offset (LBA bit 8~15) to Cylinder LSB Register.
	k_outPortByte(portBase + HDD_PORT_INDEX_CYLINDERLSB, lba >> 8);
	
	// send sector offset (LBA bit 16~23) to Cylinder MSB Register.
	k_outPortByte(portBase + HDD_PORT_INDEX_CYLINDERMSB, lba >> 16);
	
	if (master == true) {
		driveFlag = HDD_DRIVEANDHEAD_LBA;
		
	} else {
		driveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}
	
	// send sector count (LBA bit 24~27) and configuration value (LBA mode, drive number) to Drive/Head Register.
	k_outPortByte(portBase + HDD_PORT_INDEX_DRIVEANDHEAD, driveFlag | ((lba >> 24) & 0x0F));
	
	//----------------------------------------------------------------------------------------------------
	// send Sector Write Command, and wait until data is ready to receive.
	//----------------------------------------------------------------------------------------------------
	
	// wait until hard disk is ready to receive commands, or limit time expires.
	if (k_waitHddReady(primary) == false) {
		k_unlock(&(g_hddManager.mutex));
		return false;
	}
	
	// send Sector Write Command to Command Register.
	k_outPortByte(portBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_WRITE);
	
	// wait until data is ready to receive.
	while (true) {
		status = k_readHddStatus(primary);
		
		// If error occurs, return.
		if ((status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
			k_unlock(&(g_hddManager.mutex));
			return 0;
		}
		
		// <DRQ(bit 3) of Status Register == 1> means that it's ready to send data.
		if ((status & HDD_STATUS_DATAREQUEST) == HDD_STATUS_DATAREQUEST) {
			break;
		}
		
		k_sleep(1);
	}
	
	//----------------------------------------------------------------------------------------------------
	// send data, and wait for interrupts.
	//----------------------------------------------------------------------------------------------------
	
	// write data as many as sector count to Data Register.
	for (i = 0; i < sectorCount; i++) {
		
		// initialize interrupt flag, write data as many as 1 sector to Data Register.
		k_setHddInterruptFlag(primary, false);
		for (j = 0; j < (512 / 2); j++) {
			k_outPortWord(portBase + HDD_PORT_INDEX_DATA, ((word*)buffer)[writeCount++]);
		}
		
		// If error occurs in processing, return.
		status = k_readHddStatus(primary);
		if ((status & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
			k_printf("HDD error: Error has occured while writing HDD sector.\n");
			k_unlock(&(g_hddManager.mutex));
			return i; // return real written sector count.
		}
		
		// wait until it finishes sending data, or limit time expires
		if ((status & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST) {
			
			// wait until it finishes sending data and interrupt occurs, or limit time expires
			waitResult = k_waitHddInterrupt(primary);
			
			k_setHddInterruptFlag(primary, false);
			
			// If interrupt dosen't occur, return.
			if (waitResult == false) {
				k_printf("HDD error: Interrupt has not occured while writing HDD sector.\n");
				k_unlock(&(g_hddManager.mutex));
				return false;
			}
		}
	}
	
	k_unlock(&(g_hddManager.mutex));
	return i; // return real written sector count.
}

static bool k_isHddBusy(bool primary) {
	byte status;
	
	status = k_readHddStatus(primary);
	
	if ((status & HDD_STATUS_BUSY) == HDD_STATUS_BUSY) {
		return true;
	}
	
	return false;
}

static bool k_isHddReady(bool primary) {
	byte status;
	
	status = k_readHddStatus(primary);
	
	if ((status & HDD_STATUS_READY) == HDD_STATUS_READY) {
		return true;
	}
	
	return false;
}
