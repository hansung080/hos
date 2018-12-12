#include "window.h"
#include "vbe.h"
#include "task.h"
#include "multiprocessor.h"
#include "../fonts/fonts.h"
#include "dynamic_mem.h"
#include "../utils/util.h"
#include "console.h"
#include "../utils/jpeg.h"
#include "../images/images.h"

static WindowPoolManager g_windowPoolManager;
static WindowManager g_windowManager;

static void k_initWindowPool(void) {
	int i;
	void* windowPoolAddr;

	// initialize window pool manager.
	k_memset(&g_windowPoolManager, 0, sizeof(g_windowPoolManager));

	// allocate window pool.
	windowPoolAddr = k_allocMem(sizeof(Window) * WINDOW_MAXCOUNT);
	if (windowPoolAddr == null) {
		k_printf("window error: window pool allocation failure\n");
		while (true) {
			;
		}
	}

	// initialize window pool.
	g_windowPoolManager.startAddr = (Window*)windowPoolAddr;
	k_memset(windowPoolAddr, 0, sizeof(Window) * WINDOW_MAXCOUNT);

	// set window ID (offset) to windows in pool.
	for (i = 0; i < WINDOW_MAXCOUNT; i++) {
		g_windowPoolManager.startAddr[i].link.id = i;
	}

	// initialize max window count and allocated window count.
	g_windowPoolManager.maxCount = WINDOW_MAXCOUNT;
	g_windowPoolManager.allocatedCount = 1;

	// initialize mutex of window pool manager.
	k_initMutex(&g_windowPoolManager.mutex);
}

static Window* k_allocWindow(void) {
	Window* emptyWindow;
	int i; // window offset

	k_lock(&g_windowPoolManager.mutex);

	if (g_windowPoolManager.usedCount >= g_windowPoolManager.maxCount) {
		k_unlock(&g_windowPoolManager.mutex);
		return null;
	}

	for (i = 0; i < g_windowPoolManager.maxCount; i++) {
		// If allocated window count (high 32 bits) of window ID == 0, it's not allocated.
		if ((g_windowPoolManager.startAddr[i].link.id >> 32) == 0) {
			emptyWindow = &(g_windowPoolManager.startAddr[i]);
			break;
		}
	}

	// set not-0 to allocated window count (high 32 bits) of window ID in order to mark it allocated.
	// window ID consists of allocated window count (high 32 bits) and window offset (low 32 bits).
	emptyWindow->link.id = (((qword)g_windowPoolManager.allocatedCount) << 32) | i;

	g_windowPoolManager.usedCount++;
	g_windowPoolManager.allocatedCount++;
	if (g_windowPoolManager.allocatedCount == 0) {
		g_windowPoolManager.allocatedCount = 1;
	}

	k_unlock(&g_windowPoolManager.mutex);

	// initialize mutex of window.
	k_initMutex(&emptyWindow->mutex);

	return emptyWindow;
}

static void k_freeWindow(qword windowId) {
	int i; // window offset

	// get window offset (low 32 bits) of window ID.
	i = GETWINDOWOFFSET(windowId);

	k_lock(&g_windowPoolManager.mutex);

	// initialize window.
	k_memset(&(g_windowPoolManager.startAddr[i]), 0, sizeof(Window));

	// initialize window ID.
	// set 0 to allocated window count (high 32 bits) of window ID in order to mark it free.
	g_windowPoolManager.startAddr[i].link.id = i;

	g_windowPoolManager.usedCount--;

	k_unlock(&g_windowPoolManager.mutex);
}

void k_initGuiSystem(void) {
	VbeModeInfoBlock* vbeMode;
	qword backgroundId;

	/* initialize window pool */
	k_initWindowPool();

	/* initialize window manager */
	vbeMode = k_getVbeModeInfoBlock();
	g_windowManager.videoMem = (Color*)(((qword)vbeMode->physicalBaseAddr) & 0xFFFFFFFF);

	g_windowManager.mouseX = vbeMode->xResolution / 2;
	g_windowManager.mouseY = vbeMode->yResolution / 2;

	g_windowManager.screenArea.x1 = 0;
	g_windowManager.screenArea.y1 = 0;
	g_windowManager.screenArea.x2 = vbeMode->xResolution - 1;
	g_windowManager.screenArea.y2 = vbeMode->yResolution - 1;

	k_initMutex(&g_windowManager.mutex);

	k_initList(&g_windowManager.windowList);

	// allocate event buffer.
	g_windowManager.eventBuffer = (Event*)k_allocMem(sizeof(Event) * EVENTQUEUE_WINMGR_MAXCOUNT);
	if (g_windowManager.eventBuffer == null) {
		k_printf("window error: window manager event buffer allocation failure\n");
		while (true) {
			;
		}
	}

	k_initQueue(&g_windowManager.eventQueue, g_windowManager.eventBuffer, sizeof(Event), EVENTQUEUE_WINMGR_MAXCOUNT, false);

	// allocate screen bitmap.
	g_windowManager.screenBitmap = (byte*)k_allocMem((vbeMode->xResolution * vbeMode->yResolution + 7) / 8);
	if (g_windowManager.screenBitmap == null) {
		k_printf("window error: window manager screen bitmap allocation failure\n");
	}

	g_windowManager.prevButtonStatus = 0;
	g_windowManager.prevUnderMouseId = WINDOW_INVALIDID;
	g_windowManager.moving = false;
	g_windowManager.movingId = WINDOW_INVALIDID;

	g_windowManager.resizing = false;
	g_windowManager.resizingId = WINDOW_INVALIDID;
	k_memset(&g_windowManager.resizingArea, 0, sizeof(Rect));

	g_windowManager.overCloseId = WINDOW_INVALIDID;
	g_windowManager.overResizeId = WINDOW_INVALIDID;
	g_windowManager.overMenuId = WINDOW_INVALIDID;

	/* create background window */
	// set 0 to flags in order not to show. After drawing background color, It will show.
	backgroundId = k_createWindow(0, 0, vbeMode->xResolution, vbeMode->yResolution, 0, WINDOW_SYSBACKGROUND_TITLE, WINDOW_COLOR_SYSBACKGROUND, null, null, WINDOW_INVALIDID);
	g_windowManager.backgroundId = backgroundId;
	k_drawHansLogo(g_windowManager.backgroundId, (vbeMode->xResolution - WINDOW_SYSBACKGROUND_LOGOWIDTH) / 2, (vbeMode->yResolution - WINDOW_SYSBACKGROUND_LOGOHEIGHT) / 2, WINDOW_SYSBACKGROUND_LOGOWIDTH, WINDOW_SYSBACKGROUND_LOGOHEIGHT, WINDOW_COLOR_SYSBACKGROUNDLOGOBRIGHT, WINDOW_COLOR_SYSBACKGROUNDLOGODARK);
	//k_drawBackgroundImage();
	k_showWindow(backgroundId, true);
}

WindowManager* k_getWindowManager(void) {
	return &g_windowManager;
}

qword k_getBackgroundWindowId(void) {
	return g_windowManager.backgroundId;
}

void k_getScreenArea(Rect* screenArea) {
	k_memcpy(screenArea, &g_windowManager.screenArea, sizeof(Rect));
}

qword k_createWindow(int x, int y, int width, int height, dword flags, const char* title, Color backgroundColor, Menu* topMenu, void* widget, qword parentId) {
	Window* window;
	Window* parent;
	Task* task;
	qword topId;

	if ((width <= 0) || (height <= 0)) {
		k_printf("window error: negative window width or height: %d, %d\n", width, height);
		return WINDOW_INVALIDID;
	}

	if (flags & WINDOW_FLAGS_DRAWTITLEBAR) {
		if (width < WINDOW_MINWIDTH) {
			width = WINDOW_MINWIDTH;
		}

		if (height < WINDOW_MINHEIGHT) {
			height = WINDOW_MINHEIGHT;
		}
	}

	/* allocate window */
	window = k_allocWindow();
	if (window == null) {
		k_printf("window error: window allocation failure\n");
		return WINDOW_INVALIDID;
	}

	/* initialize window */
	window->area.x1 = x;
	window->area.y1 = y;
	window->area.x2 = x + width - 1;
	window->area.y2 = y + height - 1;

	k_memcpy(window->title, title, WINDOW_MAXTITLELENGTH);
	window->title[WINDOW_MAXTITLELENGTH] = '\0';

	// allocate window buffer.
	window->buffer = (Color*)k_allocMem(sizeof(Color) * width * height);
	if (window->buffer == null) {
		k_printf("window error: window buffer allocation failure\n");
		k_freeWindow(window->link.id);
		return WINDOW_INVALIDID;
	}

	// allocate event buffer.
	window->eventBuffer = (Event*)k_allocMem(sizeof(Event) * EVENTQUEUE_WINDOW_MAXCOUNT);
	if (window->eventBuffer == null) {
		k_printf("window error: event buffer allocation failure\n");
		k_freeMem(window->buffer);
		k_freeWindow(window->link.id);
		return WINDOW_INVALIDID;	
	}

	if (flags & WINDOW_FLAGS_BLOCKING) {
		k_initQueue(&window->eventQueue, window->eventBuffer, sizeof(Event), EVENTQUEUE_WINDOW_MAXCOUNT, true);

	} else {
		k_initQueue(&window->eventQueue, window->eventBuffer, sizeof(Event), EVENTQUEUE_WINDOW_MAXCOUNT, false);
	}
	
	task = k_getRunningTask(k_getApicId());
	task->flags |= TASK_FLAGS_GUI;
	window->taskId = task->link.id;

	window->flags = flags;
	window->backgroundColor = backgroundColor;
	window->topMenu = topMenu;
	window->widget = widget;

	k_initList(&window->childList);

	if ((flags & WINDOW_FLAGS_CHILD) && (parentId != WINDOW_INVALIDID) && (parentId != 0)) {
		window->parentId = parentId;
		parent = k_getWindowWithLock(parentId);
		if (parent != null) {
			k_addListToTail(&parent->childList, &window->childLink);
			parent->flags |= WINDOW_FLAGS_HASCHILD;
			k_unlock(&parent->mutex);
		}

	} else {
		window->parentId = WINDOW_INVALIDID;
	}

	/* draw window to window buffer */
	k_drawWindowBackground(window->link.id);

	if (flags & WINDOW_FLAGS_DRAWFRAME) {
		k_drawWindowFrame(window->link.id);
	}

	if (topMenu != null) {
		k_createMenu(topMenu, (FONT_DEFAULT_WIDTH * k_strlen(title)) + (MENU_ITEMWIDTHPADDING * 2), 0, MENU_ITEMHEIGHT_TITLEBAR, null, window->link.id, null, MENU_FLAGS_HORIZONTAL | MENU_FLAGS_DRAWONPARENT);
	}

	if (flags & WINDOW_FLAGS_DRAWTITLEBAR) {
		k_drawWindowTitleBar(window->link.id, true);
	}

	/* add window to window list */
	k_lock(&g_windowManager.mutex);

	topId = k_getTopWindowId();

	// add window to the top of screen.
	// window list: head -> tail == the top window -> the bottom window
	k_addListToHead(&g_windowManager.windowList, window);

	k_unlock(&g_windowManager.mutex);

	/* update screen and send window event */
	k_updateScreenById(window->link.id);
	k_sendWindowEventToWindow(window->link.id, EVENT_WINDOW_SELECT);

	if ((topId != WINDOW_INVALIDID) && (topId != g_windowManager.backgroundId)) {
		k_sendWindowEventToWindow(topId, EVENT_WINDOW_DESELECT);
		if (!((window->flags & WINDOW_FLAGS_CHILD) && (k_isTopWindowWithChild(window->parentId) == true))) {
			k_updateWindowTitleBar(topId, false);
		}
	}

	return window->link.id;
}

