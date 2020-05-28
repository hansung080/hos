#ifndef __WIDGETS_BUTTON_H__
#define __WIDGETS_BUTTON_H__

#include "../core/types.h"
#include "../core/2d_graphics.h"

/*** Button Macros ***/
// button flags
#define BUTTON_FLAGS_SHADOW 0x00000001 // 0: not draw shadow, 1: draw shadow

/*** Button Function ***/
bool k_drawButton(qword windowId, const Rect* buttonArea, Color textColor, Color backgroundColor, const char* text, dword flags);

#endif // __WIDGETS_BUTTON_H__