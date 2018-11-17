#include "image_viewer.h"
#include "../utils/util.h"
#include "../core/console.h"
#include "../core/file_system.h"
#include "../fonts/fonts.h"
#include "../utils/jpeg.h"
#include "../core/dynamic_mem.h"

void k_imageViewerTask(void) {
	qword windowId;
	int windowWidth, windowHeight;
	int editBoxWidth;
	Rect screenArea;
	Rect editBoxArea;
	Rect buttonArea;
	char fileName[FS_MAXFILENAMELENGTH];
	int fileNameLen;
	Event event;
	MouseEvent* mouseEvent;
	KeyEvent* keyEvent;
	Point windowButtonPoint;
	Point screenButtonPoint;
	
	/* check graphic mode */
	if (k_isGraphicMode() == false) {
		k_printf("[image viewer error] not graphic mode\n");
		return;
	}

	/* create window */
	k_getScreenArea(&screenArea);

	windowWidth = FONT_DEFAULT_WIDTH * FS_MAXFILENAMELENGTH + 165;
	windowHeight = WINDOW_TITLEBAR_HEIGHT + 40;

	windowId = k_createWindow((screenArea.x2 - windowWidth) / 2, screenArea.y1 + WINDOW_APPPANEL_HEIGHT, windowWidth, windowHeight, WINDOW_FLAGS_DEFAULT & ~WINDOW_FLAGS_SHOW | WINDOW_FLAGS_BLOCKING, "Image Viewer");
	if (windowId == WINDOW_INVALIDID) {
		return;
	}

	// draw file name edit box.
	k_drawText(windowId, 5, WINDOW_TITLEBAR_HEIGHT + 6, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "File Name:", 10);
	editBoxWidth = FONT_DEFAULT_WIDTH * FS_MAXFILENAMELENGTH + 4;
	k_setRect(&editBoxArea, 85, WINDOW_TITLEBAR_HEIGHT + 5, 85 + editBoxWidth, WINDOW_TITLEBAR_HEIGHT + 25);
	k_drawRect(windowId, editBoxArea.x1, editBoxArea.y1, editBoxArea.x2, editBoxArea.y2, RGB(0, 0, 0), false);

	// clear file name edit box.
	k_memset(fileName, 0, sizeof(fileName));
	fileNameLen = 0;
	k_drawFileName(windowId, &editBoxArea, fileName, fileNameLen);

	// draw show button.
	k_setRect(&buttonArea, editBoxArea.x2 + 10, editBoxArea.y1, editBoxArea.x2 + 70, editBoxArea.y2);
	k_drawButton(windowId, &buttonArea, WINDOW_COLOR_BACKGROUND, "Show", RGB(0, 0, 0), BUTTON_FLAGS_SHADOW);

	k_showWindow(windowId, true);

	/* event processing loop */
	while (true) {
		if (k_recvEventFromWindow(&event, windowId) == false) {
			k_sleep(0);
			continue;
		}
		
		switch (event.type) {
		case EVENT_MOUSE_LBUTTONDOWN:
			mouseEvent = &event.mouseEvent;

			if (k_isPointInRect(&buttonArea, mouseEvent->point.x, mouseEvent->point.y) == true) {
				k_drawButton(windowId, &buttonArea, IMAGEVIEWER_COLOR_BUTTONACTIVE, "Show", RGB(255, 255, 255), BUTTON_FLAGS_SHADOW);
				k_updateScreenByWindowArea(windowId, &buttonArea);

				if (k_showImage(windowId, fileName) == false) {
					// sleep 200 milliseconds in order to show the button-down effect when image viewer creation fails.
					k_sleep(200);
				}

				k_drawButton(windowId, &buttonArea, WINDOW_COLOR_BACKGROUND, "Show", RGB(0, 0, 0), BUTTON_FLAGS_SHADOW);
				k_updateScreenByWindowArea(windowId, &buttonArea);
			}

			break;

		case EVENT_WINDOW_CLOSE:
			k_deleteWindow(windowId);
			return;

		case EVENT_KEY_DOWN:
			keyEvent = &event.keyEvent;

			/* process backspace key */
			if (keyEvent->asciiCode == KEY_BACKSPACE) {
				if (fileNameLen > 0) {
					fileName[fileNameLen--] = '\0';
					k_drawFileName(windowId, &editBoxArea, fileName, fileNameLen);
				}

			/* process enter key */
			} else if (keyEvent->asciiCode == KEY_ENTER) {
				if (fileNameLen > 0) {
					windowButtonPoint.x = buttonArea.x1;
					windowButtonPoint.y = buttonArea.y1;
					k_convertPointWindowToScreen(windowId, &windowButtonPoint, &screenButtonPoint);
					k_sendMouseEventToWindow(windowId, EVENT_MOUSE_LBUTTONDOWN, screenButtonPoint.x, screenButtonPoint.y, 0);
				}

			/* process ESC key */
			} else if (keyEvent->asciiCode == KEY_ESC) {
				k_sendWindowEventToWindow(windowId, EVENT_WINDOW_CLOSE);

			/* process other keys */
			} else if (keyEvent->asciiCode < 128) {
				if (fileNameLen <= (FS_MAXFILENAMELENGTH - 1)) {
					fileName[fileNameLen++] = keyEvent->asciiCode;
					k_drawFileName(windowId, &editBoxArea, fileName, fileNameLen);
				}
			}

			break;

		default:
			break;
		}
	}
}

