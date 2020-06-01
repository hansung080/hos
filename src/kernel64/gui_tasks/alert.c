#include "alert.h"
#include "../utils/util.h"
#include "../core/console.h"
#include "../core/2d_graphics.h"
#include "../core/window.h"
#include "../fonts/fonts.h"

void k_alertTask(qword arg) {
    const char* msg = (const char*)arg;
    Rect screenArea;
    qword windowId;
    int len;
    int x, y;
    Event event;
    KeyEvent* keyEvent;

    /* check graphic mode */
    if (k_isGraphicMode() == false) {
        k_printf("[alert error] not graphic mode\n");
        return;
    }

    /* create window */
    k_getScreenArea(&screenArea);
    windowId = k_createWindow(
        (screenArea.x2 + 1 - ALERT_WIDTH) / 2, // x
        ((screenArea.y2 + 1 - ALERT_HEIGHT) / 2) - ALERT_HEIGHT, // y
        ALERT_WIDTH, // width
        ALERT_HEIGHT, // height
        WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_BLOCKING, // flags
        ALERT_TITLE, // title
        WINDOW_COLOR_BACKGROUND, // backgroundColor
        null, // topMenu
        null, // widget
        WINDOW_INVALIDID // parentId
    );
    if (windowId == WINDOW_INVALIDID) {
        k_printf("[alert error] window creation failure\n");
        return;
    }

    /* draw message */
    len = k_strlen(msg);
    x = (ALERT_WIDTH - FONT_DEFAULT_WIDTH * len) / 2;
    y = ((ALERT_HEIGHT - WINDOW_TITLEBAR_HEIGHT - FONT_DEFAULT_HEIGHT) / 2) + WINDOW_TITLEBAR_HEIGHT;
    k_drawText(windowId, x, y, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, msg, len);

    k_showWindow(windowId, true);

    /* event processing loop */
    while (true) {
        if (k_recvEventFromWindow(&event, windowId) == false) {
            k_sleep(0);
            continue;
        }

        switch (event.type) {
        case EVENT_WINDOW_CLOSE:
            k_deleteWindow(windowId);
            return;

        case EVENT_KEY_DOWN:
            keyEvent = &event.keyEvent;
            if (keyEvent->asciiCode == KEY_ESC) {
                k_deleteWindow(windowId);
                return;
            }
        }
    }
}
