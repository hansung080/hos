#include "window.h"
#include "vbe.h"
#include "task.h"
#include "multiprocessor.h"
#include "fonts.h"
#include "dynamic_mem.h"
#include "util.h"
#include "console.h"

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
		k_printf("window error: window pool allocation error\n");
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

void k_initGui(void) {
	VbeModeInfoBlock* vbeMode;
	qword backgroundWindowId;

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

	// allocate event buffer
	g_windowManager.eventBuffer = (Event*)k_allocMem(sizeof(Event) * EVENTQUEUE_WINDOWMANAGER_MAXCOUNT);
	if (g_windowManager.eventBuffer == null) {
		k_printf("window error: window manager event buffer allocation error\n");
		while (true) {
			;
		}
	}

	k_initQueue(&g_windowManager.eventQueue, g_windowManager.eventBuffer, EVENTQUEUE_WINDOWMANAGER_MAXCOUNT, sizeof(Event));

	g_windowManager.prevButtonStatus = 0;
	g_windowManager.windowMoving = false;
	g_windowManager.movingWindowId = WINDOW_INVALIDID;

	/* create background window */
	// set 0 to flags in order not to show. After drawing background color, It will show.
	backgroundWindowId = k_createWindow(0, 0, vbeMode->xResolution, vbeMode->yResolution, 0, WINDOW_BACKGROUNDWINDOWTITLE);
	g_windowManager.backgroundWindowId = backgroundWindowId;
	k_drawRect(backgroundWindowId, 0, 0, vbeMode->xResolution - 1, vbeMode->yResolution - 1, WINDOW_COLOR_SYSTEMBACKGROUND, true);	
	k_showWindow(backgroundWindowId, true);
}

WindowManager* k_getWindowManager(void) {
	return &g_windowManager;
}

qword k_getBackgroundWindowId(void) {
	return g_windowManager.backgroundWindowId;
}

void k_getScreenArea(Rect* screenArea) {
	k_memcpy(screenArea, &g_windowManager.screenArea, sizeof(Rect));
}

qword k_createWindow(int x, int y, int width, int height, dword flags, const char* title) {
	Window* window;
	Task* task;
	qword topWindowId;

	if ((width <= 0) || (height <= 0)) {
		return WINDOW_INVALIDID;
	}

	/* allocate window */
	window = k_allocWindow();
	if (window == null) {
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
		k_freeWindow(window->link.id);
		return WINDOW_INVALIDID;
	}

	// allocate event buffer.
	window->eventBuffer = (Event*)k_allocMem(sizeof(Event) * EVENTQUEUE_WINDOW_MAXCOUNT);
	if (window->eventBuffer == null) {
		k_freeMem(window->buffer);
		k_freeWindow(window->link.id);
		return WINDOW_INVALIDID;	
	}

	k_initQueue(&window->eventQueue, window->eventBuffer, EVENTQUEUE_WINDOW_MAXCOUNT, sizeof(Event));

	task = k_getRunningTask(k_getApicId());
	window->taskId = task->link.id;

	window->flags = flags;

	/* draw window to window buffer */
	k_drawWindowBackground(window->link.id);

	if (flags & WINDOW_FLAGS_DRAWFRAME) {
		k_drawWindowFrame(window->link.id);
	}

	if (flags & WINDOW_FLAGS_DRAWTITLEBAR) {
		k_drawWindowTitleBar(window->link.id, title, true);
	}

	/* add window to window list */
	k_lock(&g_windowManager.mutex);

	topWindowId = k_getTopWindowId();

	// add window to the top of screen.
	// Getting closer to the tail in window list means getting closer to the top in screen.
	k_addListToTail(&g_windowManager.windowList, window);

	k_unlock(&g_windowManager.mutex);

	/* update screen and send window event */
	k_updateScreenById(window->link.id);
	k_sendWindowEventToWindow(window->link.id, EVENT_WINDOW_SELECT);

	if (topWindowId != g_windowManager.backgroundWindowId) {
		k_updateWindowTitleBar(topWindowId, false);
		k_sendWindowEventToWindow(topWindowId, EVENT_WINDOW_DESELECT);
	}

	return window->link.id;
}

