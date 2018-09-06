#ifndef __TYPES_H__
#define __TYPES_H__

#define byte  unsigned char
#define word  unsigned short
#define dword unsigned int
#define qword unsigned long
#define bool  unsigned char

#define false 0
#define true  1

#define null 0

#pragma pack(push, 1)

// a character of screen
typedef struct k_Char {
	byte char_;
	byte attr;
} Char;

#pragma pack(pop)

#endif // __TYPES_H__
