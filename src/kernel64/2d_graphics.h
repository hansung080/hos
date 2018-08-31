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

#pragma pack(push, 1)

typedef struct k_Point {
	int x; // x of point
	int y; // y of point
} Point;

typedef struct k_Rect {
	int x1; // x of start point (top-left)
	int y1; // y of start point (top-left)
	int x2; // x of end point (bottom-right)
	int y2; // y of end point (bottom-right)
} Rect;

#pragma pack(pop)

/* Rectangle-related Functions */
void k_setRect(Rect* rect, int x1, int y1, int x2, int y2);
int k_getRectWidth(const Rect* rect);
int k_getRectHeight(const Rect* rect);
bool k_isPointInRect(const Rect* rect, int x, int y);
bool k_isRectOverlapped(const Rect* rect1, const Rect* rect2);
bool k_getOverlappedRect(const Rect* rect1, const Rect* rect2, Rect* overRect);

/* Drawing Functions */
void __k_drawPixel(Color* outMem, const Rect* area, int x, int y, Color color);
void __k_drawLine(Color* outMem, const Rect* area, int x1, int y1, int x2, int y2, Color color);
void __k_drawRect(Color* outMem, const Rect* area, int x1, int y1, int x2, int y2, Color color, bool fill);
void __k_drawCircle(Color* outMem, const Rect* area, int x, int y, int radius, Color color, bool fill);
void __k_drawText(Color* outMem, const Rect* area, int x, int y, Color textColor, Color backgroundColor, const char* str);

#endif // __2D_GRAPHICS_H__