static void k_drawFileName(qword windowId, Rect* area, const char* fileName, int fileNameLen) {
	k_drawRect(windowId, area->x1 + 1, area->y1 + 1, area->x2 - 1, area->y2 - 1, WINDOW_COLOR_BACKGROUND, true);
	k_drawText(windowId, area->x1 + 2, area->y1 + 2, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, fileName, fileNameLen);
	if (fileNameLen <= (FS_MAXFILENAMELENGTH - 1)) {
		k_drawText(windowId, area->x1 + 2 + FONT_DEFAULT_WIDTH * fileNameLen, area->y1 + 2, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "_", 1);	
	}

	k_updateScreenByWindowArea(windowId, area);
}

static bool k_showImage(qword mainWindowId, const char* fileName) {
	Dir* dir;
	dirent* entry;
	dword fileSize;
	File* file = null;
	byte* fileBuffer = null;
	Jpeg* jpeg = null;
	Color* imageBuffer = null;
	Rect screenArea;
	qword windowId = WINDOW_INVALIDID;
	Window* window;
	int windowWidth;
	Event event;
	KeyEvent* keyEvent;
	int imageWidth, imageHeight;
	bool exit;
	
	/* search image file in root directory */
	dir = opendir("/");
	fileSize = 0;

	while (true) {
		entry = readdir(dir);
		if (entry == null) {
			break;
		}

		if (k_equalStr(entry->d_name, fileName) == true) {
			fileSize = entry->d_size;
			break;
		}
	}

	closedir(dir);

	if (fileSize == 0) {
		k_printf("[image viewer error] %s does not exist or is zero-sized.\n", fileName);
		return false;
	}

	/* read image file and decode it */
	file = fopen(fileName, "rb");
	if (file == null) {
		k_printf("[image viewer error] %s opening failure\n", fileName);
		return false;
	}

	fileBuffer = (byte*)k_allocMem(fileSize);
	if (fileBuffer == null) {
		k_printf("[image viewer error] file buffer allocation failure\n");
		fclose(file);
		return false;
	}

	jpeg = (Jpeg*)k_allocMem(sizeof(Jpeg));
	if (jpeg == null) {
		k_printf("[image viewer error] JPEG allocation failure\n");
		k_freeMem(fileBuffer);
		fclose(file);
		return false;
	}

	if (fread(fileBuffer, 1, fileSize, file) != fileSize) {
		k_printf("[image viewer error] %s reading failure\n", fileName);
		k_freeMem(jpeg);
		k_freeMem(fileBuffer);
		fclose(file);
		return false;
	}

	fclose(file);

	if (k_initJpeg(jpeg, fileBuffer, fileSize) == false) {
		k_printf("[image viewer error] JPEG initialization failure\n");
		k_freeMem(jpeg);
		k_freeMem(fileBuffer);
		return false;
	}
	
	imageBuffer = (Color*)k_allocMem(sizeof(Color) * jpeg->width * jpeg->height);	
	if (imageBuffer == null) {
		k_printf("[image viewer error] image buffer allocation failure\n");
		k_freeMem(jpeg);
		k_freeMem(fileBuffer);
		return false;
	}

	if (k_decodeJpeg(jpeg, imageBuffer) == false) {
		k_printf("[image viewer error] JPEG decoding failure\n");
		k_freeMem(imageBuffer);
		k_freeMem(jpeg);
		k_freeMem(fileBuffer);
		return false;
	}

	k_getScreenArea(&screenArea);
	windowId = k_createWindow((screenArea.x2 - jpeg->width) / 2, (screenArea.y2 - jpeg->height) / 2, jpeg->width, jpeg->height + WINDOW_TITLEBAR_HEIGHT, WINDOW_FLAGS_DEFAULT & ~WINDOW_FLAGS_SHOW | WINDOW_FLAGS_RESIZABLE | WINDOW_FLAGS_BLOCKING, fileName);
	if (windowId == WINDOW_INVALIDID) {
		k_printf("[image viewer error] image viewer creation failure\n");
		k_freeMem(imageBuffer);
		k_freeMem(jpeg);
		k_freeMem(fileBuffer);
		return false;	
	}	

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		k_printf("[image viewer error] can not get image viewer.\n");
		k_deleteWindow(windowId);
		k_freeMem(imageBuffer);
		k_freeMem(jpeg);
		k_freeMem(fileBuffer);
		return false;		
	}

	windowWidth = k_getRectWidth(&window->area);
	k_memcpy(window->buffer + (windowWidth * WINDOW_TITLEBAR_HEIGHT), imageBuffer, sizeof(Color) * jpeg->width * jpeg->height);
	k_unlock(&window->mutex);

	// back up image width, image height before freeing jpeg.
	imageWidth = jpeg->width;
	imageHeight = jpeg->height;

	// Do not free image buffer here in order to reuse it when EVENT_WINDOW_RESIZE event receives.
	//k_freeMem(imageBuffer);
	k_freeMem(jpeg);
	k_freeMem(fileBuffer);

	k_showWindow(windowId, true);
	k_showWindow(mainWindowId, false);

	exit = false;

	/* event processing loop */	
	while (exit == false) {
		if (k_recvEventFromWindow(&event, windowId) == false) {
			k_sleep(0);
			continue;
		}

		switch (event.type) {
		case EVENT_WINDOW_RESIZE:
			k_bitblt(windowId, 0, WINDOW_TITLEBAR_HEIGHT, imageBuffer, imageWidth, imageHeight);
			k_showWindow(windowId, true);
			break;

		case EVENT_WINDOW_CLOSE:
			k_deleteWindow(windowId);
			k_showWindow(mainWindowId, true);
			exit = true;
			break;

		case EVENT_KEY_DOWN:
			keyEvent = &event.keyEvent;

			/* process ESC key */
			if (keyEvent->asciiCode == KEY_ESC) {
				k_deleteWindow(windowId);
				k_showWindow(mainWindowId, true);
				exit = true;
			}

			break;

		default:
			break;
		}
	}

	k_freeMem(imageBuffer);	

	return true;
}