bool k_deleteWindow(qword windowId) {
	Window* window;
	Rect area;
	qword topId;
	bool top;

	k_lock(&g_windowManager.mutex);

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		k_unlock(&g_windowManager.mutex);
		return false;
	}

	k_memcpy(&area, &window->area, sizeof(Rect));

	if (k_getTopWindowId() == windowId) {
		top = true;

	} else {
		top = false;
	}

	/* delete child windows */
	if (window->flags & WINDOW_FLAGS_HASCHILD) {
		k_deleteChildWindows(windowId);
	}

	/* remove window from window list */
	if (k_removeListById(&g_windowManager.windowList, windowId) == null) {
		k_unlock(&window->mutex);
		k_unlock(&g_windowManager.mutex);
		return false;
	}

	/* free window buffer */
	k_freeMem(window->buffer);
	window->buffer = null;

	/* free event buffer */
	k_closeQueue(&window->eventQueue);
	k_freeMem(window->eventBuffer);
	window->eventBuffer = null;

	k_unlock(&window->mutex);

	/* free window */
	k_freeWindow(windowId);

	k_unlock(&g_windowManager.mutex);

	/* update screen and send window event */
	k_updateScreenByScreenArea(&area);

	if (top == true) {
		topId = k_getTopWindowId();
		if (topId != WINDOW_INVALIDID) {
			k_sendWindowEventToWindow(topId, EVENT_WINDOW_SELECT);
			k_updateWindowTitleBar(topId, true);
		}
	}

	return true;
}

bool k_deleteWindowsByTask(qword taskId) {
	Window* window;
	Window* nextWindow;

	k_lock(&g_windowManager.mutex);

	window = k_getHeadFromList(&g_windowManager.windowList);
	while (window != null) {
		// Getting the next window must be before deleting the current window.
		nextWindow = k_getNextFromList(&g_windowManager.windowList, window);

		if ((window->link.id != g_windowManager.backgroundId) && (window->taskId == taskId)) {
			k_deleteWindow(window->link.id);
		}

		window = nextWindow;
	}

	k_unlock(&g_windowManager.mutex);	
}

bool k_closeWindowsByTask(qword taskId) {
	Window* window;
	Window* nextWindow;

	k_lock(&g_windowManager.mutex);

	window = k_getHeadFromList(&g_windowManager.windowList);
	while (window != null) {
		// Getting the next window must be before closing the current window.
		nextWindow = k_getNextFromList(&g_windowManager.windowList, window);

		if ((window->link.id != g_windowManager.backgroundId) && (window->taskId == taskId)) {
			k_sendWindowEventToWindow(window->link.id, EVENT_WINDOW_CLOSE);
		}

		window = nextWindow;
	}

	k_unlock(&g_windowManager.mutex);	
}

Window* k_getWindow(qword windowId) {
	Window* window;

	if (GETWINDOWOFFSET(windowId) >= WINDOW_MAXCOUNT) {
		return null;
	}

	// get window from window pool.
	window = &(g_windowPoolManager.startAddr[GETWINDOWOFFSET(windowId)]);
	if (window->link.id == windowId) {
		return window;
	}

	return null;
}

/**
  [NOTE]
  Window manager mutex has higher priority than window mutex.
  Thus, In the case that window manager mutex and window mutex are getting a lock at the same time,
  make sure that window manager mutex gets a lock before window mutex gets a lock.
*/
Window* k_getWindowWithLock(qword windowId) {
	Window* window;

	window = k_getWindow(windowId);
	if (window == null) {
		return null;
	}

	k_lock(&window->mutex);

	// Window in window pool might be changed while getting a lock.
	// Thus, getting window after getting a lock is necessary.
	// If window ID before getting a lock == window ID after getting a lock, window is not changed.
	window = k_getWindow(windowId);
	if (window == null) {
		k_unlock(&window->mutex);
		return null;
	}

	return window;
}

bool k_isWindowShown(qword windowId) {
	Window* window;

	window = k_getWindow(windowId);
	if (window == null) {
		return false;
	}

	if (window->flags & WINDOW_FLAGS_SHOW) {
		return true;
	}

	return false;
}

bool k_showWindow(qword windowId, bool show) {
	Window* window;
	Rect windowArea;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	if (show == true) {
		window->flags |= WINDOW_FLAGS_SHOW;

	} else {
		window->flags &= ~WINDOW_FLAGS_SHOW;
	}

	k_unlock(&window->mutex);

	if (show == true) {
		k_updateScreenById(windowId);

	} else {
		k_getWindowArea(windowId, &windowArea);
		k_updateScreenByScreenArea(&windowArea);
	}

	return true;
}

// param: area: requested update area
bool k_redrawWindowByArea(qword windowId, const Rect* area) {
	ScreenBitmap bitmap;
	Window* window;
	Rect copyArea;
	int copyAreaSize;
	Rect copiedAreas[WINDOW_MAXCOPIEDAREAARRAYCOUNT];
	int copiedAreaSizes[WINDOW_MAXCOPIEDAREAARRAYCOUNT];
	int minAreaSize;
	int minAreaIndex;
	int i;
	Rect cursorArea;

	/**
	  < Copied Area Array >
	  Copied area array has the largest 20 area which has been already updated.
	  This array-related logic is added in order to reduce the number of k_copyWindowBufferToVideoMem-calls
	  by skipping calling it if copy area is included in copied area which has been already updated, 
	  because k_copyWindowBufferToVideoMem takes a long time.
	*/
	k_memset(copiedAreas, 0, sizeof(copiedAreas));
	k_memset(copiedAreaSizes, 0, sizeof(copiedAreaSizes));

	k_lock(&g_windowManager.mutex);

	/* create screen bitmap */
	if (k_createScreenBitmap(&bitmap, area) == false) {
		k_unlock(&g_windowManager.mutex);		
		return false;
	}

	/**
	  redraw windows' partial area (copy area) overlapped with update area,
	  by looping from head (the top window) to tail (the bottom window) in window list.
	*/
	window = k_getHeadFromList(&g_windowManager.windowList);
	while (window != null) {
		if ((window->flags & WINDOW_FLAGS_SHOW) && (k_getOverlappedRect(&bitmap.area, &window->area, &copyArea) == true)) {
			/* check copy area is included in copied area */
			copyAreaSize = k_getRectWidth(&copyArea) * k_getRectHeight(&copyArea);

			for (i = 0; i < WINDOW_MAXCOPIEDAREAARRAYCOUNT; i++) {
				if ((copyAreaSize <= copiedAreaSizes[i]) && (k_isPointInRect(&copiedAreas[i], copyArea.x1, copyArea.y1) == true) && (k_isPointInRect(&copiedAreas[i], copyArea.x2, copyArea.y2) == true)) {
					break;
				}
			}

			// If copy area is included in copied area, go to the next window,
			// because it means the current window's copy area is invisible.
			if (i < WINDOW_MAXCOPIEDAREAARRAYCOUNT) {
				window = k_getNextFromList(&g_windowManager.windowList, window);
				continue;
			}

			/* insert copy area to copied area array */
			minAreaSize = 0x7FFFFFFF; // 2147483647 > 786432 = 1024 * 768
			minAreaIndex = 0;
			for (i = 0; i < WINDOW_MAXCOPIEDAREAARRAYCOUNT; i++) {
				if (copiedAreaSizes[i] < minAreaSize) {
					minAreaSize = copiedAreaSizes[i];
					minAreaIndex = i;
				}
			}

			if (copyAreaSize > minAreaSize) {
				k_memcpy(&copiedAreas[minAreaIndex], &copyArea, sizeof(Rect));
				copiedAreaSizes[minAreaIndex] = copyAreaSize;
			}

			/* copy or fill copy area */
			k_lock(&window->mutex);

			if ((windowId != WINDOW_INVALIDID) && (windowId != window->link.id)) {
				k_fillScreenBitmap(&bitmap, &window->area, false);

			} else {
				k_copyWindowBufferToVideoMem(window, &bitmap);
			}
						
			k_unlock(&window->mutex);
		}

		// If all pixels of update area have been updated, break the loop,
		// because it means remaining windows' copy area is invisible.
		if (k_isScreenBitmapAllOff(&bitmap) == true) {
			break;
		}

		window = k_getNextFromList(&g_windowManager.windowList, window);
	}

	k_unlock(&g_windowManager.mutex);

	/**
	  redraw mouse cursor if it's overlapped with update area.
	*/
	k_setRect(&cursorArea, g_windowManager.mouseX, g_windowManager.mouseY, g_windowManager.mouseX + MOUSE_CURSOR_WIDTH, g_windowManager.mouseY + MOUSE_CURSOR_HEIGHT);
	
	if (k_isRectOverlapped(&bitmap.area, &cursorArea) == true) {
		k_drawMouseCursor(g_windowManager.mouseX, g_windowManager.mouseY);
	}
	
	return true;
}

