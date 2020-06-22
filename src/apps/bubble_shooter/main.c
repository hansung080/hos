#include "bubble_shooter.h"

int main(const char* args) {
    qword windowId;
    Point mouse;
    Rect screen;
    int x, y;

	/* check graphic mode */
	if (isGraphicMode() == false) {
		printf("[bubble shooter error] not graphic mode\n");
		return -1;
	}

    /* create window */
    getScreenArea(&screen);
    x = (getRectWidth(&screen) - WINDOW_WIDTH) / 2;
    y = (getRectHeight(&screen) - WINDOW_HEIGHT) / 2;
	windowId = createWindow(
        x, // x
        y, // y
        WINDOW_WIDTH,  // width
        WINDOW_HEIGHT, // height
        WINDOW_FLAGS_DEFAULT, // flags
        TITLE, // title
        WINDOW_COLOR_BACKGROUND, // backgroundColor
        null, // topMenu
        null, // widget
        WINDOW_INVALIDID // parentId
    );
	if (windowId == WINDOW_INVALIDID) {
		printf("[bubble shooter error] window creation failure\n");
		return -1;
	}

    /* initialize game */
    if (initGame() == false) {
        deleteWindow(windowId);
        return -1;
    }

    mouse.x = WINDOW_WIDTH / 2;
    mouse.y = WINDOW_HEIGHT / 2;
    srand(getTickCount());

    /* draw game */
    drawInfo(windowId);
    drawGame(windowId, &mouse);
    drawText(windowId, 15, 150, RGB(255, 255, 255), RGB(0, 0, 0), GAME_START_MESSAGE, strlen(GAME_START_MESSAGE));
    showWindow(windowId, true);

    /* event processing loop */
    while (true) {
        if (processEvent(windowId, &mouse) == false) {
            break;
        }

        processGame(windowId, &mouse);
    }
}