bool k_deleteWindow(qword windowId) {
	Window* window;
	Rect area;
	qword topWindowId;
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

	/* remove window from window list */
	if (k_removeList(&g_windowManager.windowList, windowId) == null) {
		k_unlock(&window->mutex);
		k_unlock(&g_windowManager.mutex);
		return false;
	}

	/* free window buffer */
	k_freeMem(window->buffer);
	window->buffer = null;

	/* free event buffer */
	k_freeMem(window->eventBuffer);
	window->eventBuffer = null;

	k_unlock(&window->mutex);

	/* free window */
	k_freeWindow(windowId);

	k_unlock(&g_windowManager.mutex);

	/* update screen and send window event */
	k_updateScreenByScreenArea(&area);

	if (top == true) {
		topWindowId = k_getTopWindowId();
		if (topWindowId != WINDOW_INVALIDID) {
			k_updateWindowTitleBar(topWindowId, true);
			k_sendWindowEventToWindow(topWindowId, EVENT_WINDOW_SELECT);
		}
	}

	return true;
}

bool k_deleteAllWindowsByTask(qword taskId) {
	Window* window;
	Window* nextWindow;

	k_lock(&g_windowManager.mutex);

	window = k_getHeadFromList(&g_windowManager.windowList);
	while (window != null) {
		nextWindow = k_getNextFromList(&g_windowManager.windowList, window);

		if ((window->link.id != g_windowManager.backgroundWindowId) && (window->taskId == taskId)) {
			k_deleteWindow(window->link.id);
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
  [Note]
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

bool k_redrawWindowByArea(const Rect* area) {
	Window* window;
	Rect overArea;
	Rect cursorArea;

	if (k_getOverlappedRect(&g_windowManager.screenArea, area, &overArea) == false) {
		return false;
	}

	/**
	  draw windows overlapped with the parameter area,
	  by looping from head (the bottom window) to tail (the top window) in window list.
	*/
	k_lock(&g_windowManager.mutex);

	window = k_getHeadFromList(&g_windowManager.windowList);
	while (window != null) {
		if ((window->flags & WINDOW_FLAGS_SHOW) && (k_isRectOverlapped(&window->area, &overArea) == true)) {
			k_lock(&window->mutex);
			k_copyWindowBufferToVideoMem(window, &overArea);
			k_unlock(&window->mutex);
		}

		window = k_getNextFromList(&g_windowManager.windowList, window);
	}

	k_unlock(&g_windowManager.mutex);	

	/**
	  draw mouse cursor overlapped with the parameter area.
	*/
	k_setRect(&cursorArea, g_windowManager.mouseX, g_windowManager.mouseY, g_windowManager.mouseX + MOUSE_CURSOR_WIDTH, g_windowManager.mouseY + MOUSE_CURSOR_HEIGHT);
	
	if (k_isRectOverlapped(&cursorArea, &overArea) == true) {
		k_drawMouseCursor(g_windowManager.mouseX, g_windowManager.mouseY);
	}
}

static void k_copyWindowBufferToVideoMem(const Window* window, const Rect* copyArea) {
	Rect tempArea;
	Rect overArea;
	int screenWidth;
	int windowWidth;
	int overWidth;
	int overHeight;
	int i;
	Color* currentVideoMem;
	Color* currentWindowBuffer;

	if (k_getOverlappedRect(&g_windowManager.screenArea, copyArea, &tempArea) == false) {
		return;
	}

	if (k_getOverlappedRect(&window->area, &tempArea, &overArea) == false) {
		return;
	}

	screenWidth = k_getRectWidth(&g_windowManager.screenArea);
	windowWidth = k_getRectWidth(&window->area);
	overWidth = k_getRectWidth(&overArea);
	overHeight = k_getRectHeight(&overArea);

	/**
	  calculate current video memory and current window buffer.
	  Window buffer uses window coordinates. Thus, (x1, y1) of overlapped area has been recalculated on window coordinates.
	*/
	currentVideoMem = g_windowManager.videoMem + overArea.y1 * screenWidth + overArea.x1;
	currentWindowBuffer = window->buffer + (overArea.y1 - window->area.y1) * windowWidth + (overArea.x1 - window->area.x1);

	for (i = 0; i < overHeight; i++) {
		k_memcpy(currentVideoMem, currentWindowBuffer, sizeof(Color) * overWidth);
		currentVideoMem += screenWidth;
		currentWindowBuffer += windowWidth;
	}
}

qword k_findWindowByPoint(int x, int y) {
	qword windowId;
	Window* window;

	// set background window ID to default window ID,
	// because mouse point is always inside background window.
	windowId = g_windowManager.backgroundWindowId;

	/**
	  loop from the second window (the next window of background window),
	  and find the top window among windows which include mouse point.
	*/
	k_lock(&g_windowManager.mutex);

	window = k_getHeadFromList(&g_windowManager.windowList);

	do {
		window = k_getNextFromList(&g_windowManager.windowList, window);
		if ((window != null) && (window->flags & WINDOW_FLAGS_SHOW) && (k_isPointInRect(&window->area, x, y) == true)) {
			windowId = window->link.id;
		}

	} while (window != null);

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
	  loop from the first window (background window),
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
	Window* topWindow;
	qword topWindowId;

	k_lock(&g_windowManager.mutex);

	topWindow = k_getTailFromList(&g_windowManager.windowList);
	if (topWindow != null) {
		topWindowId = topWindow->link.id;

	} else {
		topWindowId = WINDOW_INVALIDID;
	}

	k_unlock(&g_windowManager.mutex);

	return topWindowId;
}

bool k_moveWindowToTop(qword windowId) {
	Window* window;
	Rect area;
	dword flags;
	qword topWindowId;
	
	topWindowId = k_getTopWindowId();
	if (topWindowId == windowId) {
		return true;
	}

	/* move window to top */
	k_lock(&g_windowManager.mutex);

	window = k_removeList(&g_windowManager.windowList, windowId);
	if (window != null) {
		k_addListToTail(&g_windowManager.windowList, window);
		k_convertRectScreenToWindow(windowId, &window->area, &area);
		flags = window->flags;
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

		k_sendWindowEventToWindow(topWindowId, EVENT_WINDOW_DESELECT);
		k_updateWindowTitleBar(topWindowId, false);

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

	if ((window->area.x2 - 1 - WINDOW_CLOSEBUTTON_SIZE <= x) && (x <= window->area.x2 - 1) && (window->area.y1 + 1 <= y) && (y <= window->area.y1 + 1 + WINDOW_CLOSEBUTTON_SIZE)) {
		return true;
	}

	return false;
}

bool k_moveWindow(qword windowId, int x, int y) {
	Window* window;
	Rect prevArea;
	int width;
	int height;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	/* change window area */
	k_memcpy(&prevArea, &window->area, sizeof(Rect));
	width = k_getRectWidth(&prevArea);
	height = k_getRectHeight(&prevArea);
	k_setRect(&window->area, x, y, x + width - 1, y + height - 1);

	k_unlock(&window->mutex);

	/* update screen and send window event */
	k_updateScreenByScreenArea(&prevArea);
	k_updateScreenById(windowId);
	k_sendWindowEventToWindow(windowId, EVENT_MOUSE_MOVE);

	return true;
}

static bool k_updateWindowTitleBar(qword windowId, bool selected) {
	Window* window;
	Rect titleBarArea;

	window = k_getWindow(windowId);
	if ((window != null) && (window->flags & WINDOW_FLAGS_DRAWTITLEBAR)) {
		k_drawWindowTitleBar(window->link.id, window->title, selected);
		k_setRect(&titleBarArea, 0, 0, k_getRectWidth(&window->area) - 1, WINDOW_TITLEBAR_HEIGHT);
		k_updateScreenByWindowArea(windowId, &titleBarArea);
		return true;
	}

	return false;
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
	windowRect->x2 = screenRect->x2 - area.x2;
	windowRect->y2 = screenRect->y2 - area.y2;

	return true;
}

bool k_convertRectWindowToScreen(qword windowId, const Rect* windowRect, Rect* screenRect) {
	Rect area;

	if (k_getWindowArea(windowId, &area) == false) {
		return false;
	}

	screenRect->x1 = windowRect->x1 + area.x1;
	screenRect->y1 = windowRect->y1 + area.y1;
	screenRect->x2 = windowRect->x2 + area.x2;
	screenRect->y2 = windowRect->y2 + area.y2;

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
		screenMousePoint.x = mouseX;
		screenMousePoint.y = mouseY;

		if (k_convertPointScreenToWindow(windowId, &screenMousePoint, &windowMousePoint) == false) {
			return false;
		}

		/* set mouse event */
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

		/* set window event */
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

	result = k_getQueue(&window->eventQueue, event);

	k_unlock(&window->mutex);

	return result;
}

bool k_sendEventToWindowManager(const Event* event) {
	bool result;

	if (k_isQueueFull(&g_windowManager.eventQueue) == true) {
		return false;
	}

	k_lock(&g_windowManager.mutex);

	result = k_putQueue(&g_windowManager.eventQueue, event);

	k_unlock(&g_windowManager.mutex);

	return result;
}

bool k_recvEventFromWindowManager(Event* event) {
	bool result;

	if (k_isQueueEmpty(&g_windowManager.eventQueue) == true) {
		return false;
	}

	k_lock(&g_windowManager.mutex);

	result = k_getQueue(&g_windowManager.eventQueue, event);

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

	// draw edges (2 pixes-thick) of a window frame.
	__k_drawRect(window->buffer, &area, 0, 0, width - 1, height - 1, WINDOW_COLOR_FRAME, false);
	__k_drawRect(window->buffer, &area, 1, 1, width - 2, height - 2, WINDOW_COLOR_FRAME, false);

	k_unlock(&window->mutex);

	return true;
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
	__k_drawRect(window->buffer, &area, x, y, width - 1 - x, height - 1 - x, WINDOW_COLOR_BACKGROUND, true);

	k_unlock(&window->mutex);
}

bool k_drawWindowTitleBar(qword windowId, const char* title, bool selected) {
	Window* window;
	int width;
	int height;
	Rect area;
	Rect closeButtonArea;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// get width, height and set clipping area on window coordinates.
	width = k_getRectWidth(&window->area);
	height = k_getRectHeight(&window->area);
	k_setRect(&area, 0, 0, width - 1, height - 1);

	/* draw a title bar */

	if (selected == true) {
		// draw a title bar background.
		__k_drawRect(window->buffer, &area, 0, 3, width - 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBACKGROUNDACTIVE, true);
		
		// draw a title.
		__k_drawText(window->buffer, &area, 6, 3, WINDOW_COLOR_TITLEBARTEXT, WINDOW_COLOR_TITLEBARBACKGROUNDACTIVE, title);

	} else {
		// draw a title bar background.
		__k_drawRect(window->buffer, &area, 0, 3, width - 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBACKGROUNDINACTIVE, true);
		
		// draw a title.
		__k_drawText(window->buffer, &area, 6, 3, WINDOW_COLOR_TITLEBARTEXT, WINDOW_COLOR_TITLEBARBACKGROUNDINACTIVE, title);
	}

	// draw top lines (2 pixels-thick) of the title bar with bright color in order to make it 3-dimensional.
	__k_drawLine(window->buffer, &area, 1, 1, width - 1, 1, WINDOW_COLOR_TITLEBARBRIGHT1);
	__k_drawLine(window->buffer, &area, 1, 2, width - 1, 2, WINDOW_COLOR_TITLEBARBRIGHT2);
	
	// draw left lines (2 pixels-thick) of the title bar with bright color in order to make it 3-dimensional.
	__k_drawLine(window->buffer, &area, 1, 2, 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBRIGHT1);
	__k_drawLine(window->buffer, &area, 2, 2, 2, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARBRIGHT2);
	
	// draw bottom lines (2 pixels-thick) of the title bar with dark color in order to make it 3-dimensional.
	__k_drawLine(window->buffer, &area, 2, WINDOW_TITLEBAR_HEIGHT - 2, width - 2, WINDOW_TITLEBAR_HEIGHT - 2, WINDOW_COLOR_TITLEBARDARK);
	__k_drawLine(window->buffer, &area, 2, WINDOW_TITLEBAR_HEIGHT - 1, width - 2, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_COLOR_TITLEBARDARK);
	
	k_unlock(&window->mutex);

	/* draw a close button */

	// draw a close button.
	k_setRect(&closeButtonArea, width - 1 - WINDOW_CLOSEBUTTON_SIZE, 1, width - 2, WINDOW_CLOSEBUTTON_SIZE - 1);
	k_drawButton(windowId, &closeButtonArea, WINDOW_COLOR_BACKGROUND, "", WINDOW_COLOR_BACKGROUND);

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// draw 'X' mark (3 pixels-thick) on the close button.
	__k_drawLine(window->buffer, &area, width - 2 - 18 + 4, 1 + 4, width - 2 - 4, WINDOW_TITLEBAR_HEIGHT - 6, WINDOW_COLOR_CLOSEBUTTONMARK);
	__k_drawLine(window->buffer, &area, width - 2 - 18 + 5, 1 + 4, width - 2 - 4, WINDOW_TITLEBAR_HEIGHT - 7, WINDOW_COLOR_CLOSEBUTTONMARK);
	__k_drawLine(window->buffer, &area, width - 2 - 18 + 4, 1 + 5, width - 2 - 5, WINDOW_TITLEBAR_HEIGHT - 6, WINDOW_COLOR_CLOSEBUTTONMARK);
	
	__k_drawLine(window->buffer, &area, width - 2 - 18 + 4, 19 - 4, width - 2 - 4, 1 + 4, WINDOW_COLOR_CLOSEBUTTONMARK);
	__k_drawLine(window->buffer, &area, width - 2 - 18 + 5, 19 - 4, width - 2 - 4, 1 + 5, WINDOW_COLOR_CLOSEBUTTONMARK);
	__k_drawLine(window->buffer, &area, width - 2 - 18 + 4, 19 - 5, width - 2 - 5, 1 + 4, WINDOW_COLOR_CLOSEBUTTONMARK);

	k_unlock(&window->mutex);

	return true;
}

bool k_drawButton(qword windowId, const Rect* buttonArea, Color backgroundColor, const char* text, Color textColor) {
	Window* window;
	int windowWidth;
	int windowHeight;
	Rect area;
	int buttonWidth;
	int buttonHeight;
	int textLen;
	int textWidth;
	int textX;
	int textY;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// get window width, window height and set clipping area on window coordinates.
	windowWidth = k_getRectWidth(&window->area);
	windowHeight = k_getRectHeight(&window->area);
	k_setRect(&area, 0, 0, windowWidth - 1, windowHeight - 1);

	// draw a button background.
	__k_drawRect(window->buffer, &area, buttonArea->x1, buttonArea->y1, buttonArea->x2, buttonArea->y2, backgroundColor, true);	

	// get button width, button height, text length, text width;
	buttonWidth = k_getRectWidth(buttonArea);
	buttonHeight = k_getRectHeight(buttonArea);
	textLen = k_strlen(text);
	textWidth = textLen * FONT_VERAMONO_ENG_WIDTH;

	// draw a text in the center.
	textX = (buttonArea->x1 + buttonWidth / 2) - textWidth / 2;
	textY = (buttonArea->y1 + buttonHeight / 2) - FONT_VERAMONO_ENG_HEIGHT / 2;
	__k_drawText(window->buffer, &area, textX, textY, textColor, backgroundColor, text);
		
	// draw top lines (2 pixels-thick) of the button with bright color in order to make it 3-dimensional.
	__k_drawLine(window->buffer, &area, buttonArea->x1, buttonArea->y1, buttonArea->x2, buttonArea->y1, WINDOW_COLOR_BUTTONBRIGHT);
	__k_drawLine(window->buffer, &area, buttonArea->x1, buttonArea->y1 + 1, buttonArea->x2 - 1, buttonArea->y1 + 1, WINDOW_COLOR_BUTTONBRIGHT);

	// draw left lines (2 pixels-thick) of the button with bright color in order to make it 3-dimensional.
	__k_drawLine(window->buffer, &area, buttonArea->x1, buttonArea->y1, buttonArea->x1, buttonArea->y2, WINDOW_COLOR_BUTTONBRIGHT);
	__k_drawLine(window->buffer, &area, buttonArea->x1 + 1, buttonArea->y1, buttonArea->x1 + 1, buttonArea->y2 - 1, WINDOW_COLOR_BUTTONBRIGHT);
	
	// draw bottom lines (2 pixels-thick) of the button with dark color in order to make it 3-dimensional.
	__k_drawLine(window->buffer, &area, buttonArea->x1 + 1, buttonArea->y2, buttonArea->x2, buttonArea->y2, WINDOW_COLOR_BUTTONDARK);
	__k_drawLine(window->buffer, &area, buttonArea->x1 + 2, buttonArea->y2 - 1, buttonArea->x2, buttonArea->y2 - 1, WINDOW_COLOR_BUTTONDARK);

	// draw right lines (2 pixels-thick) of the button with dark color in order to make it 3-dimensional.
	__k_drawLine(window->buffer, &area, buttonArea->x2, buttonArea->y1 + 1, buttonArea->x2, buttonArea->y2, WINDOW_COLOR_BUTTONDARK);
	__k_drawLine(window->buffer, &area, buttonArea->x2 - 1, buttonArea->y1 + 2, buttonArea->x2 - 1, buttonArea->y2, WINDOW_COLOR_BUTTONDARK);

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

bool k_drawText(qword windowId, int x, int y, Color textColor, Color backgroundColor, const char* str) {
	Window* window;
	Rect area;

	window = k_getWindowWithLock(windowId);
	if (window == null) {
		return false;
	}

	// set clipping area on window coordinates.
	k_setRect(&area, 0, 0, window->area.x2 - window->area.x1, window->area.y2 - window->area.y1);

	// draw a text.
	__k_drawText(window->buffer, &area, x, y, textColor, backgroundColor, str);
	
	k_unlock(&window->mutex);

	return true;	
}

// mouse cursor bitmap (20 * 20 = 400 bytes)
// A byte in bitmap represents a color (16 bits) in video memory or a pixel (16 bits) in screen.
static byte g_mouseCursorBitmap[MOUSE_CURSOR_WIDTH * MOUSE_CURSOR_HEIGHT] = {
	1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0,
	0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1,
	0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1,
	0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1, 0, 0,
	0, 1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 1, 1, 0, 0, 0, 0,
	0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 2, 3, 3, 2, 1, 1, 2, 3, 2, 2, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 1, 2, 3, 2, 2, 1, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 0,
	0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0,
	0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0
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

			case 1: // outer line
				__k_drawPixel(g_windowManager.videoMem, &g_windowManager.screenArea, x + j, y + i, MOUSE_CURSOR_COLOR_OUTERLINE);
				break;

			case 2: // outer
				__k_drawPixel(g_windowManager.videoMem, &g_windowManager.screenArea, x + j, y + i, MOUSE_CURSOR_COLOR_OUTER);
				break;

			case 3: // inner
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

	// backup previous mouse position.
	k_setRect(&prevArea, g_windowManager.mouseX, g_windowManager.mouseY, g_windowManager.mouseX + MOUSE_CURSOR_WIDTH - 1, g_windowManager.mouseY + MOUSE_CURSOR_HEIGHT - 1);

	// save current mouse position.
	g_windowManager.mouseX = x;
	g_windowManager.mouseY = y;

	k_unlock(&g_windowManager.mutex);

	// redraw window by previous mouse area.
	k_redrawWindowByArea(&prevArea);

	// draw mouse cursor at current mouse position.
	k_drawMouseCursor(x, y);
}

void k_getMouseCursorPos(int* x, int* y) {
	*x = g_windowManager.mouseX;
	*y = g_windowManager.mouseY;
}