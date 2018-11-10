#include "color_picker.h"
#include "../core/window.h"
#include "../utils/util.h"
#include "../core/console.h"
#include "../fonts/fonts.h"

void k_colorPickerTask(void) {
	Rect screenArea;
	qword windowId;
	int y;
	Event event;

	/* check graphic mode */
	if (k_isGraphicMode() == false) {
		k_printf("[color picker error] not graphic mode\n");
		return;
	}

	/* create window */
	k_getScreenArea(&screenArea);
	windowId = k_createWindow(screenArea.x2 - COLORPICKER_WIDTH - 5, screenArea.y1 + 35, COLORPICKER_WIDTH, COLORPICKER_HEIGHT, WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_BLOCKING, "Color Picker");
	if (windowId == WINDOW_INVALIDID) {
		return;
	}

	/* draw color picker */
	// draw selected color.
	y = WINDOW_TITLEBAR_HEIGHT + 10;
	k_drawText(windowId, 130, y, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "Selected Color", 14);
	k_drawRect(windowId, 100 - 1, y + 20 - 1, 270 + 1, y + 50 + 1, RGB(0, 0, 0), false);
	k_drawRect(windowId, 100, y + 20, 270, y + 50, RGB(255, 255, 255), true);
	y += 80;

	// draw color picker (Red, Green).
	k_drawText(windowId, 60, y, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "255", 3);
	k_drawText(windowId, 60 + 127, y, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "Red", 3);
	k_drawText(windowId, 60 + 255 - FONT_DEFAULT_WIDTH, y, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "0", 1);
	y += 20;
	k_drawText(windowId, 10 + FONT_DEFAULT_WIDTH * 2, y, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "255", 3);
	k_drawText(windowId, 10, y + 127, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "Green", 5);
	k_drawText(windowId, 10 + FONT_DEFAULT_WIDTH * 4, y + 255 - FONT_DEFAULT_HEIGHT, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "0", 1);
	
	k_drawRect(windowId, 60 - 1, y - 1, 60 + 255 + 1, y + 255 + 1, RGB(0, 0, 0), false);
	k_drawColorsByBlue(windowId, 60, y, 255);

	// draw color picker (Blue).
	k_drawText(windowId, 380 - FONT_DEFAULT_WIDTH * 2, y - 20, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "Blue", 4);
	k_drawLine(windowId, 380, y, 380, y + 255, RGB(0, 0, 0));
	k_drawText(windowId, 385, y, RGB(0, 0, 0), RGB(255, 255, 255), "255", 3);
	k_drawText(windowId, 385, y + 255 - FONT_DEFAULT_HEIGHT, RGB(0, 0, 0), RGB(255, 255, 255), "0", 1);
	y += 280;

	// draw number picker (RGB).
	k_drawText(windowId, 100, y, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "R", 1);
	k_drawRect(windowId, 115 - 1, y - 1, 115 + FONT_DEFAULT_WIDTH * 3 + 1, y + FONT_DEFAULT_HEIGHT + 1, RGB(0, 0, 0), false);
	k_drawRect(windowId, 115, y, 115 + FONT_DEFAULT_WIDTH * 3, y + FONT_DEFAULT_HEIGHT, RGB(255, 255, 255), true);

	k_drawText(windowId, 160, y, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "G", 1);
	k_drawRect(windowId, 175 - 1, y - 1, 175 + FONT_DEFAULT_WIDTH * 3 + 1, y + FONT_DEFAULT_HEIGHT + 1, RGB(0, 0, 0), false);
	k_drawRect(windowId, 175, y, 175 + FONT_DEFAULT_WIDTH * 3, y + FONT_DEFAULT_HEIGHT, RGB(255, 255, 255), true);

	k_drawText(windowId, 220, y, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "B", 1);
	k_drawRect(windowId, 235 - 1, y - 1, 235 + FONT_DEFAULT_WIDTH * 3 + 1, y + FONT_DEFAULT_HEIGHT + 1, RGB(0, 0, 0), false);
	k_drawRect(windowId, 235, y, 235 + FONT_DEFAULT_WIDTH * 3, y + FONT_DEFAULT_HEIGHT, RGB(255, 255, 255), true);

	k_showWindow(windowId, true);

	/* event processing loop */
	while (true) {
		if (k_recvEventFromWindow(&event, windowId) == false) {
			k_sleep(0);
			continue;
		}

		switch (event.type) {
		case EVENT_MOUSE_LBUTTONDOWN:
			break;

		case EVENT_WINDOW_CLOSE:
			k_deleteWindow(windowId);
			return;

		case EVENT_KEY_DOWN:
			break;

		default:
			break;
		}
	}
}

static void k_drawColorsByBlue(qword windowId, int x, int y, byte blue) {
	byte i, j;

	for (i = 0; i <= 255; i++) {
		for (j = 0; j <= 255; j++) {
			k_drawPixel(windowId, x + i, y + j, RGB((byte)255 - i, (byte)255 - j, blue));
		}
	}
}
