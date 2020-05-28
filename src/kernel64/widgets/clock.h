#ifndef __WIDGETS_CLOCK_H__
#define __WIDGETS_CLOCK_H__

#include "../core/types.h"
#include "../core/2d_graphics.h"
#include "../fonts/fonts.h"
#include "../core/sync.h"
#include "../utils/list.h"

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

typedef struct k_Clock {
	ListLink link;
	qword windowId;
	Rect area; // window coordinates 
	Color textColor;
	Color backgroundColor;
	byte format;
	char formatStr[12];
} Clock;

typedef struct k_ClockManager {
	Mutex mutex;
	List clockList;
	byte prevHour;
	byte prevMinute;
	byte prevSecond;
} ClockManager;

#pragma pack(pop)

void k_initClockManager(void);
void k_setClock(Clock* clock, qword windowId, int x, int y, Color textColor, Color backgroundColor, byte format, bool reset);
void k_addClock(Clock* clock);
Clock* k_removeClock(qword clockId);
void k_drawAllClocks();
static void k_drawClock(Clock* clock);

#endif // __WIDGETS_CLOCK_H__