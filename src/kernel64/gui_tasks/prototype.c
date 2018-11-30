#include "prototype.h"
#include "../core/window.h"
#include "../utils/util.h"
#include "../core/console.h"
#include "../fonts/fonts.h"
#include "../core/task.h"
#include "../core/multiprocessor.h"

#if __DEBUG__
void k_prototypeTask(void) {
	MenuItem topMenuTable[] = {
		{"Animals", null, true},
		{"Fruits", null, true},
		{"Robot", null, false}
	};

	MenuItem animalsMenuTable[] = {
		{"Mammals >", null, true},
		{"Bird", k_funcBird, false},
		{"Fish", k_funcFish, false},
		{"Reptiles >", null, true},
		{"Amphibians >", null, true}
	};

	MenuItem fruitsMenuTable[] = {
		{"Apple", k_funcApple, false},
		{"Banana", k_funcBanana, false},
		{"Strawberry", k_funcStrawberry, false},
		{"Grape", k_funcGrape, false}
	};

	MenuItem mammalsMenuTable[] = {
		{"Monkey", k_funcMonkey, false},
		{"Dogs >", null, true}, 
		{"Cats >", null, true},
		{"Lion", k_funcLion, false},
		{"Tiger", k_funcTiger, false}
	};

	MenuItem reptilesMenuTable[] = {
		{"Lizard", k_funcLizard, false},
		{"Chameleon", k_funcChameleon, false},
		{"Iguana", k_funcIguana, false}
	};

	MenuItem amphibiansMenuTable[] = {
		{"Frog", k_funcFrog, false},
		{"Toad", k_funcToad, false},
		{"Salamander", k_funcSalamander, false}
	};

	MenuItem dogsMenuTable[] = {
		{"Maltese", k_funcMaltese, false},
		{"Poodle", k_funcPoodle, false},
		{"Shihtzu", k_funcShihtzu, false}
	};

	MenuItem catsMenuTable[] = {
		{"Korean Shorthair", k_funcKoreanShorthair, false},
		{"Persian", k_funcPersian, false},
		{"Russian Blue", k_funcRussianBlue, false}
	};

	Menu topMenu = {topMenuTable, sizeof(topMenuTable) / sizeof(MenuItem)};
	Menu animalsMenu = {animalsMenuTable, sizeof(animalsMenuTable) / sizeof(MenuItem)};
	Menu fruitsMenu = {fruitsMenuTable, sizeof(fruitsMenuTable) / sizeof(MenuItem)};
	Menu mammalsMenu = {mammalsMenuTable, sizeof(mammalsMenuTable) / sizeof(MenuItem)};
	Menu reptilesMenu = {reptilesMenuTable, sizeof(reptilesMenuTable) / sizeof(MenuItem)};
	Menu amphibiansMenu = {amphibiansMenuTable, sizeof(amphibiansMenuTable) / sizeof(MenuItem)};
	Menu dogsMenu = {dogsMenuTable, sizeof(dogsMenuTable) / sizeof(MenuItem)};
	Menu catsMenu = {catsMenuTable, sizeof(catsMenuTable) / sizeof(MenuItem)};
	int mouseX, mouseY;
	qword windowId;
	Epoll epoll;
	Queue* equeues[8];

	/* check graphic mode */
	if (k_isGraphicMode() == false) {
		k_printf("[prototype error] not graphic mode\n");
		return;
	}

	/* create window */
	k_getMouseCursorPos(&mouseX, &mouseY);
	windowId = k_createWindow(mouseX + 50, mouseY + 50, 400, 200, WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_RESIZABLE, "Prototype", WINDOW_COLOR_BACKGROUND, &topMenu, WINDOW_INVALIDID);
	if (windowId == WINDOW_INVALIDID) {
		k_printf("[prototype error] window creation failure\n");
		return;
	}

	/* create prototype menus */
	k_createPrototypeMenus(&topMenu, &animalsMenu, &fruitsMenu, &mammalsMenu, &reptilesMenu, &amphibiansMenu, &dogsMenu, &catsMenu, windowId);

	/* initialize epoll */
	equeues[0] = &k_getWindow(windowId)->eventQueue;
	equeues[1] = &k_getWindow(animalsMenu.id)->eventQueue;
	equeues[2] = &k_getWindow(fruitsMenu.id)->eventQueue;
	equeues[3] = &k_getWindow(mammalsMenu.id)->eventQueue;
	equeues[4] = &k_getWindow(reptilesMenu.id)->eventQueue;
	equeues[5] = &k_getWindow(amphibiansMenu.id)->eventQueue;
	equeues[6] = &k_getWindow(dogsMenu.id)->eventQueue;
	equeues[7] = &k_getWindow(catsMenu.id)->eventQueue;
	k_initEpoll(&epoll, equeues, 8);

	/* event processing loop */
	while (true) {
		k_waitEpoll(&epoll, null);
		k_processPrototypeEvent(windowId, &epoll);
		k_processMenuEvent(&animalsMenu);
		k_processMenuEvent(&fruitsMenu);
		k_processMenuEvent(&mammalsMenu);
		k_processMenuEvent(&reptilesMenu);
		k_processMenuEvent(&amphibiansMenu);
		k_processMenuEvent(&dogsMenu);
		k_processMenuEvent(&catsMenu);
	}

	k_closeEpoll(&epoll);
}