static void k_copyWindowBufferToVideoMem(const Window* window, ScreenBitmap* bitmap) {
	int screenWidth;
	int windowWidth;
	Color* currentVideoMem;
	Color* currentWindowBuffer;
	Rect copyArea;     // copy area
	int copyWidth;     // copy area width
	int copyHeight;    // copy area height
	int copyX;         // x of copy area coordinates
	int copyY;         // y of copy area coordinates
	int byteOffset;    // byte offset in bitmap
	int bitOffset;     // bit offset in byte
	int lastBitOffset; // last bit offset in byte
	int byteCount;     // byte count
	byte data;         // data to set bitmap
	int i;

	if (k_getOverlappedRect(&bitmap->area, &window->area, &copyArea) == false) {
		return;
	}

	screenWidth = k_getRectWidth(&g_windowManager.screenArea);
	windowWidth = k_getRectWidth(&window->area);
	copyWidth = k_getRectWidth(&copyArea);
	copyHeight = k_getRectHeight(&copyArea);

	/* copy area loop */
	for (copyY = 0; copyY < copyHeight; copyY++) {
		if (k_getStartOffsetInScreenBitmap(bitmap, copyArea.x1, copyArea.y1 + copyY, &byteOffset, &bitOffset) == false) {
			break;
		}

		// for current video memory, convert (0, copy y) from copy area coordinates to screen coordinates.
		// for current window buffer, convert (0, copy y) from copy area coordinates to window coordinates.
		currentVideoMem = g_windowManager.videoMem + (copyArea.y1 + copyY) * screenWidth + copyArea.x1;
		currentWindowBuffer = window->buffer + (copyArea.y1 - window->area.y1 + copyY) * windowWidth + (copyArea.x1 - window->area.x1);

		for (copyX = 0; copyX < copyWidth; ) {
			/* copy memory and set bitmap by byte-level */
			if ((bitmap->bitmap[byteOffset] == 0xFF) && (bitOffset == 0) && ((copyWidth - copyX) >= 8)) {
				// [REF] remaining bit count in a line of copy area: copy width - copy x
				for (byteCount = 0; byteCount < ((copyWidth - copyX) >> 3); byteCount++) {
					if (bitmap->bitmap[byteOffset + byteCount] != 0xFF) {
						break;
					}
				}

				k_memcpy(currentVideoMem, currentWindowBuffer, (sizeof(Color) * byteCount) << 3);
				currentVideoMem += byteCount << 3;
				currentWindowBuffer += byteCount << 3;

				k_memset(bitmap->bitmap + byteOffset, 0x00, byteCount);
				copyX += byteCount << 3;
				byteOffset += byteCount;
				bitOffset = 0;

			/* skip memory and bitmap by byte-level */
			} else if ((bitmap->bitmap[byteOffset] == 0x00) && (bitOffset == 0) && ((copyWidth - copyX) >= 8)) {
				for (byteCount = 0; byteCount < ((copyWidth - copyX) >> 3); byteCount++) {
					if (bitmap->bitmap[byteOffset + byteCount] != 0x00) {
						break;
					}
				}

				currentVideoMem += byteCount << 3;
				currentWindowBuffer += byteCount << 3;

				copyX += byteCount << 3;
				byteOffset += byteCount;
				bitOffset = 0;

			/* copy memory and set bitmap by bit-level */	
			} else {
				data = bitmap->bitmap[byteOffset];

				lastBitOffset = MIN(8, bitOffset + copyWidth - copyX);
				for (i = bitOffset; i < lastBitOffset; i++) {
					if (data & (0x01 << i)) {
						*currentVideoMem = *currentWindowBuffer;
						data &= ~(0x01 << i);
					}

					currentVideoMem++;
					currentWindowBuffer++;
				}

				bitmap->bitmap[byteOffset] = data;

				copyX += lastBitOffset - bitOffset;
				byteOffset++;
				bitOffset = 0;
			}
		}
	}
}

/**
  < Screen Bitmap Management >
                       
         screen             bitmap (1024 * 768 bits)   
    (1024 * 768 pixels)        bit offset
                                    <--
     -->                       76543210
    -----------------         ----------
  | |               |         |11001001| 0 | byte offset
  v |  update area  |         |       1| 1 v
    |    -------    |         |        | 2
    |    |O|X|X|    |         |        | 3
    |    -------    |         |        | 4
    |    |O|X|X|    |         |        | ...
    |    -------    |         |        |
    |    |O|O|O|    |         ----------
    |    -------    |
    |               |
    -----------------
	
    - update area: 9 pixels
    - fill area == copy area: 4 pixels (fill area or copy area is the overlapped area between update area and window area.)
    - O (1): on (to update)
    - X (0): off (updated)
	- memory increasement direction: >, <, v, ^
    - k_createScreenBitmap fills bitmap corresponding to updaing area from the start of bitmap.
*/

// param: area: requested update area
bool k_createScreenBitmap(ScreenBitmap* bitmap, const Rect* area) {
	if (k_getOverlappedRect(&g_windowManager.screenArea, area, &bitmap->area) == false) {
		return false;
	}

	// Do not allocate bitmap memory whenever k_createScreenBitmap is called,
	// because it has two problems written below.
	// - problem 1: k_allocMem takes too long.
	// - problem 2: If free memory is not enough and memory allocation fails, screen can not be updated.
	// Thus, bitmap memory has already been allocated as much as screen-sized and shared.
	bitmap->bitmap = g_windowManager.screenBitmap;

	return k_fillScreenBitmap(bitmap, &bitmap->area, true);
}

// param: area: requested fill area
static bool k_fillScreenBitmap(const ScreenBitmap* bitmap, const Rect* area, bool on) {
	Rect fillArea;     // fill area
	int fillWidth;     // fill area width
	int fillHeight;    // fill area height
	int fillX;         // x of fill area coordinates
	int fillY;         // y of fill area coordinates
	int byteOffset;    // byte offset in bitmap
	int bitOffset;     // bit offset in byte
	int lastBitOffset; // last bit offset in byte
	int byteCount;     // byte count
	byte data;         // data to set bitmap
	int i;

	if (k_getOverlappedRect(&bitmap->area, area, &fillArea) == false) {
		return false;
	}

	fillWidth = k_getRectWidth(&fillArea);
	fillHeight = k_getRectHeight(&fillArea);

	/* fill area loop */
	for (fillY = 0; fillY < fillHeight; fillY++) {
		if (k_getStartOffsetInScreenBitmap(bitmap, fillArea.x1, fillArea.y1 + fillY, &byteOffset, &bitOffset) == false) {
			break;
		}

		for (fillX = 0; fillX < fillWidth; ) {
			/* set bitmap by byte-level */
			if ((bitOffset == 0) && ((fillWidth - fillX) >= 8)) {
				// [REF] remaining bit count in a line of fill area: fill width - fill x
				byteCount = (fillWidth - fillX) >> 3; // byte count = (fill width - fill x) / 8

				if (on == true) {
					k_memset(bitmap->bitmap + byteOffset, 0xFF, byteCount);

				} else {
					k_memset(bitmap->bitmap + byteOffset, 0x00, byteCount);
				}

				fillX += byteCount << 3; // fill x += byte count * 8
				byteOffset += byteCount;
				bitOffset = 0;

			/* set bitmap by bit-level */
			} else {
				lastBitOffset = MIN(8, bitOffset + fillWidth - fillX);

				data = 0;
				for (i = bitOffset; i < lastBitOffset; i++) {
					data |= 0x01 << i;
				}

				if (on == true) {
					bitmap->bitmap[byteOffset] |= data;

				} else {
					bitmap->bitmap[byteOffset] &= ~data;
				}

				fillX += lastBitOffset - bitOffset;
				byteOffset++;
				bitOffset = 0;
			}
		}
	}

	return true;
}

inline bool k_getStartOffsetInScreenBitmap(const ScreenBitmap* bitmap, int x, int y, int* byteOffset, int* bitOffset) {
	int updateX;     // x of update area coordinates
	int updateY;     // y of update area coordinates
	int updateWidth; // update area width 

	if (k_isPointInRect(&bitmap->area, x, y) == false) {
		return false;
	}

	// convert (x, y) from screen coordinates to update area coordinates.
	updateX = x - bitmap->area.x1;
	updateY = y - bitmap->area.y1;

	// get update area width.
	updateWidth = k_getRectWidth(&bitmap->area);

	// byte offset in bitmap = bit offset in bitmap / 8
	*byteOffset = (updateY * updateWidth + updateX) >> 3;

	// bit offset in byte = bit offset in bitmap % 8
	*bitOffset = (updateY * updateWidth + updateX) & 0x7;

	return true;
}

