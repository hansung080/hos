#ifndef __WIDGETS_CLOCK_H__
#define __WIDGETS_CLOCK_H__

#include "../types.h"
#include "../fonts.h"
#include "../2d_graphics.h"
#include "../list.h"

// clock size
#define CLOCK_MAXWIDTH (FONT_DEFAULT_WIDTH * 11)
#define CLOCK_HEIGHT   FONT_DEFAULT_HEIGHT

// clock format
#define CLOCK_FORMAT_H    1 // hh
#define CLOCK_FORMAT_HA   2 // hh AM
#define CLOCK_FORMAT_HM   3 // hh:mm
#define CLOCK_FORMAT_HMA  4 // hh:mm AM
#define CLOCK_FORMAT_HMS  5 // hh:mm:ss
#define CLOCK_FORMAT_HMSA 6 // hh:mm:ss AM

#pragma pack(push, 1)

typedef struct __Clock {
	ListLink link;
	qword windowId;
	Rect area; // window coordinates 
	Color textColor;
	Color backgroundColor;
	byte format;
	char formatStr[12];
} Clock;

#pragma pack(pop)

#endif // __WIDGETS_CLOCK_H__