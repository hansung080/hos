#ifndef __GUITASKS_COLORPICKER_H__
#define __GUITASKS_COLORPICKER_H__

#include "../core/types.h"

#define COLORPICKER_WIDTH  450
#define COLORPICKER_HEIGHT 450

void k_colorPickerTask(void);
static void k_drawColorsByBlue(qword windowId, int x, int y, byte blue);

#endif // __GUITASKS_COLORPICKER_H__