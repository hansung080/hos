#ifndef __TYPES_H__
#define __TYPES_H__

/**
  [NOTE] __CORE_TYPES_H__ has been renamed to __TYPES_H__,
         in order to avoid redefinition and conflicting types compile errors
         occurred in src/kernel32/page.h.
*/

#define byte  unsigned char
#define word  unsigned short
#define dword unsigned int
#define qword unsigned long
#define bool  unsigned char

#define false 0
#define true  1

#define null 0

// debug mode
#define __DEBUG__ 1 // 0: release mode, 1: debug mode

// system mode
#define SYSMODE_SYSTEM     0 // system
#define SYSMODE_QEMUOLD    1 // QEMU older versions than 2.0
#define SYSMODE_QEMUNEW    2 // QEMU newer versions than 2.0
#define SYSMODE_VIRTUALBOX 3 // VirtualBox
#define SYSMODE SYSMODE_QEMUNEW

// Original offsetof macro function is defined in stddef.h.
// But, the same macro function is defined here in order to avoid type duplication.
#define offsetof(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)

#pragma pack(push, 1)

// a character of screen
typedef struct k_Char {
	byte char_;
	byte attr;
} Char;

#pragma pack(pop)

#endif // __TYPES_H__