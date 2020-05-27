#ifndef __CORE_VBE_H__
#define __CORE_VBE_H__

#include "types.h"

#define VBE_MODEINFOBLOCKADDRESS   0x7E00 // VBE mode info block address: It's right after boot-loader address <0x7C00>.
#define VBE_GRAPHICMODEFLAGADDRESS 0x7C0A // GRAPHIC_MODE_FLAG is defined in boot_loader.asm.

// graphic mode flag
#define VBE_GRAPHICMODEFLAG             *(byte*)VBE_GRAPHICMODEFLAGADDRESS // graphic mode flag (0: text mode, 1: graphic mode)
#define VBE_GRAPHICMODEFLAG_TEXTMODE    0x00 // text mode
#define VBE_GRAPHICMODEFLAG_GRAPHICMODE 0x01 // graphic mode

#pragma pack(push, 1)

// VBE mode info block size must be 256 bytes. (It's the fixed size.)
// hOS uses VBE mode 0x117 (resolution: 1024 * 768 pixels, color count: 16 bits (64K) color, R:G:B=5:6:5)
typedef struct k_VbeModeInfoBlock {
	//--------------------------------------------------
	// Common Fields in All VBE Versions
	//--------------------------------------------------
	word modeAttr;         // mode attribute
	byte winAAttr;         // window A attribute
	byte winBAttr;         // window B attribute
	word winGranularity;   // window granularity -> 0x10
	word winSize;          // window size
	word winASegment;      // window A starting segment address
	word winBSegment;      // window B starting segment address
	dword winFuncAddr;     // window function address (for real mode)
	word bytesPerScanLine; // byte count per a screen scan line
	
	//--------------------------------------------------
	// Common Fields in VEB Version 1.2 or Higher
	//--------------------------------------------------
	word xResolution;        // X-axis pixel count or character count of screen -> 1024 pixels
	word yResolution;        // Y-axis pixel count or character count of screen -> 768 pixels
	byte xCharSize;          // X-axis pixel count of a character
	byte yCharSize;          // Y-axis pixel count of a character
	byte numberOfPlane;      // memory plane count
	byte bitsPerPixel;       // bit count per a pixel -> 16 bits
	byte numberOfBanks;      // bank count
	byte memoryModel;        // video memory model
	byte bankSize;           // bank size (KB)
	byte numberOfImagePages; // image page count
	byte reserved1;          // reserved for paging
	
	/* Direct Color-related Field */
	byte redMaskSize;         // red-occuping size in a color (bit)       -> 5 bits
	byte redFieldPos;         // red-starting position in a color (bit)   -> bit 11
	byte greenMaskSize;       // green-occuping size in a color (bit)     -> 6 bits
	byte greenFieldPos;       // green-starting position in a color (bit) -> bit 5
	byte blueMaskSize;        // blue-occuping size in a color (bit)      -> 5 bits
	byte blueFieldPos;        // blue-starting position in a color (bit)  -> bit 0
	byte reservedMaskSize;    // reserved-occuping size in a color (bit)
	byte reservedFieldPos;    // reserved-starting position in a color (bit)
	byte directColorModeInfo; // direct color mode info
	
	//--------------------------------------------------
	// Common Fields in VEB Version 2.0 or Higher
	//--------------------------------------------------
	dword physicalBaseAddr; // video memory address in graphic mode (linear frame buffer address) -> 0xF0000000 bytes
	dword reserved2;        // reserved field
	dword reserved3;        // reserved field
	
	//--------------------------------------------------
	// Common Fields in VEB Version 3.0 or Higher
	//--------------------------------------------------
	word linearBytesPerScanLine;   // byte count per a screen scan line in linear frame buffer mode
	byte bankNumberOfImagePages;   // image page count in bank mode
	byte linearNumberOfImagePages; // image page count in linear frame buffer mode
	
	/* Direct Color-related Field in Linear Frame Buffer Mode */
	byte linearRedMaskSize;      // red-occuping size in a color (bit)
	byte linearRedFieldPos;      // red-starting position in a color (bit)
	byte linearGreenMaskSize;    // green-occuping size in a color (bit)
	byte linearGreenFieldPos;    // green-starting position in a color (bit)
	byte linearBlueMaskSize;     // blue-occuping size in a color (bit)
	byte linearBlueFieldPos;     // blue-starting position in a color (bit)
	byte linearReservedMaskSize; // reserved-occuping size in a color (bit)
	byte linearReservedFieldPos; // reserved-starting position in a color (bit)
	dword maxPixelClock;         // max value of pixel clock (Hz)
	
	byte reserved4[188]; // reserved area: align this structure size with 256 bytes.
} VbeModeInfoBlock;

#pragma pack(pop)

VbeModeInfoBlock* k_getVbeModeInfoBlock(void);

#endif // __CORE_VBE_H__