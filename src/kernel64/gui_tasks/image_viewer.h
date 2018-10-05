#ifndef __GUITASKS_IMAGEVIEWER_H__
#define __GUITASKS_IMAGEVIEWER_H__

#include "../core/types.h"
#include "../core/window.h"

// image viewer color
#define IMAGEVIEWER_COLOR_BUTTONACTIVE RGB(79, 204, 11) // bright green

void k_imageViewerTask(void);
static void k_drawFileName(qword windowId, Rect* area, const char* fileName, int fileNameLen);
static bool k_showImage(qword mainWindowId, const char* fileName);

#endif // __GUITASKS_IMAGEVIEWER_H__