#ifndef __TYPES_2DGRAPHICS_H__
#define __TYPES_2DGRAPHICS_H__

#include "types.h"

// hOS uses 16 bits color.
// A color (16 bits) in video memory represents a pixel (16 bits) in screen.
typedef word Color;

// change r, g, b (24 bits) to 16 bits color.
// - r: 0 ~ 255 (8 bits) / 8 = 0 ~ 31 (5 bits)
// - g: 0 ~ 255 (8 bits) / 4 = 0 ~ 63 (6 bits)
// - b: 0 ~ 255 (8 bits) / 8 = 0 ~ 31 (5 bits)
// Being closer to 255 represents brighter color.
#define RGB(r, g, b) ((((byte)(r) >> 3) << 11) | (((byte)(g) >> 2) << 5) | ((byte)(b) >> 3))
#define GETR(rgb)    ((((rgb) & 0xF800) >> 11) << 3)
#define GETG(rgb)    ((((rgb) & 0x07E0) >> 5) << 2)
#define GETB(rgb)    (((rgb) & 0x001F) << 3)

#pragma pack(push, 1)

typedef struct __Point {
	int x; // x of point
	int y; // y of point
} Point;

typedef struct __Rect {
	int x1; // x of start point (top-left)
	int y1; // y of start point (top-left)
	int x2; // x of end point (bottom-right)
	int y2; // y of end point (bottom-right)
} Rect;

typedef struct __Circle {
	int x;      // x of origin
	int y;      // y of origin
	int radius; // radius
} Circle;

#pragma pack(pop)

#endif // __TYPES_2DGRAPHICS_H__
