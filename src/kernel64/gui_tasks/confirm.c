#include "confirm.h"
#include "../utils/util.h"
#include "../core/console.h"
#include "../core/window.h"
#include "../fonts/fonts.h"
#include "../core/task.h"

void k_confirm(const ConfirmArg* arg) {
    k_createTask(TASK_PRIORITY_LOW | TASK_FLAGS_THREAD, null, 0, (qword)k_confirmTask, (qword)arg, TASK_AFFINITY_LB);
}

static void k_confirmTask(qword _arg) {
    const ConfirmArg* arg = (const ConfirmArg*)_arg;
    Rect screenArea;
    int len;
    int x, y;
    Confirm confirm = {arg};

    /* check argument */
    if (arg->msg == null) {
        k_printf("[alert error] 'message' argument not provided\n");
        return;
    }

    if (arg->okFunc == null) {
        k_printf("[alert error] 'okFunction' argument not provided\n");
        return;
    }

    /* check graphic mode */
    if (k_isGraphicMode() == false) {
        k_printf("[confirm error] not graphic mode\n");
        return;
    }

    /* create window */
    k_getScreenArea(&screenArea);
    confirm.windowId = k_createWindow(
        (screenArea.x2 + 1 - CONFIRM_WIDTH) / 2, // x
        ((screenArea.y2 + 1 - CONFIRM_HEIGHT) / 2) - CONFIRM_HEIGHT, // y
        CONFIRM_WIDTH, // width
        CONFIRM_HEIGHT, // height
        WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_BLOCKING, // flags
        CONFIRM_TITLE, // title
        WINDOW_COLOR_BACKGROUND, // backgroundColor
        null, // topMenu
        null, // widget
        WINDOW_INVALIDID // parentId
    );
    if (confirm.windowId == WINDOW_INVALIDID) {
        k_printf("[confirm error] window creation failure\n");
        return;
    }

    /* draw message */
    len = k_strlen(arg->msg);
    x = (CONFIRM_WIDTH - FONT_DEFAULT_WIDTH * len) / 2;
    y = ((CONFIRM_HEIGHT - WINDOW_TITLEBAR_HEIGHT - FONT_DEFAULT_HEIGHT) / 2) + WINDOW_TITLEBAR_HEIGHT - 10;
    k_drawText(confirm.windowId, x, y, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, arg->msg, len);

    /* draw buttons */
    k_setRect(&confirm.cancelButton, CONFIRM_WIDTH - 10 - CONFIRM_BUTTON_WIDTH, CONFIRM_HEIGHT - 10 - CONFIRM_BUTTON_HEIGHT, CONFIRM_WIDTH - 10, CONFIRM_HEIGHT - 10);
    k_setRect(&confirm.okButton, CONFIRM_WIDTH - 20 - CONFIRM_BUTTON_WIDTH * 2, CONFIRM_HEIGHT - 10 - CONFIRM_BUTTON_HEIGHT, CONFIRM_WIDTH - 20 - CONFIRM_BUTTON_WIDTH, CONFIRM_HEIGHT - 10);
    confirm.ok = true;
    k_drawConfirmButtons(&confirm);

    k_showWindow(confirm.windowId, true);

    /* event processing loop */
    while (true) {
        if (k_processConfirmEvent(&confirm) == false) {
            break;
        }
    }
}

static bool k_processConfirmEvent(Confirm* confirm) {
    Event event;
    KeyEvent* keyEvent;
    MouseEvent* mouseEvent;

    while (true) {
        if (k_recvEventFromWindow(&event, confirm->windowId) == false) {
            k_printf("[confirm error] k_recvEventFromWindow error in blocking mode");
            return false;
        }

        switch (event.type) {
        case EVENT_MOUSE_MOVE:
            mouseEvent = &event.mouseEvent;
            if (confirm->ok == false && k_isPointInRect(&confirm->okButton, mouseEvent->point.x, mouseEvent->point.y) == true) {
                confirm->ok = true;
                k_drawConfirmButtons(confirm);

            } else if (confirm->ok == true && k_isPointInRect(&confirm->cancelButton, mouseEvent->point.x, mouseEvent->point.y) == true) {
                confirm->ok = false;
                k_drawConfirmButtons(confirm);
            }
            break;

        case EVENT_MOUSE_LBUTTONDOWN:
            mouseEvent = &event.mouseEvent;
            if (k_isPointInRect(&confirm->okButton, mouseEvent->point.x, mouseEvent->point.y) == true) {
                confirm->arg->okFunc();
                return false;

            } else if (k_isPointInRect(&confirm->cancelButton, mouseEvent->point.x, mouseEvent->point.y) == true) {
                k_deleteWindow(confirm->windowId);
                return false;
            }
            break;

        case EVENT_KEY_DOWN:
            keyEvent = &event.keyEvent;
            switch (keyEvent->asciiCode) {
            case KEY_LEFT:
                if (confirm->ok == false) {
                    confirm->ok = true;
                    k_drawConfirmButtons(confirm);
                }
                break;

            case KEY_RIGHT:
                if (confirm->ok == true) {
                    confirm->ok = false;
                    k_drawConfirmButtons(confirm);
                }
                break;

            case KEY_ENTER:
                if (confirm->ok == true) {
                    confirm->arg->okFunc();
                    return false;

                } else {
                    k_deleteWindow(confirm->windowId);
                    return false;
                }
                break;

            case KEY_ESC:
                k_deleteWindow(confirm->windowId);
                return false;
            }
            break;

        case EVENT_WINDOW_CLOSE:
            k_deleteWindow(confirm->windowId);
            return false;
        }
    }

    return true;
}

static void k_drawConfirmButtons(const Confirm* confirm) {
    Rect updateArea;

    if (confirm->ok == true) {
        k_drawButton(confirm->windowId, &confirm->okButton, CONFIRM_COLOR_BUTTONTEXT, CONFIRM_COLOR_BUTTONBACKGROUNDACTIVE, "OK", BUTTON_FLAGS_SHADOW);
        k_drawButton(confirm->windowId, &confirm->cancelButton, CONFIRM_COLOR_BUTTONTEXT, CONFIRM_COLOR_BUTTONBACKGROUNDINACTIVE, "Cancel", BUTTON_FLAGS_SHADOW);
    } else {
        k_drawButton(confirm->windowId, &confirm->okButton, CONFIRM_COLOR_BUTTONTEXT, CONFIRM_COLOR_BUTTONBACKGROUNDINACTIVE, "OK", BUTTON_FLAGS_SHADOW);
        k_drawButton(confirm->windowId, &confirm->cancelButton, CONFIRM_COLOR_BUTTONTEXT, CONFIRM_COLOR_BUTTONBACKGROUNDACTIVE, "Cancel", BUTTON_FLAGS_SHADOW);
    }

    k_setRect(&updateArea, confirm->okButton.x1, confirm->okButton.y1, confirm->cancelButton.x2, confirm->cancelButton.y2);
    k_updateScreenByWindowArea(confirm->windowId, &updateArea);
}