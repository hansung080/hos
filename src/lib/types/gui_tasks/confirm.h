#ifndef __GUITASKS_CONFIRM_H__
#define __GUITASKS_CONFIRM_H__

typedef void (*ConfirmOkFunc)(void);

#pragma pack(push, 1)

typedef struct k_ConfirmArg {
    const char* const msg;
    const ConfirmOkFunc const okFunc;
} ConfirmArg;

#pragma pack(pop)

#endif // __GUITASKS_CONFIRM_H__