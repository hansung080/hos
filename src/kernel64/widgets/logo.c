#include "logo.h"
#include "../core/window.h"
#include "../core/sync.h"

bool k_drawHosLogo(qword windowId, int x, int y, int width, int height, Color brightColor, Color darkColor) {
	Window* window;
	Rect area;
	int w1, w2, w3;
	int h1, h2, h3;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// set clipping area on window coordinates.
	k_setRect(&area, 0, 0, window->area.x2 - window->area.x1, window->area.y2 - window->area.y1);

	w1 = width / 3;
	w2 = (width * 2) / 3;
	w3 = width;

	h1 = height / 3;
	h2 = (height * 2) /3;
	h3 = height;

	/* draw dark shadow of mark */
	__k_drawLine(window->buffer, &area, x + w1 - 1, y, x + w1 - 1, y + h1, darkColor);
	__k_drawLine(window->buffer, &area, x + w1, y, x + w1, y + h1, darkColor);

	__k_drawLine(window->buffer, &area, x + w1 - 1, y + h2, x + w1 - 1, y + h3, darkColor);
	__k_drawLine(window->buffer, &area, x + w1, y + h2, x + w1, y + h3, darkColor);	
	
	__k_drawLine(window->buffer, &area, x, y + h3 - 1, x + w1, y + h3 - 1, darkColor);
	__k_drawLine(window->buffer, &area, x, y + h3, x + w1, y + h3, darkColor);

	__k_drawLine(window->buffer, &area, x + w1, y + h2 - 1, x + w2, y + h2 - 1, darkColor);
	__k_drawLine(window->buffer, &area, x + w1, y + h2, x + w2, y + h2, darkColor);

	__k_drawLine(window->buffer, &area, x + w3 - 1, y, x + w3 - 1, y + h3, darkColor);
	__k_drawLine(window->buffer, &area, x + w3, y, x + w3, y + h3, darkColor);

	__k_drawLine(window->buffer, &area, x + w2, y + h3 - 1, x + w3, y + h3 - 1, darkColor);
	__k_drawLine(window->buffer, &area, x + w2, y + h3, x + w3, y + h3, darkColor);

	/* draw bright shadow of mark */
	__k_drawLine(window->buffer, &area, x, y, x + w1, y, brightColor);
	__k_drawLine(window->buffer, &area, x, y + 1, x + w1, y + 1, brightColor);

	__k_drawLine(window->buffer, &area, x, y, x, y + h3, brightColor);
	__k_drawLine(window->buffer, &area, x + 1, y, x + 1, y + h3, brightColor);

	__k_drawLine(window->buffer, &area, x + w1, y + h1, x + w2, y + h1, brightColor);
	__k_drawLine(window->buffer, &area, x + w1, y + h1 + 1, x + w2, y + h1 + 1, brightColor);

	__k_drawLine(window->buffer, &area, x + w2, y, x + w3, y, brightColor);
	__k_drawLine(window->buffer, &area, x + w2, y + 1, x + w3, y + 1, brightColor);

	__k_drawLine(window->buffer, &area, x + w2, y, x + w2, y + h1, brightColor);
	__k_drawLine(window->buffer, &area, x + w2 + 1, y, x + w2 + 1, y + h1, brightColor);

	__k_drawLine(window->buffer, &area, x + w2, y + h2, x + w2, y + h3, brightColor);
	__k_drawLine(window->buffer, &area, x + w2 + 1, y + h2, x + w2 + 1, y + h3, brightColor);

	k_unlock(&window->mutex);

	return true;
}
