#ifndef __2D_GRAPHICS_H__
#define __2D_GRAPHICS_H__

#include "types.h"

// HansOS uses 16 bits color.
// A color (16 bits) in video memory represents a pixel (16 bits) in screen.
typedef word Color;

// change r, g, b (24 bits) to 16 bits color.
// - r: 0 ~ 255 (8 bits) / 8 = 0 ~ 31 (5 bits)
// - g: 0 ~ 255 (8 bits) / 4 = 0 ~ 63 (6 bits)
// - b: 0 ~ 255 (8 bits) / 8 = 0 ~ 31 (5 bits)
// Being closer to 255 represents brighter color.
#define RGB(r, g, b) ((((byte)(r) >> 3) << 11) | (((byte)(g) >> 2) << 5) | ((byte)(b) >> 3))

void k_drawPixel(int x, int y, Color color);
void k_drawLine(int x1, int y1, int x2, int y2, Color color);
void k_drawRect(int x1, int y1, int x2, int y2, Color color, bool fill);
void k_drawCircle(int x, int y, int radius, Color color, bool fill);
void k_drawText(int x, int y, Color textColor, Color backgroundColor, const char* str);

#endif // __2D_GRAPHICS_H__