inline bool k_isScreenBitmapAllOff(const ScreenBitmap* bitmap) {
	int totalBitCount;  // total bit count of update area
	int totalByteCount; // total byte count of update area
	int remainBitCount;	// remain bit count of update area
	byte* data;         // data from bitmap
	int i;

	totalBitCount = k_getRectWidth(&bitmap->area) * k_getRectHeight(&bitmap->area);
	totalByteCount = totalBitCount >> 3; // total byte count = total bit count / 8

	/* compare bitmap by 8 bytes */
	data = bitmap->bitmap;
	for (i = 0; i < (totalByteCount >> 3); i++) { // loop while i < (total byte count / 8)
		if (*(qword*)data != 0) {
			return false;
		}

		data += 8;
	}

	/* compare remaining bitmap by 1 byte */
	for (i = 0; i < (totalByteCount & 0x7); i++) { // loop while i < (total byte count % 8)
		if (*data != 0) {
			return false;
		}

		data++;
	}

	/* compare remaining bitmap by 1 bit */
	remainBitCount = totalBitCount & 0x7; // remain bit count = total bit count % 8
	for (i = 0; i < remainBitCount; i++) {
		if (*data & (0x01 << i)) {
			return false;
		}
	}

	return true;
}

qword k_findWindowByPoint(int x, int y) {
	qword windowId;
	Window* window;

	// set background window ID to default window ID,
	// because mouse point is always inside the background window.
	windowId = g_windowManager.backgroundId;

	/**
	  loop from the first window (the top window),
	  and find the top window among windows which include mouse point.
	*/
	k_lock(&g_windowManager.mutex);

	window = k_getHeadFromList(&g_windowManager.windowList);

	while (window != null) {
		if ((window->flags & WINDOW_FLAGS_SHOW) && (k_isPointInRect(&window->area, x, y) == true)) {
			windowId = window->link.id;
			break;
		}

		window = k_getNextFromList(&g_windowManager.windowList, window);
	};

	k_unlock(&g_windowManager.mutex);

	return windowId;
}

qword k_findWindowByTitle(const char* title) {
	qword windowId;
	Window* window;
	int titleLen;

	// set invalid window ID to default window ID.
	windowId = WINDOW_INVALIDID;
	titleLen = k_strlen(title);

	/**
	  loop from the first window (the top window),
	  and find the window matching title.
	*/
	k_lock(&g_windowManager.mutex);

	window = k_getHeadFromList(&g_windowManager.windowList);
	while (window != null) {
		if ((k_strlen(window->title) == titleLen) && (k_memcmp(window->title, title, titleLen) == 0)) {
			windowId = window->link.id;
			break;
		}

		window = k_getNextFromList(&g_windowManager.windowList, window);
	}

	k_unlock(&g_windowManager.mutex);

	return windowId;
}

bool k_existWindow(qword windowId) {
	if (k_getWindow(windowId) == null) {
		return false;
	}

	return true;
}

qword k_getTopWindowId(void) {
	Window* top;
	qword topId;

	k_lock(&g_windowManager.mutex);

	top = k_getHeadFromList(&g_windowManager.windowList);
	if (top != null) {
		topId = top->link.id;

	} else {
		topId = WINDOW_INVALIDID;
	}

	k_unlock(&g_windowManager.mutex);

	return topId;
}

bool k_isTopWindow(qword windowId) {
	if (windowId == k_getTopWindowId()) {
		return true;
	}

	return false;
}

bool k_isTopWindowWithChild(qword windowId) {
	qword topId;
	Window* top;

	topId = k_getTopWindowId();
	if (topId == windowId) {
		return true;
	}

	top = k_getWindow(topId);
	if ((top != null) && (top->parentId != WINDOW_INVALIDID) && (top->parentId == windowId)) {
		return true;
	}

	return false;
}

bool k_moveWindowToTop(qword windowId) {
	Window* window;
	Rect area;
	dword flags;
	qword parentId;
	qword topId;
	Window* top;
	
	topId = k_getTopWindowId();

	if (topId == windowId) {
		return true;
	}

	/* move window to top */
	k_lock(&g_windowManager.mutex);

	window = k_removeListById(&g_windowManager.windowList, windowId);
	if (window != null) {
		k_addListToHead(&g_windowManager.windowList, window);
		k_convertRectScreenToWindow(windowId, &window->area, &area);
		flags = window->flags;
		parentId = window->parentId;
	}

	k_unlock(&g_windowManager.mutex);

	/* send window event and update screen */
	if (window != null) {
		k_sendWindowEventToWindow(windowId, EVENT_WINDOW_SELECT);
		if (flags & WINDOW_FLAGS_DRAWTITLEBAR) {
			k_updateWindowTitleBar(windowId, true);
			area.y1 += WINDOW_TITLEBAR_HEIGHT;
			k_updateScreenByWindowArea(windowId, &area);

		} else {
			k_updateScreenById(windowId);
		}

		k_sendWindowEventToWindow(topId, EVENT_WINDOW_DESELECT);
		top = k_getWindow(topId);
		if (!(((flags & WINDOW_FLAGS_CHILD) && (k_isTopWindowWithChild(parentId) == true)) ||
			  ((top != null) && (top->flags & WINDOW_FLAGS_CHILD) && (k_isTopWindowWithChild(top->parentId) == true)))) {
			k_updateWindowTitleBar(topId, false);
		}

		/* not show all child windows */
		if (flags & WINDOW_FLAGS_HASCHILD) {
			k_showChildWindows(windowId, false, 0, true);
		}

		return true;
	}

	return false;
}

bool k_isPointInTitleBar(qword windowId, int x, int y) {
	Window* window;

	window = k_getWindow(windowId);
	if ((window == null) || ((window->flags & WINDOW_FLAGS_DRAWTITLEBAR) != WINDOW_FLAGS_DRAWTITLEBAR)) {
		return false;
	}

	if ((window->area.x1 <= x) && (x <= window->area.x2) && (window->area.y1 <= y) && (y <= window->area.y1 + WINDOW_TITLEBAR_HEIGHT)) {
		return true;
	}

	return false;
}

bool k_isPointInCloseButton(qword windowId, int x, int y) {
	Window* window;

	window = k_getWindow(windowId);
	if ((window == null) || ((window->flags & WINDOW_FLAGS_DRAWTITLEBAR) != WINDOW_FLAGS_DRAWTITLEBAR)) {
		return false;
	}

	if ((window->area.x2 - 1 - WINDOW_XBUTTON_SIZE <= x) && (x <= window->area.x2 - 1) && (window->area.y1 + 1 <= y) && (y <= window->area.y1 + 1 + WINDOW_XBUTTON_SIZE)) {
		return true;
	}

	return false;
}

bool k_isPointInResizeButton(qword windowId, int x, int y) {
	Window* window;

	window = k_getWindow(windowId);
	if ((window == null) || ((window->flags & WINDOW_FLAGS_DRAWTITLEBAR) != WINDOW_FLAGS_DRAWTITLEBAR) || ((window->flags & WINDOW_FLAGS_RESIZABLE) != WINDOW_FLAGS_RESIZABLE)) {
		return false;
	}

	if ((window->area.x2 - 2 - (WINDOW_XBUTTON_SIZE * 2) <= x) && (x <= window->area.x2 - 2 - WINDOW_XBUTTON_SIZE) && (window->area.y1 + 1 <= y) && (y <= window->area.y1 + 1 + WINDOW_XBUTTON_SIZE)) {
		return true;
	}

	return false;
}

bool k_isPointInTopMenu(qword windowId, int x, int y) {
	Window* window;
	int menuOffsetX;
	int menuWidth;

	if (k_isTopWindowWithChild(windowId) == false) {
		return false;
	}

	window = k_getWindow(windowId);
	if ((window == null) || (window->topMenu == null)) {
		return false;
	}

	menuOffsetX = (FONT_DEFAULT_WIDTH * k_strlen(window->title)) + (MENU_ITEMWIDTHPADDING * 2);
	menuWidth = window->topMenu->table[window->topMenu->itemCount - 1].area.x2 - window->topMenu->table[0].area.x1 + 1;
	if ((window->area.x1 + menuOffsetX <= x) && (x <= window->area.x1 + menuOffsetX + menuWidth - 1) && (window->area.y1 <= y) && (y <= window->area.y1 + MENU_ITEMHEIGHT_TITLEBAR)) {
		return true;
	}

	return false;
}

static bool k_updateWindowTitleBar(qword windowId, bool selected) {
	Window* window;
	Rect titleBarArea;

	window = k_getWindow(windowId);

	if (window != null) {
		// [NOTE] Child window has no title bar.
		//        Thus, it gets parent window to update title bar.
		if (window->flags & WINDOW_FLAGS_CHILD) {
			window = k_getWindow(window->parentId);
		}

		if ((window->flags & WINDOW_FLAGS_HASCHILD) && (selected == false)) {
			k_showChildWindows(window->link.id, false, 0, false);
		}
	}

	if ((window != null) && (window->flags & WINDOW_FLAGS_DRAWTITLEBAR)) {
		k_drawWindowTitleBar(window->link.id, selected);
		k_setRect(&titleBarArea, 0, 0, k_getRectWidth(&window->area) - 1, WINDOW_TITLEBAR_HEIGHT - 1);
		k_updateScreenByWindowArea(window->link.id, &titleBarArea);
		return true;
	}

	return false;
}

bool k_updateCloseButton(qword windowId, bool mouseOver) {
	Window* window;
	int width;
	Rect buttonArea;

	window = k_getWindow(windowId);
	if ((window != null) && (window->flags & WINDOW_FLAGS_DRAWTITLEBAR)) {
		if (k_isTopWindowWithChild(window->link.id) == true) {
			k_drawCloseButton(window->link.id, true, mouseOver);

		} else {
			k_drawCloseButton(window->link.id, false, mouseOver);
		}

		width = k_getRectWidth(&window->area);
		k_setRect(&buttonArea, width - 1 - WINDOW_XBUTTON_SIZE, 1, width - 2, WINDOW_XBUTTON_SIZE - 1);
		k_updateScreenByWindowArea(window->link.id, &buttonArea);	
		return true;
	}

	return false;
}

