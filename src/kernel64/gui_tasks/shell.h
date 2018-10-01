#ifndef __GUITASKS_SHELL_H__
#define __GUITASKS_SHELL_H__

#include "../core/types.h"

void k_guiShellTask(void);
static void k_processConsoleScreenBuffer(qword windowId);

extern volatile qword g_guiShellWindowId;

#endif // __GUITASKS_SHELL_H__