static bool k_createPrototypeMenus(Menu* topMenu, Menu* animalsMenu, Menu* fruitsMenu, Menu* mammalsMenu, Menu* reptilesMenu, Menu* amphibiansMenu, Menu* dogsMenu, Menu* catsMenu, qword parentId) {
	Rect itemArea;

	k_convertRectWindowToScreen(topMenu->id, &topMenu->table[0].area, &itemArea);
	if (k_createMenu(animalsMenu, itemArea.x1, itemArea.y2 + 1, MENU_ITEMHEIGHT_NORMAL, null, parentId, topMenu, 0) == false) {
		k_printf("[prototype error] animals menu creation failure\n");
		return false;
	}

	k_convertRectWindowToScreen(topMenu->id, &topMenu->table[1].area, &itemArea);
	if (k_createMenu(fruitsMenu, itemArea.x1, itemArea.y2 + 1, MENU_ITEMHEIGHT_NORMAL, null, parentId, topMenu, 0) == false) {
		k_printf("[prototype error] fruits menu creation failure\n");
		return false;
	}

	k_convertRectWindowToScreen(animalsMenu->id, &animalsMenu->table[0].area, &itemArea);
	if (k_createMenu(mammalsMenu, itemArea.x2 + 1, itemArea.y1, MENU_ITEMHEIGHT_NORMAL, null, parentId, topMenu, 0) == false) {
		k_printf("[prototype error] mammals menu creation failure\n");
		return false;
	}

	k_convertRectWindowToScreen(animalsMenu->id, &animalsMenu->table[3].area, &itemArea);
	if (k_createMenu(reptilesMenu, itemArea.x2 + 1, itemArea.y1, MENU_ITEMHEIGHT_NORMAL, null, parentId, topMenu, 0) == false) {
		k_printf("[prototype error] reptiles menu creation failure\n");
		return false;
	}

	k_convertRectWindowToScreen(animalsMenu->id, &animalsMenu->table[4].area, &itemArea);
	if (k_createMenu(amphibiansMenu, itemArea.x2 + 1, itemArea.y1, MENU_ITEMHEIGHT_NORMAL, null, parentId, topMenu, 0) == false) {
		k_printf("[prototype error] amphibians menu creation failure\n");
		return false;
	}	

	k_convertRectWindowToScreen(mammalsMenu->id, &mammalsMenu->table[1].area, &itemArea);
	if (k_createMenu(dogsMenu, itemArea.x2 + 1, itemArea.y1, MENU_ITEMHEIGHT_NORMAL, null, parentId, topMenu, 0) == false) {
		k_printf("[prototype error] dogs menu creation failure\n");
		return false;
	}

	k_convertRectWindowToScreen(mammalsMenu->id, &mammalsMenu->table[2].area, &itemArea);
	if (k_createMenu(catsMenu, itemArea.x2 + 1, itemArea.y1, MENU_ITEMHEIGHT_NORMAL, null, parentId, topMenu, 0) == false) {
		k_printf("[prototype error] cats menu creation failure\n");
		return false;
	}	

	topMenu->table[0].param = (qword)animalsMenu;
	topMenu->table[1].param = (qword)fruitsMenu;
	animalsMenu->table[0].param = (qword)mammalsMenu;
	animalsMenu->table[3].param = (qword)reptilesMenu;
	animalsMenu->table[4].param = (qword)amphibiansMenu;
	mammalsMenu->table[1].param = (qword)dogsMenu;
	mammalsMenu->table[2].param = (qword)catsMenu;

	return true;
}