bool k_updateResizeButton(qword windowId, bool mouseOver) {
	Window* window;
	int width;
	Rect buttonArea;

	window = k_getWindow(windowId);
	if ((window != null) && (window->flags & WINDOW_FLAGS_RESIZABLE)) {
		if (k_isTopWindowWithChild(window->link.id) == true) {
			k_drawResizeButton(window->link.id, true, mouseOver);

		} else {
			k_drawResizeButton(window->link.id, false, mouseOver);
		}

		width = k_getRectWidth(&window->area);
		k_setRect(&buttonArea, width - 2 - (WINDOW_XBUTTON_SIZE * 2), 1, width - 2 - WINDOW_XBUTTON_SIZE, WINDOW_XBUTTON_SIZE - 1);
		k_updateScreenByWindowArea(window->link.id, &buttonArea);	
		return true;
	}

	return false;
}

bool k_moveWindow(qword windowId, int x, int y) {
	Window* window;
	Rect prevArea;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	/* change window area */
	k_memcpy(&prevArea, &window->area, sizeof(Rect));
	window->area.x1 = x;
	window->area.y1 = y;
	window->area.x2 = x + k_getRectWidth(&prevArea) - 1;
	window->area.y2 = y + k_getRectHeight(&prevArea) - 1;

	k_unlock(&window->mutex);

	/* move child windows */
	if (window->flags & WINDOW_FLAGS_HASCHILD) {
		k_moveChildWindows(windowId, x - prevArea.x1, y - prevArea.y1);
	}

	/* update screen and send window event */
	k_updateScreenByScreenArea(&prevArea);
	k_updateScreenById(windowId);
	k_sendWindowEventToWindow(windowId, EVENT_WINDOW_MOVE);

	return true;
}

bool k_resizeWindow(qword windowId, int x, int y, int width, int height) {
	Window* window;
	Color* newBuffer;
	Color* oldBuffer;
	Rect prevArea;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	if (window->flags & WINDOW_FLAGS_DRAWTITLEBAR) {
		if (width < WINDOW_MINWIDTH) {
			width = WINDOW_MINWIDTH;
		}

		if (height < WINDOW_MINHEIGHT) {
			height = WINDOW_MINHEIGHT;
		}
	}

	/* change window buffer */
	newBuffer = (Color*)k_allocMem(sizeof(Color) * width * height);
	if (newBuffer == null) {
		k_unlock(&window->mutex);
		return false;
	}

	oldBuffer = window->buffer;
	window->buffer = newBuffer;
	k_freeMem(oldBuffer);

	/* change window area */
	k_memcpy(&prevArea, &window->area, sizeof(Rect));
	window->area.x1 = x;
	window->area.y1 = y;
	window->area.x2 = x + width - 1;
	window->area.y2 = y + height - 1;

	k_unlock(&window->mutex);

	/* move child windows */
	if (window->flags & WINDOW_FLAGS_HASCHILD) {
		k_moveChildWindows(windowId, x - prevArea.x1, y - prevArea.y1);
	}

	/** 
	  draw window basic objects
	  - GUI task have to draw window contents when it receives EVENT_WINDOW_RESIZE event from window manger task. 
	*/
	k_drawWindowBackground(windowId);

	if (window->flags & WINDOW_FLAGS_DRAWFRAME) {
		k_drawWindowFrame(windowId);
	}

	if (window->flags & WINDOW_FLAGS_DRAWTITLEBAR) {
		k_drawWindowTitleBar(windowId, true);
	}

	/* update screen and send window event */
	if (window->flags & WINDOW_FLAGS_SHOW) {
		k_updateScreenByScreenArea(&prevArea);
		k_updateScreenById(windowId);
		k_sendWindowEventToWindow(windowId, EVENT_WINDOW_RESIZE);
	}

	return true;
}

void k_moveChildWindows(qword windowId, int moveX, int moveY) {
	Window* window;
	void* childLink;
	Window* child;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return;
	}

	childLink = k_getHeadFromList(&window->childList);
	while (childLink != null) {
		child = GETWINDOWFROMCHILDLINK(childLink);
		k_moveWindow(child->link.id, child->area.x1 + moveX, child->area.y1 + moveY);
		childLink = k_getNextFromList(&window->childList, childLink);
	}

	k_unlock(&window->mutex);
}

void k_showChildWindows(qword windowId, bool show, dword flags, bool parentToTop) {
	Window* window;
	void* childLink;
	Window* child;
	bool childShow;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return;
	}

	childLink = k_getHeadFromList(&window->childList);
	while (childLink != null) {
		child = GETWINDOWFROMCHILDLINK(childLink);

		if ((child->flags & WINDOW_FLAGS_VISIBLE) && (show == false) && (window->flags & WINDOW_FLAGS_SHOW)) {
			if (parentToTop == true) {
				k_moveWindowToTop(child->link.id);
			}
			
			childLink = k_getNextFromList(&window->childList, childLink);
			continue;
		}

		// If flags == 0, process all child windows.
		// If flags == WINDOW_FLAGS_MENU, process only menu child windows.
		if ((flags == 0) || (child->flags & flags)) {
			if (child->flags & WINDOW_FLAGS_SHOW) {
				childShow = true;

			} else {
				childShow = false;
			}

			if (childShow != show) {
				k_showWindow(child->link.id, show);	
			}

		}

		childLink = k_getNextFromList(&window->childList, childLink);
	}

	k_unlock(&window->mutex);
}

void k_deleteChildWindows(qword windowId) {
	Window* window;
	int count;
	int i;
	void* childLink;
	Window* child;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return;
	}

	count = k_getListCount(&window->childList);
	for (i = 0; i < count; i++) {
		childLink = k_removeListFromHead(&window->childList);
		if (childLink == null) {
			break;
		}

		child = GETWINDOWFROMCHILDLINK(childLink);
		k_deleteWindow(child->link.id);
	}

	k_unlock(&window->mutex);
}

void* k_getNthChildWidget(qword windowId, int n) {
	Window* window;
	void* childLink;
	Window* child;
	int count = 0;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return null;
	}

	childLink = k_getHeadFromList(&window->childList);
	while (childLink != null) {
		child = GETWINDOWFROMCHILDLINK(childLink);
		if (count == n) {
			k_unlock(&window->mutex);
			return child->widget;
		}

		childLink = k_getNextFromList(&window->childList, childLink);
		count++;
	}

	k_unlock(&window->mutex);

	return null;	
}

bool k_getWindowArea(qword windowId, Rect* area) {
	Window* window;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	k_memcpy(area, &window->area, sizeof(Rect));

	k_unlock(&window->mutex);

	return true;
}

bool k_convertPointScreenToWindow(qword windowId, const Point* screenPoint, Point* windowPoint) {
	Rect area;

	if (k_getWindowArea(windowId, &area) == false) {
		return false;
	}

	windowPoint->x = screenPoint->x - area.x1;
	windowPoint->y = screenPoint->y - area.y1;

	return true;
}

bool k_convertPointWindowToScreen(qword windowId, const Point* windowPoint, Point* screenPoint) {
	Rect area;

	if (k_getWindowArea(windowId, &area) == false) {
		return false;
	}

	screenPoint->x = windowPoint->x + area.x1;
	screenPoint->y = windowPoint->y + area.y1;

	return true;
}

bool k_convertRectScreenToWindow(qword windowId, const Rect* screenRect, Rect* windowRect) {
	Rect area;

	if (k_getWindowArea(windowId, &area) == false) {
		return false;
	}

	windowRect->x1 = screenRect->x1 - area.x1;
	windowRect->y1 = screenRect->y1 - area.y1;
	windowRect->x2 = screenRect->x2 - area.x1;
	windowRect->y2 = screenRect->y2 - area.y1;

	return true;
}

bool k_convertRectWindowToScreen(qword windowId, const Rect* windowRect, Rect* screenRect) {
	Rect area;

	if (k_getWindowArea(windowId, &area) == false) {
		return false;
	}

	screenRect->x1 = windowRect->x1 + area.x1;
	screenRect->y1 = windowRect->y1 + area.y1;
	screenRect->x2 = windowRect->x2 + area.x1;
	screenRect->y2 = windowRect->y2 + area.y1;

	return true;	
}

bool k_setMouseEvent(Event* event, qword windowId, qword eventType, int mouseX, int mouseY, byte buttonStatus) {
	Point screenMousePoint;
	Point windowMousePoint;

	switch (eventType) {
	case EVENT_MOUSE_MOVE:
	case EVENT_MOUSE_LBUTTONDOWN:
	case EVENT_MOUSE_LBUTTONUP:
	case EVENT_MOUSE_RBUTTONDOWN:
	case EVENT_MOUSE_RBUTTONUP:
	case EVENT_MOUSE_MBUTTONDOWN:
	case EVENT_MOUSE_MBUTTONUP:
	case EVENT_MOUSE_OUT:
		screenMousePoint.x = mouseX;
		screenMousePoint.y = mouseY;

		if (k_convertPointScreenToWindow(windowId, &screenMousePoint, &windowMousePoint) == false) {
			return false;
		}

		event->type = eventType;
		event->mouseEvent.windowId = windowId;
		k_memcpy(&event->mouseEvent.point, &windowMousePoint, sizeof(Point));
		event->mouseEvent.buttonStatus = buttonStatus;

		break;

	default:
		return false;
	}

	return true;
}

bool k_setWindowEvent(Event* event, qword windowId, qword eventType) {
	Rect area;

	switch (eventType) {
	case  EVENT_WINDOW_SELECT:
	case  EVENT_WINDOW_DESELECT:
	case  EVENT_WINDOW_MOVE:
	case  EVENT_WINDOW_RESIZE:
	case  EVENT_WINDOW_CLOSE:
		if (k_getWindowArea(windowId, &area) == false) {
			return false;
		}

		event->type = eventType;
		event->windowEvent.windowId = windowId;
		k_memcpy(&event->windowEvent.area, &area, sizeof(Rect));

		break;

	default:
		return false;
	}

	return true;
}

