#ifndef __GUITASKS_PROTOTYPE_H__
#define __GUITASKS_PROTOTYPE_H__

#include "../core/types.h"
#include "../core/widgets.h"
#include "../utils/queue.h"

#define PROTOTYPE_TOPMENU_ANIMALS 0
#define PROTOTYPE_TOPMENU_FRUITS  1
#define PROTOTYPE_TOPMENU_ROBOT   2

#if __DEBUG__
void k_prototypeTask(void);
static bool k_createPrototypeMenus(Menu* topMenu, Menu* animalsMenu, Menu* fruitsMenu, Menu* mammalsMenu, Menu* reptilesMenu, Menu* amphibiansMenu, Menu* dogsMenu, Menu* catsMenu, qword parentId);
static bool k_processPrototypeEvent(qword windowId, const Epoll* epoll);
static void k_drawPrototypeMessage(qword windowId, const char* message);

/* Animals Menu Functions */
static void k_funcBird(qword parentId);
static void k_funcFish(qword parentId);

/* Fruits Menu Functions */
static void k_funcApple(qword parentId);
static void k_funcBanana(qword parentId);
static void k_funcStrawberry(qword parentId);
static void k_funcGrape(qword parentId);

/* Mammals Menu Functions */
static void k_funcMonkey(qword parentId);
static void k_funcLion(qword parentId);
static void k_funcTiger(qword parentId);

/* Reptiles Menu Functions */
static void k_funcLizard(qword parentId);
static void k_funcChameleon(qword parentId);
static void k_funcIguana(qword parentId);

/* Amphibians Menu Functions */
static void k_funcFrog(qword parentId);
static void k_funcToad(qword parentId);
static void k_funcSalamander(qword parentId);

/* Dogs Menu Functions */
static void k_funcMaltese(qword parentId);
static void k_funcPoodle(qword parentId);
static void k_funcShihtzu(qword parentId);

/* Cats Menu Functions */
static void k_funcKoreanShorthair(qword parentId);
static void k_funcPersian(qword parentId);
static void k_funcRussianBlue(qword parentId);

#endif // __DEBUG__

#endif // __GUITASKS_PROTOTYPE_H__