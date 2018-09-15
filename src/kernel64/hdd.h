#ifndef __HDD_H__
#define __HDD_H__

#include "types.h"
#include "sync.h"

// I/O port base value of Hard Disk Controller
#define HDD_PORT_PRIMARYBASE   0x1F0 // first PATA port base value
#define HDD_PORT_SECONDARYBASE 0x170 // second PATA port base value

// I/O port index of Hard Disk Controller
#define HDD_PORT_INDEX_DATA          0x00  // Data Register (0x1F0, 0x170): Read/Write, 2 bytes-sized, save data which is sent/received to/from hard disk.
#define HDD_PORT_INDEX_SECTORCOUNT   0x02  // Sector Count Register (0x1F2, 0x172): Read/Write, 1 byte-sized, save sector count. (range 1~256 sectors, 0 means 256)
#define HDD_PORT_INDEX_SECTORNUMBER  0x03  // Sector Number Register (0x1F3, 0x173): Read/Write, 1 byte-sized, save sector number.
#define HDD_PORT_INDEX_CYLINDERLSB   0x04  // Cylinder LSB Register (0x1F4, 0x174): Read/Write, 1 byte-sized, save low 8 bits of cylinder number.
#define HDD_PORT_INDEX_CYLINDERMSB   0x05  // Cylinder MSB Register (0x1F5, 0x175): Read/Write, 1 byte-sized, save high 8 bits of cylinder number.
#define HDD_PORT_INDEX_DRIVEANDHEAD  0x06  // Drive/Head Register (0x1F6, 0x176): Read/Write, 1 byte-sized, save drive number and head number.
#define HDD_PORT_INDEX_STATUS        0x07  // Status Register (0x1F7, 0x177): Read, 1 byte-sized, save status of hard disk.
#define HDD_PORT_INDEX_COMMAND       0x07  // Command Register (0x1F7, 0x177): Write, 1 byte-sized, save command to send to hard disk.
#define HDD_PORT_INDEX_DIGITALOUTPUT 0x206 // Digital Output Register (0x3F6, 0x376): Read/Write, 1 byte-sized, process interrupt enable and hard disk reset.

// commands of Command Register (8 bits)
#define HDD_COMMAND_READ     0x20 // read sectors: The required registers to read sectors are Sector Count Register, Sector Number Register, Cylinder LSB/MSB Register, and Drive/Head Register.
#define HDD_COMMAND_WRITE    0x30 // write sectors: The required registers to write sectors are Sector Count Register, Sector Number Register, Cylinder LSB/MSB Register, and Drive/Head Register.
#define HDD_COMMAND_IDENTIFY 0xEC // recognize drive (read hard disk info): The required registers to recognize drive are Drive/Head Register.

// fields of Status Register (8 bits)
#define HDD_STATUS_ERROR         0x01 // ERR(bit 0): Error, mean that error has occurred in previous command.
#define HDD_STATUS_INDEX         0x02 // IDX(bit 1): Index, mean that index mark of disk has been detected.
#define HDD_STATUS_CORRECTEDDATA 0x04 // CORR(bit 2): Correctable Data Error, mean that error has been corrected as ECC info.
#define HDD_STATUS_DATAREQUEST   0x08 // DRQ(bit 3): Data Request, mean that hard disk is ready to send/receive data.
#define HDD_STATUS_SEEKCOMPLETE  0x10 // DSC(bit 4): Device Seek Complete, mean that the head of a access arm has been move to the requested position.
#define HDD_STATUS_WRITEFAULT    0x20 // DF(bit 5): Device Fault, mean that problem has occurred in working.
#define HDD_STATUS_READY         0x40 // DRDY(bit 6): Device Ready, mean that hard disk is ready to receive a command.
#define HDD_STATUS_BUSY          0x80 // BSY(bit 7): Busy, mean that hard disk is executing a command.

/**
  fields of Drive/Head Register (8 bits)
  - LBA mode (bit 6)=0: CHS mode: save sector count to Sector Count Register, sector number to Sector Number Register, cylinder number to Cylinder LSB/MSB Register, and head number to head number field of Drive/Head Register.
  - LBA mode (bit 6)=1: LBA mode: save sector count to Sector Count Register, and other registers are integrated into LBA address.
                        LBA address (28 bits): [bit 0~7: Sector Number Register], [bit 8~15: Cylinder LSB Register], [bit 16~23: Cylinder MSB Register], [bit 24~27: head number field of Drive/Head Register]
  - drive number (bit 4)=0: send/receive to/from master hard disk.
  - drive number (bit 4)=1: send/receive to/from slave hard disk.
*/
#define HDD_DRIVEANDHEAD_LBA   0xE0 // 1110 0000 : fixed(bit 7)=1, LBA mode(bit 6)=1, fixed(bit 5)=1, drive number(bit 4)=0, head number(bit 3~0)=0000
#define HDD_DRIVEANDHEAD_SLAVE 0x10 // 0001 0000 : drive number(bit 4)=1

// waiting time for responses from hard disk (millisecond).
#define HDD_WAITTIME 500

// max sector count to read/write from/to hard disk at once.
#define HDD_MAXBULKSECTORCOUNT 256

#pragma pack(push, 1)

typedef struct k_HddInfo {
	// configuration value
	word config;
	
	// cylinder count (used in CHS mode)
	word numberOfCylinder;
	word reserved1;
	
	// head count (used in CHS mode)
	word numberOfHead;
	word unformattedBytesPerTrack;
	word unformattedBytesPerSector;
	
	// sector count per cylinder (used in CHS mode)
	word numberOfSectorPerCylinder;
	word interSectorGap;
	word bytesInPhaseLock;
	word numberOfVendorUniqueStatusWord;
	
	// serial number
	word serialNumber[10];
	word controllerType;
	word bufferSize;
	word numberOfEccBytes;
	word firmwareRevision[4];
	
	// model number
	word modelNumber[20];
	word reserved2[13];
	
	// total sector count (used in LBA mode)
	dword totalSectors;
	word reserved3[196];
} HddInfo;

typedef struct k_HddManager {
	bool hddDetected;                         // hard disk detected flag
	bool writable;                            // writable flag: can write to hard disk only on QEMU
	volatile bool primaryInterruptOccurred;   // first interrupt flag
	volatile bool secondaryInterruptOccurred; // second interrupt flag
	Mutex mutex;                              // mutex: synchronization object
	HddInfo hddInfo;                          // hard disk info.
} HddManager;

#pragma pack(pop)

bool k_initHdd(void);
bool k_readHddInfo(bool primary, bool master, HddInfo* hddInfo);
int k_readHddSector(bool primary, bool master, dword lba, int sectorCount, char* buffer);
int k_writeHddSector(bool primary, bool master, dword lba, int sectorCount, char* buffer);
void k_setHddInterruptFlag(bool primary, bool flag);
static void k_swapByteInWord(word* data, int wordCount);
static byte k_readHddStatus(bool primary);
static bool k_isHddBusy(bool primary);  // [NOTE] implemented by hs.kwon.
static bool k_isHddReady(bool primary); // [NOTE] implemented by hs.kwon.
static bool k_waitHddNoBusy(bool primary);
static bool k_waitHddReady(bool primary);
static bool k_waitHddInterrupt(bool primary);

#endif // __HDD_H__