void k_setKeyEvent(Event* event, qword windowId, const Key* key) {
	if (key->flags & KEY_FLAGS_DOWN) {
		event->type = EVENT_KEY_DOWN;

	} else {
		event->type = EVENT_KEY_UP;
	}

	event->keyEvent.windowId = windowId;
	event->keyEvent.scanCode = key->scanCode;
	event->keyEvent.asciiCode = key->asciiCode;
	event->keyEvent.flags = key->flags;
}

bool k_setMenuEvent(Event* event, qword windowId, qword eventType, int mouseX, int mouseY) {
	Menu* topMenu;
	int index;

	switch (eventType) {
	case EVENT_TOPMENU_CLICK:
		index = k_getTopMenuItemIndex(windowId, mouseX, mouseY);
		if (index == -1) {
			break;
		}

		event->type = eventType;
		event->menuEvent.windowId = windowId;
		event->menuEvent.index = index;

		topMenu = k_getWindow(windowId)->topMenu;
		if (topMenu->table[index].hasSubMenu == true) {
			k_toggleMenuVisibility((Menu*)topMenu->table[index].param, topMenu, index);

		} else {
			k_drawMenuItem(topMenu, index, false, topMenu->textColor, topMenu->backgroundColor, topMenu->activeColor);
		}

		break;

	default:
		return false;
	}

	return true;
}

bool k_sendEventToWindow(const Event* event, qword windowId) {
	Window* window;
	bool result;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	result = k_putQueue(&window->eventQueue, event);

	k_unlock(&window->mutex);

	return result;
}

bool k_recvEventFromWindow(Event* event, qword windowId) {
	Window* window;
	bool result;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	result = k_getQueue(&window->eventQueue, event, &window->mutex);

	k_unlock(&window->mutex);

	return result;
}

bool k_sendEventToWindowManager(const Event* event) {
	bool result;

	k_lock(&g_windowManager.mutex);

	result = k_putQueue(&g_windowManager.eventQueue, event);

	k_unlock(&g_windowManager.mutex);

	return result;
}

bool k_recvEventFromWindowManager(Event* event) {
	bool result;

	if (g_windowManager.eventQueue.blocking == false && k_isQueueEmpty(&g_windowManager.eventQueue) == true) {
		return false;
	}

	k_lock(&g_windowManager.mutex);

	result = k_getQueue(&g_windowManager.eventQueue, event, &g_windowManager.mutex);

	k_unlock(&g_windowManager.mutex);

	return result;	
}

inline bool k_sendMouseEventToWindow(qword windowId, qword eventType, int mouseX, int mouseY, byte buttonStatus) {
	Event event;

	if (k_setMouseEvent(&event, windowId, eventType, mouseX, mouseY, buttonStatus) == false) {
		return false;
	}

	return k_sendEventToWindow(&event, windowId);
}

inline bool k_sendWindowEventToWindow(qword windowId, qword eventType) {
	Event event;

	if (k_setWindowEvent(&event, windowId, eventType) == false) {
		return false;
	}

	return k_sendEventToWindow(&event, windowId);	
}

inline bool k_sendKeyEventToWindow(qword windowId, const Key* key) {
	Event event;

	k_setKeyEvent(&event, windowId, key);

	return k_sendEventToWindow(&event, windowId);
}

inline bool k_sendMenuEventToWindow(qword windowId, qword eventType, int mouseX, int mouseY) {
	Event event;

	if (k_setMenuEvent(&event, windowId, eventType, mouseX, mouseY) == false) {
		return false;
	}

	return k_sendEventToWindow(&event, windowId);
}

bool k_updateScreenById(qword windowId) {
	Event event;
	Window* window;

	window = k_getWindow(windowId);
	if ((window == null) || ((window->flags & WINDOW_FLAGS_SHOW) != WINDOW_FLAGS_SHOW)) {
		return false;
	}

	/* set screen update event */
	event.type = EVENT_SCREENUPDATE_BYID;
	event.screenUpdateEvent.windowId = windowId;

	/* send screen update event */
	return k_sendEventToWindowManager(&event);
}

bool k_updateScreenByWindowArea(qword windowId, const Rect* area) {
	Event event;
	Window* window;

	window = k_getWindow(windowId);
	if ((window == null) || ((window->flags & WINDOW_FLAGS_SHOW) != WINDOW_FLAGS_SHOW)) {
		return false;
	}

	/* set screen update event */
	event.type = EVENT_SCREENUPDATE_BYWINDOWAREA;
	event.screenUpdateEvent.windowId = windowId;
	k_memcpy(&event.screenUpdateEvent.area, area, sizeof(Rect));

	/* send screen update event */
	return k_sendEventToWindowManager(&event);
}

bool k_updateScreenByScreenArea(const Rect* area) {
	Event event;

	/* set screen update event */
	event.type = EVENT_SCREENUPDATE_BYSCREENAREA;
	event.screenUpdateEvent.windowId = WINDOW_INVALIDID;
	k_memcpy(&event.screenUpdateEvent.area, area, sizeof(Rect));

	/* send screen update event */
	return k_sendEventToWindowManager(&event);
}

bool k_drawWindowBackground(qword windowId) {
	Window* window;
	int width;
	int height;	
	Rect area;
	int x = 0;
	int y = 0;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// get width, height and set clipping area on window coordinates.
	width = k_getRectWidth(&window->area);
	height = k_getRectHeight(&window->area);
	k_setRect(&area, 0, 0, width - 1, height - 1);

	if (window->flags & WINDOW_FLAGS_DRAWFRAME) {
		x = 2; // window frame thick (2 pixels)
		y = 2; // window frame thick (2 pixels)
	}

	if (window->flags & WINDOW_FLAGS_DRAWTITLEBAR) {
		y = WINDOW_TITLEBAR_HEIGHT;
	}

	// draw a window background.
	__k_drawRect(window->buffer, &area, x, y, width - 1 - x, height - 1 - x, window->backgroundColor, true);

	k_unlock(&window->mutex);
}

bool k_drawWindowFrame(qword windowId) {
	Window* window;
	int width;
	int height;	
	Rect area;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// get width, height and set clipping area on window coordinates.
	width = k_getRectWidth(&window->area);
	height = k_getRectHeight(&window->area);
	k_setRect(&area, 0, 0, width - 1, height - 1);

	// draw a window frame (2 pixes-thick).
	__k_drawRect(window->buffer, &area, 0, 0, width - 1, height - 1, WINDOW_COLOR_FRAME, false);
	__k_drawRect(window->buffer, &area, 1, 1, width - 2, height - 2, WINDOW_COLOR_FRAME, false);

	k_unlock(&window->mutex);

	return true;
}

bool k_drawWindowTitleBar(qword windowId, bool selected) {
	Window* window;
	int width;
	Rect area;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// get width and set clipping area on window coordinates.
	width = k_getRectWidth(&window->area);
	k_setRect(&area, 0, 0, width - 1, k_getRectHeight(&window->area) - 1);

	/* draw a title bar */

	if (selected == true) {
		// [NOTE] The title bar includes the frame.
		// draw a title bar background.
		__k_drawRect(window->buffer, &area, 0, 0, width - 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBACKGROUNDACTIVE, true);
		
		// draw a title.
		if (window->topMenu == null) {
			__k_drawText(window->buffer, &area, MENU_ITEMWIDTHPADDING, 2, WINDOW_COLOR_TITLEBARTEXTACTIVE, WINDOW_COLOR_TITLEBARBACKGROUNDACTIVE, window->title, k_strlen(window->title));

		} else {
			__k_drawText(window->buffer, &area, MENU_ITEMWIDTHPADDING, 2, WINDOW_COLOR_TITLEBARTEXTACTIVEWITHMENU, WINDOW_COLOR_TITLEBARBACKGROUNDACTIVE, window->title, k_strlen(window->title));
		}
		

	} else {
		// draw a title bar background.
		__k_drawRect(window->buffer, &area, 0, 0, width - 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBACKGROUNDINACTIVE, true);
		
		// draw a title.
		__k_drawText(window->buffer, &area, MENU_ITEMWIDTHPADDING, 2, WINDOW_COLOR_TITLEBARTEXTINACTIVE, WINDOW_COLOR_TITLEBARBACKGROUNDINACTIVE, window->title, k_strlen(window->title));
	}

	#if 0
	// draw top lines (2 pixels-thick) of the title bar with bright color in order to make it 3-dimensional.
	__k_drawLine(window->buffer, &area, 1, 1, width - 1, 1, WINDOW_COLOR_TITLEBARBRIGHT1);
	__k_drawLine(window->buffer, &area, 1, 2, width - 1, 2, WINDOW_COLOR_TITLEBARBRIGHT2);

	// draw left lines (2 pixels-thick) of the title bar with bright color in order to make it 3-dimensional.
	__k_drawLine(window->buffer, &area, 1, 2, 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBRIGHT1);
	__k_drawLine(window->buffer, &area, 2, 2, 2, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBRIGHT2);

	// draw bottom lines (2 pixels-thick) of the title bar with dark color in order to make it 3-dimensional.
	__k_drawLine(window->buffer, &area, 2, WINDOW_TITLEBAR_HEIGHT - 2, width - 2, WINDOW_TITLEBAR_HEIGHT - 2, WINDOW_COLOR_TITLEBARDARK);
	__k_drawLine(window->buffer, &area, 2, WINDOW_TITLEBAR_HEIGHT - 1, width - 2, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARDARK);
	#endif

	k_unlock(&window->mutex);

	/* draw all menu items */
	if (window->topMenu != null) {
		if (selected == true) {
			k_drawAllMenuItems(window->topMenu, window->topMenu->textColor, window->topMenu->backgroundColor, window->topMenu->activeColor);

		} else {
			k_drawAllMenuItems(window->topMenu, window->topMenu->textColor, WINDOW_COLOR_TITLEBARBACKGROUNDINACTIVE, window->topMenu->activeColor);
		}
	}

	/* draw a close button */
	k_drawCloseButton(windowId, selected, false);

	/* draw a resize button */
	if (window->flags & WINDOW_FLAGS_RESIZABLE) {
		k_drawResizeButton(windowId, selected, false);
	}

	return true;
}

