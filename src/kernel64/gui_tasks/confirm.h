#ifndef __GUITASKS_CONFIRM_H__
#define __GUITASKS_CONFIRM_H__

#include "../core/types.h"
#include "../core/2d_graphics.h"

// title
#define CONFIRM_TITLE "Confirm"

// window size
#define CONFIRM_WIDTH  300
#define CONFIRM_HEIGHT 120

// button size
#define CONFIRM_BUTTON_WIDTH  60
#define CONFIRM_BUTTON_HEIGHT 20

// button color
#define CONFIRM_COLOR_BUTTONTEXT               RGB(0, 0, 0)
#define CONFIRM_COLOR_BUTTONBACKGROUNDACTIVE   RGB(109, 213, 237)
#define CONFIRM_COLOR_BUTTONBACKGROUNDINACTIVE RGB(255, 255, 255)

typedef void (*ConfirmOkFunc)(void);

#pragma pack(push, 1)

typedef struct k_ConfirmArg {
    const char* const msg;
    const ConfirmOkFunc const okFunc;
} ConfirmArg;

typedef struct k_Confirm {
    const ConfirmArg* const arg;
    qword windowId;
    Rect okButton;
    Rect cancelButton;
    bool ok;
} Confirm;

#pragma pack(pop)

void k_confirm(const ConfirmArg* arg);
static void k_confirmTask(qword _arg);
static bool k_processConfirmEvent(Confirm* confirm);
static void k_drawConfirmButtons(const Confirm* confirm);

#endif // __GUITASKS_CONFIRM_H__