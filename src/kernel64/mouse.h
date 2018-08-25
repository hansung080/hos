#ifndef __MOUSE_H__
#define __MOUSE_H__

#include "types.h"
#include "sync.h"

// mouse queue-related macros
#define MOUSE_MAXQUEUECOUNT 100

// button status
#define MOUSE_LBUTTONDOWN 0x01 // left button down
#define MOUSE_RBUTTONDOWN 0x02 // right button down
#define MOUSE_MBUTTONDOWN 0x04 // middle button down

#pragma pack(push, 1)

/**
  < PS/2 Mouse Packet Structure >
               bit 7        bit 6      bit 5    bit 4       bit 3           bit 2           bit 1         bit 0
           ----------------------------------------------------------------------------------------------------------
    byte 1 | y overflow | x overflow | y sign | x sign | reserved as 1 | middle button | right button | left button |
           ----------------------------------------------------------------------------------------------------------
    byte 2 |                                              x movement                                                |
           ----------------------------------------------------------------------------------------------------------
    byte 3 |                                              y movement                                                |
           ----------------------------------------------------------------------------------------------------------
    
    - left button: status of left button, 0: up, 1: down
    - right button: status of right button, 0: up, 1: down
    - middle button: status of middle button, 0: up, 1: down
    - x sign: sign bit of x movement, 0: plus, 1: minus
    - y sign: sign bit of y movement, 0: plus, 1: minus
    - x overflow: overflow of x movement, 0: not overflow, 1: overflow (more than 256)
    - y overflow: overflow of y movement, 0: not overflow, 1: overflow (more than 256)
    - x movement: x movement from last packet, x sign and x movement represents 2^9 (-256 ~ +255).
    - y movement: y movement form last packet, y sign and y movement represents 2^9 (-256 ~ +255).
*/
typedef struct k_MouseData {
	byte flagsAndButtonStatus;
	byte xMovement;
	byte yMovement;
} MouseData;

typedef struct k_MouseManager {
	Spinlock spinlock;     // spinlock 
	int byteCount;         // received byte count: Mouse data is 3 bytes. Thus, byte count repeats 0 ~ 2.
	MouseData currentData; // current mouse data
} MouseManager;

#pragma pack(pop)

bool k_initMouse(void);
bool k_activateMouse(void);
void k_enableMouseInterrupt(void);
bool k_accumulateMouseDataAndPutQueue(byte mouseData);
bool k_getMouseDataFromMouseQueue(byte* buttonStatus, int* relativeX, int* relativeY);
bool k_isMouseDataInOutputBuffer(void);

#endif // __MOUSE_H__