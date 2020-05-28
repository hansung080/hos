#include "clock.h"
#include "../core/window.h"
#include "../utils/util.h"
#include "../core/rtc.h"
#include "../utils/kid.h"

static ClockManager g_clockManager;

void k_initClockManager(void) {
	k_initMutex(&g_clockManager.mutex);
	k_initList(&g_clockManager.clockList);
	g_clockManager.prevHour = 0;
	g_clockManager.prevMinute = 0;
	g_clockManager.prevSecond = 0;
}

void k_setClock(Clock* clock, qword windowId, int x, int y, Color textColor, Color backgroundColor, byte format, bool reset) {
	Rect clockArea;

	if ((reset == true) && (clock->link.id != KID_INVALID)) {
		clock->link.id = k_allocKid();	
	}	
	
	clock->windowId = windowId;
	clock->textColor = textColor;
	clock->backgroundColor = backgroundColor;
	clock->format = format;

	switch (format) {
	case CLOCK_FORMAT_H:
		k_strcpy(clock->formatStr, "00");
		k_setRect(&clockArea, x, y, x + FONT_DEFAULT_WIDTH * 2, y + FONT_DEFAULT_HEIGHT);
		k_memcpy(&clock->area, &clockArea, sizeof(Rect));
		break;

	case CLOCK_FORMAT_HA:
		k_strcpy(clock->formatStr, "00 AM");
		k_setRect(&clockArea, x, y, x + FONT_DEFAULT_WIDTH * 5, y + FONT_DEFAULT_HEIGHT);
		k_memcpy(&clock->area, &clockArea, sizeof(Rect));
		break;

	case CLOCK_FORMAT_HM:
		k_strcpy(clock->formatStr, "00:00");
		k_setRect(&clockArea, x, y, x + FONT_DEFAULT_WIDTH * 5, y + FONT_DEFAULT_HEIGHT);
		k_memcpy(&clock->area, &clockArea, sizeof(Rect));
		break;

	case CLOCK_FORMAT_HMA:
		k_strcpy(clock->formatStr, "00:00 AM");
		k_setRect(&clockArea, x, y, x + FONT_DEFAULT_WIDTH * 8, y + FONT_DEFAULT_HEIGHT);
		k_memcpy(&clock->area, &clockArea, sizeof(Rect));
		break;

	case CLOCK_FORMAT_HMS:
		k_strcpy(clock->formatStr, "00:00:00");
		k_setRect(&clockArea, x, y, x + FONT_DEFAULT_WIDTH * 8, y + FONT_DEFAULT_HEIGHT);
		k_memcpy(&clock->area, &clockArea, sizeof(Rect));
		break;

	case CLOCK_FORMAT_HMSA:
	default:
		k_strcpy(clock->formatStr, "00:00:00 AM");
		k_setRect(&clockArea, x, y, x + FONT_DEFAULT_WIDTH * 11, y + FONT_DEFAULT_HEIGHT);
		k_memcpy(&clock->area, &clockArea, sizeof(Rect));
		break;
	}	
}

void k_addClock(Clock* clock) {
	k_lock(&g_clockManager.mutex);

	k_addListToTail(&g_clockManager.clockList, clock);

	k_unlock(&g_clockManager.mutex);
}

Clock* k_removeClock(qword clockId) {
	Clock* clock;

	k_lock(&g_clockManager.mutex);

	clock = k_removeListById(&g_clockManager.clockList, clockId);

	k_unlock(&g_clockManager.mutex);

	return clock;
}

void k_drawAllClocks() {
	Clock* clock;

	k_lock(&g_clockManager.mutex);

	clock = k_getHeadFromList(&g_clockManager.clockList);
	while (clock != null) {
		k_drawClock(clock);
		clock = k_getNextFromList(&g_clockManager.clockList, clock);
	}

	k_unlock(&g_clockManager.mutex);
}

static void k_drawClock(Clock* clock) {
	byte hour, minute, second;
	bool pm = false;

	k_readRtcTime(&hour, &minute, &second);

	if ((g_clockManager.prevHour == hour) && (g_clockManager.prevMinute == minute) && (g_clockManager.prevSecond == second)) {
		return;
	}

	g_clockManager.prevHour = hour;
	g_clockManager.prevMinute = minute;
	g_clockManager.prevSecond = second;

	if (hour >= 12) {
		pm = true;
		if (hour > 12) {
			hour -= 12;
		}
	}

	switch (clock->format) {
	case CLOCK_FORMAT_H:
		clock->formatStr[0] = hour / 10 + '0';
		clock->formatStr[1] = hour % 10 + '0';
		break;

	case CLOCK_FORMAT_HA:
		clock->formatStr[0] = hour / 10 + '0';
		clock->formatStr[1] = hour % 10 + '0';
		if (pm == true) {
			clock->formatStr[3] = 'P';
		}
		break;

	case CLOCK_FORMAT_HM:
		clock->formatStr[0] = hour / 10 + '0';
		clock->formatStr[1] = hour % 10 + '0';
		clock->formatStr[3] = minute / 10 + '0';
		clock->formatStr[4] = minute % 10 + '0';
		break;

	case CLOCK_FORMAT_HMA:
		clock->formatStr[0] = hour / 10 + '0';
		clock->formatStr[1] = hour % 10 + '0';
		clock->formatStr[3] = minute / 10 + '0';
		clock->formatStr[4] = minute % 10 + '0';
		if (pm == true) {
			clock->formatStr[6] = 'P';
		}
		break;

	case CLOCK_FORMAT_HMS:
		clock->formatStr[0] = hour / 10 + '0';
		clock->formatStr[1] = hour % 10 + '0';
		clock->formatStr[3] = minute / 10 + '0';
		clock->formatStr[4] = minute % 10 + '0';
		clock->formatStr[6] = second / 10 + '0';
		clock->formatStr[7] = second % 10 + '0';
		break;

	case CLOCK_FORMAT_HMSA:
	default:
		clock->formatStr[0] = hour / 10 + '0';
		clock->formatStr[1] = hour % 10 + '0';
		clock->formatStr[3] = minute / 10 + '0';
		clock->formatStr[4] = minute % 10 + '0';
		clock->formatStr[6] = second / 10 + '0';
		clock->formatStr[7] = second % 10 + '0';
		if (pm == true) {
			clock->formatStr[9] = 'P';
		}
		break;
	}

	k_drawText(clock->windowId, clock->area.x1, clock->area.y1, clock->textColor, clock->backgroundColor, clock->formatStr, k_strlen(clock->formatStr));
	k_updateScreenByWindowArea(clock->windowId, &clock->area);
}
