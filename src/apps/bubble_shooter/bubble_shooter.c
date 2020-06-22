#include "bubble_shooter.h"

static Game g_game = {0, };

bool initGame(void) {
    if (g_game.bubbles == null) {
        g_game.bubbles = malloc(sizeof(Bubble) * BUBBLE_MAXCOUNT);
        if (g_game.bubbles == null) {
            printf("[bubble shooter error] bubbles allocation failure\n");
            return false;
        }
    }

    memset(g_game.bubbles, 0, sizeof(Bubble) * BUBBLE_MAXCOUNT);
    g_game.aliveBubbles = 0;
    g_game.life = PLAYER_MAXLIFE;
    g_game.score = 0;
    g_game.started = false;
    g_game.lastDrawing = getTickCount();
    return true;
}

bool processEvent(qword windowId, Point* mouse) {
    Event event;
    MouseEvent* mouseEvent;
    KeyEvent* keyEvent;

    while (true) {
        if (recvEventFromWindow(&event, windowId) == false) {
            return true;
        }

        switch (event.type) {
        case EVENT_MOUSE_MOVE:
            memcpy(mouse, &mouseEvent->point, sizeof(Point));
            break;

        case EVENT_MOUSE_LBUTTONDOWN:
            mouseEvent = &event.mouseEvent;
            deleteBubble(&mouseEvent->point);
            memcpy(mouse, &mouseEvent->point, sizeof(Point));
            break;

        case EVENT_KEY_DOWN:
            keyEvent = &event.keyEvent;
            switch (keyEvent->asciiCode) {
            case KEY_ENTER:
                if (g_game.started == false) {
                    initGame();
                    g_game.started = true;
                }
                break;

            case KEY_ESC:
                deleteWindow(windowId);
                free(g_game.bubbles);
                return false;
            }
            break;

        case EVENT_WINDOW_CLOSE:
            deleteWindow(windowId);
            free(g_game.bubbles);
            return false;
        }
    }

    return true;
}

void processGame(qword windowId, Point* mouse) {
    if (g_game.started == false || (getTickCount() - g_game.lastDrawing) < 50) {
        sleep(0);
        return;
    }

    g_game.lastDrawing = getTickCount();

    if (rand() % 7 == 1) {
        createBubble();
    }

    moveBubble();
    drawGame(windowId, mouse);
    drawInfo(windowId);
    if (g_game.life <= 0) {
        g_game.started = false;
        drawText(windowId, 80, 130, RGB(255, 255, 255), RGB(0, 0, 0), GAME_END_MESSAGE, strlen(GAME_END_MESSAGE));
        drawText(windowId, 15, 150, RGB(255, 255, 255), RGB(0, 0, 0), GAME_START_MESSAGE, strlen(GAME_START_MESSAGE));
    }

    showWindow(windowId, true);
}

static bool createBubble(void) {
    Bubble* bubble;
    int i;

    if (g_game.aliveBubbles >= BUBBLE_MAXCOUNT) {
        return false;
    }

    for (i = 0; i < BUBBLE_MAXCOUNT; ++i) {
        if (g_game.bubbles[i].alive == false) {
            bubble = &g_game.bubbles[i];
            bubble->alive = true;
            bubble->x = rand() % (WINDOW_WIDTH - 2 * BUBBLE_RADIUS) + BUBBLE_RADIUS;
            bubble->y = WINDOW_TITLEBAR_HEIGHT + INFO_HEIGHT + BUBBLE_RADIUS + 1;
            bubble->dy = (rand() % 8) + BUBBLE_DEFAULTSPEED;
            bubble->color = RGB(rand() % 256, rand() % 256, rand() % 256);
            
            g_game.aliveBubbles++;
            return true;
        }
    }

    return false;
}

static void moveBubble(void) {
    Bubble* bubble;
    int i;

    for (i = 0; i < BUBBLE_MAXCOUNT; ++i) {
        if (g_game.bubbles[i].alive == true) {
            bubble = &g_game.bubbles[i];
            bubble->y += bubble->dy;
            if (bubble->y + BUBBLE_RADIUS >= WINDOW_HEIGHT) {
                bubble->alive = false;
                g_game.aliveBubbles--;
                if (g_game.life > 0) {
                    g_game.life--;
                }
            }
        }
    }
}

static void deleteBubble(Point* mouse) {
    Bubble* bubble;
    int i;
    qword distance;

    // loop backward to delete the top bubble.
    for (i = BUBBLE_MAXCOUNT - 1; i >= 0; --i) {
        if (g_game.bubbles[i].alive == true) {
            bubble = &g_game.bubbles[i];
            distance = ((mouse->x - bubble->x) * (mouse->x - bubble->x)) + ((mouse->y - bubble->y) * (mouse->y - bubble->y));
            if (distance < BUBBLE_RADIUS * BUBBLE_RADIUS) {
                bubble->alive = false;
                g_game.aliveBubbles--;
                g_game.score++;
                break;
            }
        }
    }
}

void drawInfo(qword windowId) {
    char buffer[200];
    int len;
    
    drawRect(windowId, 1, WINDOW_TITLEBAR_HEIGHT - 1, WINDOW_WIDTH - 2, WINDOW_TITLEBAR_HEIGHT + INFO_HEIGHT, COLOR_INFOBACKGROUND, true);
    sprintf(buffer, "Life: %d  Score: %d\n", g_game.life, g_game.score);
    len = strlen(buffer);
    drawText(windowId, (WINDOW_WIDTH - len * FONT_DEFAULT_WIDTH) / 2, WINDOW_TITLEBAR_HEIGHT + 2, RGB(255, 255, 255), COLOR_INFOBACKGROUND, buffer, len);
}

void drawGame(qword windowId, Point* mouse) {
    Bubble* bubble;
    int i;

    // draw background.
    drawRect(windowId, 0, WINDOW_TITLEBAR_HEIGHT + INFO_HEIGHT, WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1, COLOR_GAMEBACKGROUND, true);
    
    // draw bubbles.
    for (i = 0; i < BUBBLE_MAXCOUNT; ++i) {
        if (g_game.bubbles[i].alive == true) {
            bubble = &g_game.bubbles[i];
            drawCircle(windowId, bubble->x, bubble->y, BUBBLE_RADIUS, bubble->color, true);
            drawCircle(windowId, bubble->x, bubble->y, BUBBLE_RADIUS, ~bubble->color, false);
        }
    }

    if (mouse->y < WINDOW_TITLEBAR_HEIGHT + BUBBLE_RADIUS) {
        mouse->y = WINDOW_TITLEBAR_HEIGHT + BUBBLE_RADIUS;
    }

    // draw cross-hair.
    drawLine(windowId, mouse->x, mouse->y - BUBBLE_RADIUS, mouse->x, mouse->y + BUBBLE_RADIUS, COLOR_CROSSHAIR);
    drawLine(windowId, mouse->x - BUBBLE_RADIUS, mouse->y, mouse->x + BUBBLE_RADIUS, mouse->y, COLOR_CROSSHAIR);

    // draw border.
    drawRect(windowId, 0, WINDOW_TITLEBAR_HEIGHT + INFO_HEIGHT, WINDOW_WIDTH - 1, WINDOW_HEIGHT - 1, COLOR_GAMEBORDER, false);
}