static bool k_drawCloseButton(qword windowId, bool selected, bool mouseOver) {
	Window* window;
	int width;
	Rect area;
	Rect buttonArea;
	Color markColor;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// get width and set clipping area on window coordinates.
	width = k_getRectWidth(&window->area);
	k_setRect(&area, 0, 0, width - 1, k_getRectHeight(&window->area) - 1);

	k_unlock(&window->mutex);

	// draw a close button.
	k_setRect(&buttonArea, width - 1 - WINDOW_XBUTTON_SIZE, 1, width - 2, WINDOW_XBUTTON_SIZE - 1);

	if (selected == true) {
		if (mouseOver == true) {
			k_drawButton(windowId, &buttonArea, WINDOW_COLOR_XBUTTONBACKGROUNDACTIVE, WINDOW_COLOR_XBUTTONBACKGROUNDACTIVE, "", BUTTON_FLAGS_SHADOW);
			markColor = WINDOW_COLOR_XBUTTONMARKACTIVE;

		} else {
			k_drawButton(windowId, &buttonArea, WINDOW_COLOR_XBUTTONBACKGROUNDACTIVE, WINDOW_COLOR_XBUTTONBACKGROUNDACTIVE, "", 0);
			markColor = WINDOW_COLOR_XBUTTONMARKINACTIVE;
		}
		
	} else {
		if (mouseOver == true) {
			k_drawButton(windowId, &buttonArea, WINDOW_COLOR_XBUTTONBACKGROUNDINACTIVE, WINDOW_COLOR_XBUTTONBACKGROUNDINACTIVE, "", BUTTON_FLAGS_SHADOW);
			markColor = WINDOW_COLOR_XBUTTONMARKACTIVE;

		} else {
			k_drawButton(windowId, &buttonArea, WINDOW_COLOR_XBUTTONBACKGROUNDINACTIVE, WINDOW_COLOR_XBUTTONBACKGROUNDINACTIVE, "", 0);
			markColor = WINDOW_COLOR_XBUTTONMARKINACTIVE;
		}
	}

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// draw 'X' mark on the close button.
	// draw '\' mark (3 pixels-thick).
	__k_drawLine(window->buffer, &area, width - 2 - 18 + 4, 1 + 4, width - 2 - 4, WINDOW_TITLEBAR_HEIGHT - 6, markColor);
	__k_drawLine(window->buffer, &area, width - 2 - 18 + 5, 1 + 4, width - 2 - 4, WINDOW_TITLEBAR_HEIGHT - 7, markColor);
	__k_drawLine(window->buffer, &area, width - 2 - 18 + 4, 1 + 5, width - 2 - 5, WINDOW_TITLEBAR_HEIGHT - 6, markColor);
	
	// draw '/' mark (3 pixels-thick).
	__k_drawLine(window->buffer, &area, width - 2 - 18 + 4, 19 - 4, width - 2 - 4, 1 + 4, markColor);
	__k_drawLine(window->buffer, &area, width - 2 - 18 + 5, 19 - 4, width - 2 - 4, 1 + 5, markColor);
	__k_drawLine(window->buffer, &area, width - 2 - 18 + 4, 19 - 5, width - 2 - 5, 1 + 4, markColor);

	k_unlock(&window->mutex);

	return true;
}

static bool k_drawResizeButton(qword windowId, bool selected, bool mouseOver) {
	Window* window;
	int width;
	Rect area;
	Rect buttonArea;
	Color markColor;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// get width and set clipping area on window coordinates.
	width = k_getRectWidth(&window->area);
	k_setRect(&area, 0, 0, width - 1, k_getRectHeight(&window->area) - 1);

	k_unlock(&window->mutex);

	// draw a resize button.
	k_setRect(&buttonArea, width - 2 - (WINDOW_XBUTTON_SIZE * 2), 1, width - 2 - WINDOW_XBUTTON_SIZE, WINDOW_XBUTTON_SIZE - 1);

	if (selected == true) {
		if (mouseOver == true) {
			k_drawButton(windowId, &buttonArea, WINDOW_COLOR_XBUTTONBACKGROUNDACTIVE, WINDOW_COLOR_XBUTTONBACKGROUNDACTIVE, "", BUTTON_FLAGS_SHADOW);
			markColor = WINDOW_COLOR_XBUTTONMARKACTIVE;

		} else {
			k_drawButton(windowId, &buttonArea, WINDOW_COLOR_XBUTTONBACKGROUNDACTIVE, WINDOW_COLOR_XBUTTONBACKGROUNDACTIVE, "", 0);
			markColor = WINDOW_COLOR_XBUTTONMARKINACTIVE;
		}
		

	} else {
		if (mouseOver == true) {
			k_drawButton(windowId, &buttonArea, WINDOW_COLOR_XBUTTONBACKGROUNDINACTIVE, WINDOW_COLOR_XBUTTONBACKGROUNDINACTIVE, "", BUTTON_FLAGS_SHADOW);
			markColor = WINDOW_COLOR_XBUTTONMARKACTIVE;

		} else {
			k_drawButton(windowId, &buttonArea, WINDOW_COLOR_XBUTTONBACKGROUNDINACTIVE, WINDOW_COLOR_XBUTTONBACKGROUNDINACTIVE, "", 0);
			markColor = WINDOW_COLOR_XBUTTONMARKINACTIVE;
		}
	}
	
	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// draw '<->' mark on the resize button.
	// draw '/' mark (3 pixels-thick).
	__k_drawLine(window->buffer, &area, buttonArea.x1 + 4, buttonArea.y2 - 4, buttonArea.x2 - 5, buttonArea.y1 + 3, markColor);
	__k_drawLine(window->buffer, &area, buttonArea.x1 + 4, buttonArea.y2 - 3, buttonArea.x2 - 4, buttonArea.y1 + 3, markColor);
	__k_drawLine(window->buffer, &area, buttonArea.x1 + 5, buttonArea.y2 - 3, buttonArea.x2 - 4, buttonArea.y1 + 4, markColor);

	// draw '>' mark (2 pixels-thick).
	__k_drawLine(window->buffer, &area, buttonArea.x1 + 9, buttonArea.y1 + 3, buttonArea.x2 - 4, buttonArea.y1 + 3, markColor);
	__k_drawLine(window->buffer, &area, buttonArea.x1 + 9, buttonArea.y1 + 4, buttonArea.x2 - 4, buttonArea.y1 + 4, markColor);
	__k_drawLine(window->buffer, &area, buttonArea.x2 - 4, buttonArea.y1 + 5, buttonArea.x2 - 4, buttonArea.y1 + 9, markColor);
	__k_drawLine(window->buffer, &area, buttonArea.x2 - 5, buttonArea.y1 + 5, buttonArea.x2 - 5, buttonArea.y1 + 9, markColor);

	// draw '<' mark (2 pixels-thick).
	__k_drawLine(window->buffer, &area, buttonArea.x1 + 4, buttonArea.y1 + 8, buttonArea.x1 + 4, buttonArea.y2 - 3, markColor);
	__k_drawLine(window->buffer, &area, buttonArea.x1 + 5, buttonArea.y1 + 8, buttonArea.x1 + 5, buttonArea.y2 - 3, markColor);
	__k_drawLine(window->buffer, &area, buttonArea.x1 + 6, buttonArea.y2 - 4, buttonArea.x1 + 10, buttonArea.y2 - 4, markColor);
	__k_drawLine(window->buffer, &area, buttonArea.x1 + 6, buttonArea.y2 - 3, buttonArea.x1 + 10, buttonArea.y2 - 3, markColor);

	k_unlock(&window->mutex);

	return true;
}

bool k_drawPixel(qword windowId, int x, int y, Color color) {
	Window* window;
	Rect area;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// set clipping area on window coordinates.
	k_setRect(&area, 0, 0, window->area.x2 - window->area.x1, window->area.y2 - window->area.y1);

	// draw a pixel.
	__k_drawPixel(window->buffer, &area, x, y, color);
	
	k_unlock(&window->mutex);

	return true;
}

bool k_drawLine(qword windowId, int x1, int y1, int x2, int y2, Color color) {
	Window* window;
	Rect area;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// set clipping area on window coordinates.
	k_setRect(&area, 0, 0, window->area.x2 - window->area.x1, window->area.y2 - window->area.y1);

	// draw a line.
	__k_drawLine(window->buffer, &area, x1, y1, x2, y2, color);
	
	k_unlock(&window->mutex);

	return true;
}

bool k_drawRect(qword windowId, int x1, int y1, int x2, int y2, Color color, bool fill) {
	Window* window;
	Rect area;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// set clipping area on window coordinates.
	k_setRect(&area, 0, 0, window->area.x2 - window->area.x1, window->area.y2 - window->area.y1);

	// draw a rectangle.
	__k_drawRect(window->buffer, &area, x1, y1, x2, y2, color, fill);
	
	k_unlock(&window->mutex);

	return true;
}

bool k_drawCircle(qword windowId, int x, int y, int radius, Color color, bool fill) {
	Window* window;
	Rect area;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// set clipping area on window coordinates.
	k_setRect(&area, 0, 0, window->area.x2 - window->area.x1, window->area.y2 - window->area.y1);

	// draw a circle.
	__k_drawCircle(window->buffer, &area, x, y, radius, color, fill);
	
	k_unlock(&window->mutex);

	return true;
}

bool k_drawText(qword windowId, int x, int y, Color textColor, Color backgroundColor, const char* str, int len) {
	Window* window;
	Rect area;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// set clipping area on window coordinates.
	k_setRect(&area, 0, 0, window->area.x2 - window->area.x1, window->area.y2 - window->area.y1);

	// draw a text.
	__k_drawText(window->buffer, &area, x, y, textColor, backgroundColor, str, len);
	
	k_unlock(&window->mutex);

	return true;
}

