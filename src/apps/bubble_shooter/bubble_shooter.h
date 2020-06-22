#ifndef __BUBBLESHOOTER_H__
#define __BUBBLESHOOTER_H__

#include <hlib.h>

#define TITLE "Bubble Shooter"

// window size
#define WINDOW_WIDTH  250
#define WINDOW_HEIGHT 350
#define INFO_HEIGHT   20

// bubble-related
#define BUBBLE_MAXCOUNT     50
#define BUBBLE_RADIUS       16
#define BUBBLE_DEFAULTSPEED 3

// player-related
#define PLAYER_MAXLIFE 20

// color
#define COLOR_INFOBACKGROUND RGB(33, 147, 176)
#define COLOR_GAMEBACKGROUND RGB(0, 0, 0)
#define COLOR_GAMEBORDER     RGB(33, 147, 176)
#define COLOR_CROSSHAIR      RGB(255, 0, 0)

// game message
#define GAME_START_MESSAGE "Press Enter key to start~!!"
#define GAME_END_MESSAGE   "Game Over~!!"

typedef struct __Bubble {
    qword x;
    qword y;
    qword dy;
    Color color;
    bool alive;
} Bubble;

typedef struct __Game {
    Bubble* bubbles;
    int aliveBubbles;
    int life;
    qword score;
    bool started;
    qword lastDrawing;
} Game;

bool initGame(void);
bool processEvent(qword windowId, Point* mouse);
void processGame(qword windowId, Point* mouse);
static bool createBubble(void);
static void moveBubble(void);
static void deleteBubble(Point* mouse);
void drawInfo(qword windowId);
void drawGame(qword windowId, Point* mouse);

#endif // __BUBBLESHOOTER_H__