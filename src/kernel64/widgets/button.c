#include "button.h"
#include "../core/window.h"
#include "../utils/util.h"
#include "../core/sync.h"

bool k_drawButton(qword windowId, const Rect* buttonArea, Color textColor, Color backgroundColor, const char* text, dword flags) {
	Window* window;
	Rect area;
	int textLen;
	int textX;
	int textY;
	Color brighterColor;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// set clipping area on window coordinates.
	k_setRect(&area, 0, 0, window->area.x2 - window->area.x1, window->area.y2 - window->area.y1);

	// draw a button background.
	__k_drawRect(window->buffer, &area, buttonArea->x1, buttonArea->y1, buttonArea->x2, buttonArea->y2, backgroundColor, true);	

	// get text length.
	textLen = k_strlen(text);

	// draw a text in the center.
	textX = (buttonArea->x1 + k_getRectWidth(buttonArea) / 2) - (textLen * FONT_DEFAULT_WIDTH) / 2;
	textY = (buttonArea->y1 + k_getRectHeight(buttonArea) / 2) - FONT_DEFAULT_HEIGHT / 2;
	__k_drawText(window->buffer, &area, textX, textY, textColor, backgroundColor, text, textLen);
	
	if (flags & BUTTON_FLAGS_SHADOW) {
		brighterColor = k_changeColorBrightness2(backgroundColor, 30, 30, 30);

		// draw top lines (2 pixels-thick) of the button with bright color in order to make it 3-dimensional.
		__k_drawLine(window->buffer, &area, buttonArea->x1, buttonArea->y1, buttonArea->x2, buttonArea->y1, brighterColor);
		__k_drawLine(window->buffer, &area, buttonArea->x1, buttonArea->y1 + 1, buttonArea->x2 - 1, buttonArea->y1 + 1, brighterColor);

		// draw left lines (2 pixels-thick) of the button with bright color in order to make it 3-dimensional.
		__k_drawLine(window->buffer, &area, buttonArea->x1, buttonArea->y1, buttonArea->x1, buttonArea->y2, brighterColor);
		__k_drawLine(window->buffer, &area, buttonArea->x1 + 1, buttonArea->y1, buttonArea->x1 + 1, buttonArea->y2 - 1, brighterColor);
		
		// draw bottom lines (2 pixels-thick) of the button with dark color in order to make it 3-dimensional.
		__k_drawLine(window->buffer, &area, buttonArea->x1 + 1, buttonArea->y2, buttonArea->x2, buttonArea->y2, WINDOW_COLOR_BUTTONDARK);
		__k_drawLine(window->buffer, &area, buttonArea->x1 + 2, buttonArea->y2 - 1, buttonArea->x2, buttonArea->y2 - 1, WINDOW_COLOR_BUTTONDARK);

		// draw right lines (2 pixels-thick) of the button with dark color in order to make it 3-dimensional.
		__k_drawLine(window->buffer, &area, buttonArea->x2, buttonArea->y1 + 1, buttonArea->x2, buttonArea->y2, WINDOW_COLOR_BUTTONDARK);
		__k_drawLine(window->buffer, &area, buttonArea->x2 - 1, buttonArea->y1 + 2, buttonArea->x2 - 1, buttonArea->y2, WINDOW_COLOR_BUTTONDARK);
	}

	k_unlock(&window->mutex);

	return true;
}