bool k_bitblt(qword windowId, int x, int y, const Color* buffer, int width, int height) {
	Window* window;
	Rect windowArea;
	Rect bufferArea;
	Rect overArea;
	int windowWidth;
	int overWidth, overHeight;
	int startX, startY;
	int i;
	int windowOffset;
	int bufferOffset;
	
	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// convert window area from screen coordinates to window coordinates.
	k_setRect(&windowArea, 0, 0, window->area.x2 - window->area.x1, window->area.y2 - window->area.y1);
	k_setRect(&bufferArea, x, y, x + width - 1, y + height - 1);
	if (k_getOverlappedRect(&windowArea, &bufferArea, &overArea) == false) {
		k_unlock(&window->mutex);
		return false;
	}

	windowWidth = k_getRectWidth(&windowArea);
	overWidth = k_getRectWidth(&overArea);
	overHeight = k_getRectHeight(&overArea);

	// process clipping: If x < 0 or y < 0, buffer to copy is clipped.
	if (x < 0) {
		startX = x;

	} else {
		startX = 0;
	}

	if (y < 0) {
		startY = y;

	} else {
		startY = 0;
	}

	for (i = 0; i < overHeight; i++) {
		windowOffset = windowWidth * (overArea.y1 + i) + overArea.x1;
		bufferOffset = width * (startY + i) + startX;
		k_memcpy(window->buffer + windowOffset, buffer + bufferOffset, sizeof(Color) * overWidth);
	}

	k_unlock(&window->mutex);

	return true;
}

static void k_drawBackgroundImage(void) {
	Jpeg* jpeg;
	Color* imageBuffer;
		
	jpeg = (Jpeg*)k_allocMem(sizeof(Jpeg));
	if (jpeg == null) {
		return;
	}

	if (k_initJpeg(jpeg, IMAGE_WALLPAPER, IMAGE_WALLPAPER_SIZE) == false) {
		k_freeMem(jpeg);
		return;
	}

	imageBuffer = (Color*)k_allocMem(sizeof(Color) * jpeg->width * jpeg->height);
	if (imageBuffer == null) {
		k_freeMem(jpeg);
		return;
	}

	if (k_decodeJpeg(jpeg, imageBuffer) == false) {
		k_freeMem(imageBuffer);
		k_freeMem(jpeg);
		return;
	}

	k_bitblt(g_windowManager.backgroundId, g_windowManager.screenArea.x1, g_windowManager.screenArea.y1, imageBuffer, jpeg->width, jpeg->height);

	k_freeMem(imageBuffer);
	k_freeMem(jpeg);
}

// mouse cursor bitmap (10 * 18 = 180 bytes)
// - A byte in bitmap represents a color (16 bits) in video memory or a pixel (16 bits) in screen.
static byte g_mouseCursorBitmap[MOUSE_CURSOR_WIDTH * MOUSE_CURSOR_HEIGHT] = {
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 1, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 1, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 1, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 1, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 1, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 1,
	1, 2, 2, 2, 2, 2, 1, 1, 1, 1,
	1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	1, 2, 2, 1, 2, 2, 2, 1, 0, 0,
	1, 2, 1, 0, 1, 2, 2, 1, 0, 0,
	1, 1, 0, 0, 1, 2, 2, 2, 1, 0,
	1, 0, 0, 0, 0, 1, 2, 2, 1, 0,
	0, 0, 0, 0, 0, 1, 2, 2, 1, 0,
	0, 0, 0, 0, 0, 0, 1, 1, 0, 0
};

static void k_drawMouseCursor(int x, int y) {
	int i, j;
	byte* currentPos;

	currentPos = g_mouseCursorBitmap;

	for (i = 0; i < MOUSE_CURSOR_HEIGHT; i++) {
		for (j = 0; j < MOUSE_CURSOR_WIDTH; j++) {
			switch (*currentPos) {
			case 0: // background
				break;

			case 1: // outer
				__k_drawPixel(g_windowManager.videoMem, &g_windowManager.screenArea, x + j, y + i, MOUSE_CURSOR_COLOR_OUTER);
				break;

			case 2: // inner
				__k_drawPixel(g_windowManager.videoMem, &g_windowManager.screenArea, x + j, y + i, MOUSE_CURSOR_COLOR_INNER);
				break;
			}

			currentPos++;
		}
	}
}

void k_moveMouseCursor(int x, int y) {
	Rect prevArea;

	// keep mouse position always inside screen.
	if (x < g_windowManager.screenArea.x1) {
		x = g_windowManager.screenArea.x1;

	} else if (x > g_windowManager.screenArea.x2) {
		x = g_windowManager.screenArea.x2;
	}

	if (y < g_windowManager.screenArea.y1) {
		y = g_windowManager.screenArea.y1;

	} else if (y > g_windowManager.screenArea.y2) {
		y = g_windowManager.screenArea.y2;
	}

	k_lock(&g_windowManager.mutex);

	// back up previous mouse position.
	k_setRect(&prevArea, g_windowManager.mouseX, g_windowManager.mouseY, g_windowManager.mouseX + MOUSE_CURSOR_WIDTH - 1, g_windowManager.mouseY + MOUSE_CURSOR_HEIGHT - 1);

	// save current mouse position.
	g_windowManager.mouseX = x;
	g_windowManager.mouseY = y;

	k_unlock(&g_windowManager.mutex);

	// redraw window by previous mouse area (clear previous mouse cursor).
	k_redrawWindowByArea(WINDOW_INVALIDID, &prevArea);

	// draw mouse cursor at current mouse position.
	k_drawMouseCursor(x, y);
}

void k_getMouseCursorPos(int* x, int* y) {
	*x = g_windowManager.mouseX;
	*y = g_windowManager.mouseY;
}

void k_drawResizeMarker(const Rect* area, bool show) {
	Rect markerArea;

	/* draw resize marker */
	if (show == true) {
		// draw left-top marker.
		k_setRect(&markerArea, area->x1, area->y1, area->x1 + RESIZEMARKER_SIZE, area->y1 + RESIZEMARKER_SIZE);
		__k_drawRect(g_windowManager.videoMem, &g_windowManager.screenArea, markerArea.x1, markerArea.y1, markerArea.x2, markerArea.y1 + RESIZEMARKER_THICK, RESIZEMARKER_COLOR, true);
		__k_drawRect(g_windowManager.videoMem, &g_windowManager.screenArea, markerArea.x1, markerArea.y1, markerArea.x1 + RESIZEMARKER_THICK, markerArea.y2, RESIZEMARKER_COLOR, true);

		// draw right-top marker.
		k_setRect(&markerArea, area->x2 - RESIZEMARKER_SIZE, area->y1, area->x2, area->y1 + RESIZEMARKER_SIZE);
		__k_drawRect(g_windowManager.videoMem, &g_windowManager.screenArea, markerArea.x1, markerArea.y1, markerArea.x2, markerArea.y1 + RESIZEMARKER_THICK, RESIZEMARKER_COLOR, true);
		__k_drawRect(g_windowManager.videoMem, &g_windowManager.screenArea, markerArea.x2 - RESIZEMARKER_THICK, markerArea.y1, markerArea.x2, markerArea.y2, RESIZEMARKER_COLOR, true);

		// draw left-bottom marker.
		k_setRect(&markerArea, area->x1, area->y2 - RESIZEMARKER_SIZE, area->x1 + RESIZEMARKER_SIZE, area->y2);
		__k_drawRect(g_windowManager.videoMem, &g_windowManager.screenArea, markerArea.x1, markerArea.y2 - RESIZEMARKER_THICK, markerArea.x2, markerArea.y2, RESIZEMARKER_COLOR, true);
		__k_drawRect(g_windowManager.videoMem, &g_windowManager.screenArea, markerArea.x1, markerArea.y1, markerArea.x1 + RESIZEMARKER_THICK, markerArea.y2, RESIZEMARKER_COLOR, true);

		// draw right-bottom marker.
		k_setRect(&markerArea, area->x2 - RESIZEMARKER_SIZE, area->y2 - RESIZEMARKER_SIZE, area->x2, area->y2);
		__k_drawRect(g_windowManager.videoMem, &g_windowManager.screenArea, markerArea.x1, markerArea.y2 - RESIZEMARKER_THICK, markerArea.x2, markerArea.y2, RESIZEMARKER_COLOR, true);
		__k_drawRect(g_windowManager.videoMem, &g_windowManager.screenArea, markerArea.x2 - RESIZEMARKER_THICK, markerArea.y1, markerArea.x2, markerArea.y2, RESIZEMARKER_COLOR, true);

	/* clear resize marker */
	} else {
		// clear left-top marker.
		k_setRect(&markerArea, area->x1, area->y1, area->x1 + RESIZEMARKER_SIZE, area->y1 + RESIZEMARKER_SIZE);
		k_redrawWindowByArea(WINDOW_INVALIDID, &markerArea);

		// clear right-top marker.
		k_setRect(&markerArea, area->x2 - RESIZEMARKER_SIZE, area->y1, area->x2, area->y1 + RESIZEMARKER_SIZE);
		k_redrawWindowByArea(WINDOW_INVALIDID, &markerArea);

		// clear left-bottom marker.
		k_setRect(&markerArea, area->x1, area->y2 - RESIZEMARKER_SIZE, area->x1 + RESIZEMARKER_SIZE, area->y2);
		k_redrawWindowByArea(WINDOW_INVALIDID, &markerArea);

		// clear right-bottom marker.
		k_setRect(&markerArea, area->x2 - RESIZEMARKER_SIZE, area->y2 - RESIZEMARKER_SIZE, area->x2, area->y2);
		k_redrawWindowByArea(WINDOW_INVALIDID, &markerArea);
	}
}