static bool k_processPrototypeEvent(qword windowId, const Epoll* epoll) {
	Event event;
	MenuEvent* menuEvent;
	const char* message;

	while(true) {
		if (k_recvEventFromWindow(&event, windowId) == false) {
			return false;
		}

		switch (event.type) {
		case EVENT_WINDOW_CLOSE:
			k_deleteWindow(windowId);
			k_closeEpoll(epoll);
			k_exitTask();
			return true;

		case EVENT_TOPMENU_CLICK:
			menuEvent = &event.menuEvent;

			if (menuEvent->index == PROTOTYPE_TOPMENU_ROBOT) {
				message = "Robot was selected.";
				k_drawPrototypeMessage(windowId, message);
			}

			break;
		}
	}

	return true;
}

static void k_drawPrototypeMessage(qword windowId, const char* message) {
	int len;
	Rect area;
	int width;
	int height;
	Rect updateArea;

	len = k_strlen(message);
	k_getWindowArea(windowId, &area);
	width = k_getRectWidth(&area);
	height = k_getRectHeight(&area);

	k_setRect(&updateArea, (width - FONT_DEFAULT_WIDTH * len) / 2, (height - FONT_DEFAULT_HEIGHT) / 2, ((width - FONT_DEFAULT_WIDTH * len) / 2) + FONT_DEFAULT_WIDTH * len, ((height - FONT_DEFAULT_HEIGHT) / 2) + FONT_DEFAULT_HEIGHT);
	k_drawText(windowId, updateArea.x1, updateArea.y1, RGB(0, 0, 255), WINDOW_COLOR_BACKGROUND, message, len);
	k_updateScreenByWindowArea(windowId, &updateArea);

	k_sleep(1000);
	k_drawRect(windowId, updateArea.x1, updateArea.y1, updateArea.x2, updateArea.y2, WINDOW_COLOR_BACKGROUND, true);
	k_updateScreenByWindowArea(windowId, &updateArea);
}

static void k_funcBird(qword parentId) {
	const char* message = "Bird was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcFish(qword parentId) {
	const char* message = "Fish was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcApple(qword parentId) {
	const char* message = "Apple was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcBanana(qword parentId) {
	const char* message = "Banana was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcStrawberry(qword parentId) {
	const char* message = "Strawberry was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcGrape(qword parentId) {
	const char* message = "Grape was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcMonkey(qword parentId) {
	const char* message = "Monkey was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcLion(qword parentId) {
	const char* message = "Lion was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcTiger(qword parentId) {
	const char* message = "Tiger was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcLizard(qword parentId) {
	const char* message = "Lizard was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcChameleon(qword parentId) {
	const char* message = "Chameleon was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcIguana(qword parentId) {
	const char* message = "Iguana was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcFrog(qword parentId) {
	const char* message = "Frog was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcToad(qword parentId) {
	const char* message = "Toad was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcSalamander(qword parentId) {
	const char* message = "Salamander was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcMaltese(qword parentId) {
	const char* message = "Maltese was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcPoodle(qword parentId) {
	const char* message = "Poodle was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcShihtzu(qword parentId) {
	const char* message = "Shihtzu was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcKoreanShorthair(qword parentId) {
	const char* message = "Korean Shorthair was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcPersian(qword parentId) {
	const char* message = "Persian was selected.";

	k_drawPrototypeMessage(parentId, message);
}

static void k_funcRussianBlue(qword parentId) {
	const char* message = "Russian Blue was selected.";

	k_drawPrototypeMessage(parentId, message);
}

#endif // __DEBUG__