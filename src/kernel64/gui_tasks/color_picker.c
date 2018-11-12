#include "color_picker.h"
#include "../core/window.h"
#include "../utils/util.h"
#include "../core/console.h"
#include "../fonts/fonts.h"

void k_colorPickerTask(void) {
	ColorPicker cp;
	Rect screenArea;
	int y;
	Event event;
	MouseEvent* mouseEvent;
	KeyEvent* keyEvent;

	/* check graphic mode */
	if (k_isGraphicMode() == false) {
		k_printf("[color picker error] not graphic mode\n");
		return;
	}

	/* create window */
	k_getScreenArea(&screenArea);
	cp.windowId = k_createWindow(screenArea.x2 - COLORPICKER_WIDTH, screenArea.y1 + WINDOW_APPPANEL_HEIGHT, COLORPICKER_WIDTH, COLORPICKER_HEIGHT, WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_BLOCKING, "Color Picker");
	if (cp.windowId == WINDOW_INVALIDID) {
		return;
	}

	k_initColorPicker(&cp);

	/* draw color picker */
	// draw selected color.
	y = WINDOW_TITLEBAR_HEIGHT + 10;
	k_drawText(cp.windowId, 130, y, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, "Selected Color", 14);
	k_setRect(&cp.selArea, 100, y + 20, 270, y + 50);
	k_drawRect(cp.windowId, cp.selArea.x1 - 1, cp.selArea.y1 - 1, cp.selArea.x2 + 1, cp.selArea.y2 + 1, RGB(0, 0, 0), false);
	k_updateSelectedColor(&cp, RGB(cp.red, cp.green, cp.blue));
	y += 80;

	// draw color picker (Red, Green).
	k_drawText(cp.windowId, 60, y, RGB(255, 0, 0), WINDOW_COLOR_BACKGROUND, "0", 1);
	k_drawText(cp.windowId, 60 + 127, y, RGB(255, 0, 0), WINDOW_COLOR_BACKGROUND, "Red", 3);
	k_drawText(cp.windowId, 60 + 255 - FONT_DEFAULT_WIDTH * 3, y, RGB(255, 0, 0), WINDOW_COLOR_BACKGROUND, "255", 3);
	y += 20;
	k_drawText(cp.windowId, 10 + FONT_DEFAULT_WIDTH * 4, y, RGB(0, 255, 0), WINDOW_COLOR_BACKGROUND, "0", 1);
	k_drawText(cp.windowId, 10, y + 127, RGB(0, 255, 0), WINDOW_COLOR_BACKGROUND, "Green", 5);
	k_drawText(cp.windowId, 10 + FONT_DEFAULT_WIDTH * 2, y + 255 - FONT_DEFAULT_HEIGHT, RGB(0, 255, 0), WINDOW_COLOR_BACKGROUND, "255", 3);
	
	k_setRect(&cp.rgArea, 60, y, 60 + 255, y + 255);
	k_drawRect(cp.windowId, cp.rgArea.x1 - 1, cp.rgArea.y1 - 1, cp.rgArea.x2 + 1, cp.rgArea.y2 + 1, RGB(0, 0, 0), false);
	k_drawRedGreenByBlue(&cp, cp.blue);
	k_updateRGPicker(&cp, cp.red, cp.green, cp.blue, true);

	// draw color picker (Blue).
	k_drawText(cp.windowId, 370 - FONT_DEFAULT_WIDTH, y - 20, RGB(0, 0, 255), WINDOW_COLOR_BACKGROUND, "Blue", 4);
	k_setRect(&cp.bArea, 370, y, 370 + 10, y + 255);
	k_drawRect(cp.windowId, cp.bArea.x1 - 1, cp.bArea.y1 -1, cp.bArea.x2 + 1, cp.bArea.y2 + 1, RGB(0, 0, 0), false);
	k_drawBlue(&cp);
	k_updateBPicker(&cp, cp.blue, true);
	k_drawText(cp.windowId, 388, y, RGB(0, 0, 255), WINDOW_COLOR_BACKGROUND, "0", 1);
	k_drawText(cp.windowId, 388, y + 255 - FONT_DEFAULT_HEIGHT, RGB(0, 0, 255), WINDOW_COLOR_BACKGROUND, "255", 3);
	y += 280;

	// draw value picker (RGB).
	k_drawText(cp.windowId, 100, y, RGB(255, 0, 0), WINDOW_COLOR_BACKGROUND, "R", 1);
	k_setRect(&cp.rvalArea, 115, y, 115 + FONT_DEFAULT_WIDTH * 3, y + FONT_DEFAULT_HEIGHT);
	k_drawRect(cp.windowId, cp.rvalArea.x1 - 1, cp.rvalArea.y1 - 1, cp.rvalArea.x2 + 1, cp.rvalArea.y2 + 1, COLORPICKER_COLOR_VALAREABORDER, false);

	k_drawText(cp.windowId, 160, y, RGB(0, 255, 0), WINDOW_COLOR_BACKGROUND, "G", 1);
	k_setRect(&cp.gvalArea, 175, y, 175 + FONT_DEFAULT_WIDTH * 3, y + FONT_DEFAULT_HEIGHT);	
	k_drawRect(cp.windowId, cp.gvalArea.x1 - 1, cp.gvalArea.y1 - 1, cp.gvalArea.x2 + 1, cp.gvalArea.y2 + 1, COLORPICKER_COLOR_VALAREABORDER, false);
	k_updateRGValue(&cp, cp.red, cp.green);

	k_drawText(cp.windowId, 220, y, RGB(0, 0, 255), WINDOW_COLOR_BACKGROUND, "B", 1);
	k_setRect(&cp.bvalArea, 235, y, 235 + FONT_DEFAULT_WIDTH * 3, y + FONT_DEFAULT_HEIGHT);
	k_drawRect(cp.windowId, cp.bvalArea.x1 - 1, cp.bvalArea.y1 - 1, cp.bvalArea.x2 + 1, cp.bvalArea.y2 + 1, COLORPICKER_COLOR_VALAREABORDER, false);
	k_updateBValue(&cp, cp.blue);

	k_setRect(&cp.rupdArea, cp.rvalArea.x1 - 2, cp.rvalArea.y1 - 2, cp.rvalArea.x2 + 2, cp.rvalArea.y2 + 2);
	k_setRect(&cp.gupdArea, cp.gvalArea.x1 - 2, cp.gvalArea.y1 - 2, cp.gvalArea.x2 + 2, cp.gvalArea.y2 + 2);
	k_setRect(&cp.bupdArea, cp.bvalArea.x1 - 2, cp.bvalArea.y1 - 2, cp.bvalArea.x2 + 2, cp.bvalArea.y2 + 2);

	k_showWindow(cp.windowId, true);

	/* event processing loop */
	while (true) {
		if (k_recvEventFromWindow(&event, cp.windowId) == false) {
			k_sleep(0);
			continue;
		}

		switch (event.type) {
		case EVENT_MOUSE_LBUTTONDOWN:
			mouseEvent = &event.mouseEvent;

			if (cp.rval != -1 && cp.rval != cp.red) {
				k_processRedGreenEvent(&cp, cp.rval, cp.green);
			}

			if (cp.gval != -1 && cp.gval != cp.green) {
				k_processRedGreenEvent(&cp, cp.red, cp.gval);
			}

			if (cp.bval != -1 && cp.bval != cp.blue) {
				k_processBlueEvent(&cp, cp.bval);
			}

			if (k_isPointInRect(&cp.rgArea, mouseEvent->point.x, mouseEvent->point.y) == true) {
				cp.focus = COLORPICKER_FOCUS_RGAREA;
				k_processRedGreenEvent(&cp, mouseEvent->point.x - cp.rgArea.x1, mouseEvent->point.y - cp.rgArea.y1);

			} else if (k_isPointInRect(&cp.bArea, mouseEvent->point.x, mouseEvent->point.y) == true) {
				cp.focus = COLORPICKER_FOCUS_BAREA;
				k_processBlueEvent(&cp, mouseEvent->point.y - cp.bArea.y1);

			} else if (k_isPointInRect(&cp.rvalArea, mouseEvent->point.x, mouseEvent->point.y) == true) {
				cp.focus = COLORPICKER_FOCUS_RVALAREA;
				k_processFocus(&cp, COLORPICKER_FOCUS_RVALAREA, true);

			} else if (k_isPointInRect(&cp.gvalArea, mouseEvent->point.x, mouseEvent->point.y) == true) {
				cp.focus = COLORPICKER_FOCUS_GVALAREA;
				k_processFocus(&cp, COLORPICKER_FOCUS_GVALAREA, true);

			} else if (k_isPointInRect(&cp.bvalArea, mouseEvent->point.x, mouseEvent->point.y) == true) {
				cp.focus = COLORPICKER_FOCUS_BVALAREA;
				k_processFocus(&cp, COLORPICKER_FOCUS_BVALAREA, true);

			} else {
				cp.focus = COLORPICKER_FOCUS_DEFAULT;
			}

			if (cp.focus != COLORPICKER_FOCUS_RVALAREA) {
				k_processFocus(&cp, COLORPICKER_FOCUS_RVALAREA, false);
			}

			if (cp.focus != COLORPICKER_FOCUS_GVALAREA) {
				k_processFocus(&cp, COLORPICKER_FOCUS_GVALAREA, false);
			}

			if (cp.focus != COLORPICKER_FOCUS_BVALAREA) {
				k_processFocus(&cp, COLORPICKER_FOCUS_BVALAREA, false);
			}

			break;

		case EVENT_WINDOW_CLOSE:
			k_deleteWindow(cp.windowId);
			return;

		case EVENT_KEY_DOWN:
			keyEvent = &event.keyEvent;

			switch (cp.focus) {
			case COLORPICKER_FOCUS_RGAREA: 
				switch (keyEvent->asciiCode) {
				case KEY_RIGHT:
					k_processRedGreenEvent(&cp, cp.red + 1, cp.green);
					break;

				case KEY_LEFT:
					k_processRedGreenEvent(&cp, cp.red - 1, cp.green);
					break;

				case KEY_DOWN:
					k_processRedGreenEvent(&cp, cp.red, cp.green + 1);
					break;

				case KEY_UP:
					k_processRedGreenEvent(&cp, cp.red, cp.green - 1);
					break;
				}

				break;

			case COLORPICKER_FOCUS_BAREA:
				switch (keyEvent->asciiCode) {
				case KEY_DOWN:
					k_processBlueEvent(&cp, cp.blue + 1);
					break;

				case KEY_UP:
					k_processBlueEvent(&cp, cp.blue - 1);
					break;
				}

				break;

			case COLORPICKER_FOCUS_RVALAREA:
				switch (keyEvent->asciiCode) {
				case KEY_UP:
					if (cp.rval != -1 && cp.rval != cp.red) {
						k_processRedGreenEvent(&cp, cp.rval + 1, cp.green);

					} else {
						k_processRedGreenEvent(&cp, cp.red + 1, cp.green);
					}
					
					break;

				case KEY_DOWN:
					if (cp.rval != -1 && cp.rval != cp.red) {
						k_processRedGreenEvent(&cp, cp.rval - 1, cp.green);

					} else {
						k_processRedGreenEvent(&cp, cp.red - 1, cp.green);
					}
					
					break;

				case KEY_ENTER:
					if (cp.rval != -1 && cp.rval != cp.red) {
						k_processRedGreenEvent(&cp, cp.rval, cp.green);
					}

					k_processFocus(&cp, COLORPICKER_FOCUS_RVALAREA, false);
					cp.focus = COLORPICKER_FOCUS_DEFAULT;
					break;

				default:
					if ('0' <= keyEvent->asciiCode && keyEvent->asciiCode <= '9' && cp.index < 3) {
						cp.value[cp.index++] = keyEvent->asciiCode;
						cp.rval = k_atoi10(cp.value);
						k_drawText(cp.windowId, cp.rvalArea.x1, cp.rvalArea.y1, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, cp.value, 3);
						k_updateScreenByWindowArea(cp.windowId, &cp.rvalArea);

					} else if (keyEvent->asciiCode == KEY_BACKSPACE && cp.index > 0) {
						cp.value[--cp.index] = '\0';
						cp.rval = k_atoi10(cp.value);
						k_drawText(cp.windowId, cp.rvalArea.x1, cp.rvalArea.y1, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, cp.value, 3);
						k_updateScreenByWindowArea(cp.windowId, &cp.rvalArea);
					}

					break;
				}

				break;

			case COLORPICKER_FOCUS_GVALAREA:
				switch (keyEvent->asciiCode) {
				case KEY_UP:
					if (cp.gval != -1 && cp.gval != cp.green) {
						k_processRedGreenEvent(&cp, cp.red, cp.gval + 1);

					} else {
						k_processRedGreenEvent(&cp, cp.red, cp.green + 1);
					}
					
					break;

				case KEY_DOWN:
					if (cp.gval != -1 && cp.gval != cp.green) {
						k_processRedGreenEvent(&cp, cp.red, cp.gval - 1);

					} else {
						k_processRedGreenEvent(&cp, cp.red, cp.green - 1);
					}

					break;
					
				case KEY_ENTER:
					if (cp.gval != -1 && cp.gval != cp.green) {
						k_processRedGreenEvent(&cp, cp.red, cp.gval);
					}
					
					k_processFocus(&cp, COLORPICKER_FOCUS_GVALAREA, false);
					cp.focus = COLORPICKER_FOCUS_DEFAULT;
					break;

				default:
					if ('0' <= keyEvent->asciiCode && keyEvent->asciiCode <= '9' && cp.index < 3) {
						cp.value[cp.index++] = keyEvent->asciiCode;
						cp.gval = k_atoi10(cp.value);
						k_drawText(cp.windowId, cp.gvalArea.x1, cp.gvalArea.y1, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, cp.value, 3);
						k_updateScreenByWindowArea(cp.windowId, &cp.gvalArea);

					} else if (keyEvent->asciiCode == KEY_BACKSPACE && cp.index > 0) {
						cp.value[--cp.index] = '\0';
						cp.gval = k_atoi10(cp.value);
						k_drawText(cp.windowId, cp.gvalArea.x1, cp.gvalArea.y1, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, cp.value, 3);
						k_updateScreenByWindowArea(cp.windowId, &cp.gvalArea);
					}

					break;
				}

				break;

			case COLORPICKER_FOCUS_BVALAREA:
				switch (keyEvent->asciiCode) {
				case KEY_UP:
					if (cp.bval != -1 && cp.bval != cp.blue) {
						k_processBlueEvent(&cp, cp.bval + 1);

					} else {
						k_processBlueEvent(&cp, cp.blue + 1);
					}
					
					break;

				case KEY_DOWN:
					if (cp.bval != -1 && cp.bval != cp.blue) {
						k_processBlueEvent(&cp, cp.bval - 1);

					} else {
						k_processBlueEvent(&cp, cp.blue - 1);
					}
					
					break;
					
				case KEY_ENTER:
					if (cp.bval != -1 && cp.bval != cp.blue) {
						k_processBlueEvent(&cp, cp.bval);
					}

					k_processFocus(&cp, COLORPICKER_FOCUS_BVALAREA, false);
					cp.focus = COLORPICKER_FOCUS_DEFAULT;
					break;

				default:
					if ('0' <= keyEvent->asciiCode && keyEvent->asciiCode <= '9' && cp.index < 3) {
						cp.value[cp.index++] = keyEvent->asciiCode;
						cp.bval = k_atoi10(cp.value);
						k_drawText(cp.windowId, cp.bvalArea.x1, cp.bvalArea.y1, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, cp.value, 3);
						k_updateScreenByWindowArea(cp.windowId, &cp.bvalArea);

					} else if (keyEvent->asciiCode == KEY_BACKSPACE && cp.index > 0) {
						cp.value[--cp.index] = '\0';
						cp.bval = k_atoi10(cp.value);
						k_drawText(cp.windowId, cp.bvalArea.x1, cp.bvalArea.y1, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, cp.value, 3);
						k_updateScreenByWindowArea(cp.windowId, &cp.bvalArea);
					}

					break;
				}

				break;
			}

			break;
		}
	}
}

static void k_initColorPicker(ColorPicker* cp) {
	cp->red = 0;
	cp->green = 0;
	cp->blue = 0;
	k_memset(cp->value, '\0', sizeof(cp->value));
	cp->index = 0;
	cp->rval = -1;
	cp->gval = -1;
	cp->bval = -1;
	cp->focus = COLORPICKER_FOCUS_DEFAULT;
}

static void k_drawRedGreenByBlue(const ColorPicker* cp, int blue) {
	int red, green;

	for (red = 0; red <= 255; red++) {
		for (green = 0; green <= 255; green++) {
			k_drawPixel(cp->windowId, cp->rgArea.x1 + red, cp->rgArea.y1 + green, RGB(red, green, blue));
		}
	}
}

static void k_drawBlue(const ColorPicker* cp) {
	int blue;

	for (blue = 0; blue <= 255; blue++) {
		k_drawLine(cp->windowId, cp->bArea.x1, cp->bArea.y1 + blue, cp->bArea.x2, cp->bArea.y1 + blue, RGB(0, 0, blue));
	}
}

static void k_updateSelectedColor(const ColorPicker* cp, Color color) {
	k_drawRect(cp->windowId, cp->selArea.x1, cp->selArea.y1, cp->selArea.x2, cp->selArea.y2, color, true);
}

static void k_updateRGPicker(const ColorPicker* cp, int red, int green, int blue, bool new) {
	int i, j;

	for (i = -2; i <= 2; i++) {
		for (j = -2; j <= 2; j++) {
			if ((0 <= red + i) && (red + i <= 255) && (0 <= green + j) && (green + j <= 255)) {
				if (new == true) {
					if ((red >= 128) && (green >= 128)) {
						k_drawPixel(cp->windowId, cp->rgArea.x1 + red + i, cp->rgArea.y1 + green + j, RGB(0, 0, 0));

					} else {
						k_drawPixel(cp->windowId, cp->rgArea.x1 + red + i, cp->rgArea.y1 + green + j, RGB(255, 255, 255));
					}
					
				} else {
					k_drawPixel(cp->windowId, cp->rgArea.x1 + red + i, cp->rgArea.y1 + green + j, RGB(red + i, green + j, blue));
				}
				
			}
		}
	}
}

static void k_updateBPicker(const ColorPicker* cp, int blue, bool new) {
	int i;

	for (i = -1; i <= 1; i++) {
		if ((0 <= blue + i) && (blue + i <= 255)) {
			if (new == true) {
				k_drawLine(cp->windowId, cp->bArea.x1, cp->bArea.y1 + blue + i, cp->bArea.x2, cp->bArea.y1 + blue + i, RGB(255, 255, 255));	

			} else {
				k_drawLine(cp->windowId, cp->bArea.x1, cp->bArea.y1 + blue + i, cp->bArea.x2, cp->bArea.y1 + blue + i, RGB(0, 0, blue));
			}
		}
	}
}

static void k_updateRGValue(const ColorPicker* cp, int red, int green) {
	char red_[4] = {'\0', '\0', '\0', '\0'};
	char green_[4] = {'\0', '\0', '\0', '\0'};

	k_itoa10(red & 0xFF, red_);
	k_itoa10(green & 0xFF, green_);

	k_drawText(cp->windowId, cp->rvalArea.x1, cp->rvalArea.y1, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, red_, 3);
	k_drawText(cp->windowId, cp->gvalArea.x1, cp->gvalArea.y1, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, green_, 3);
}

static void k_updateBValue(const ColorPicker* cp, int blue) {
	char blue_[4] = {'\0', '\0', '\0', '\0'};

	k_itoa10(blue & 0xFF, blue_);

	k_drawText(cp->windowId, cp->bvalArea.x1, cp->bvalArea.y1, RGB(0, 0, 0), WINDOW_COLOR_BACKGROUND, blue_, 3);
}

static void k_processRedGreenEvent(ColorPicker* cp, int red, int green) {
	if (red < 0) {
		red = 0;

	} else if (red > 255) {
		red = 255;
	}

	if (green < 0) {
		green = 0;

	} else if (green > 255) {
		green = 255;
	}

	k_updateRGPicker(cp, cp->red, cp->green, cp->blue, false);

	cp->red = red;
	cp->green = green;
	
	k_updateRGValue(cp, cp->red, cp->green);
	k_updateRGPicker(cp, cp->red, cp->green, cp->blue, true);
	k_updateSelectedColor(cp, RGB(cp->red, cp->green, cp->blue));

	k_updateScreenById(cp->windowId);

	k_memset(cp->value, '\0', sizeof(cp->value));
	cp->index = 0;
	cp->rval = -1;
	cp->gval = -1;
}

static void k_processBlueEvent(ColorPicker* cp, int blue) {
	if (blue < 0) {
		blue = 0;

	} else if (blue > 255) {
		blue = 255;
	}

	k_updateBPicker(cp, cp->blue, false);

	cp->blue = blue;

	k_updateBValue(cp, cp->blue);
	k_updateBPicker(cp, cp->blue, true);
	k_drawRedGreenByBlue(cp, cp->blue);
	k_updateRGPicker(cp, cp->red, cp->green, cp->blue, true);
	k_updateSelectedColor(cp, RGB(cp->red, cp->green, cp->blue));

	k_updateScreenById(cp->windowId);

	k_memset(cp->value, '\0', sizeof(cp->value));
	cp->index = 0;
	cp->bval = -1;
}

static void k_processFocus(const ColorPicker* cp, int focus, bool on) {
	switch (focus) {
	case COLORPICKER_FOCUS_RVALAREA:
		if (on == true) {
			k_drawRect(cp->windowId, cp->rvalArea.x1 - 1, cp->rvalArea.y1 - 1, cp->rvalArea.x2 + 1, cp->rvalArea.y2 + 1, RGB(255, 0, 0), false);
			k_drawRect(cp->windowId, cp->rvalArea.x1 - 2, cp->rvalArea.y1 - 2, cp->rvalArea.x2 + 2, cp->rvalArea.y2 + 2, RGB(255, 0, 0), false);
			k_updateScreenByWindowArea(cp->windowId, &cp->rupdArea);

		} else {
			k_drawRect(cp->windowId, cp->rvalArea.x1 - 1, cp->rvalArea.y1 - 1, cp->rvalArea.x2 + 1, cp->rvalArea.y2 + 1, COLORPICKER_COLOR_VALAREABORDER, false);
			k_drawRect(cp->windowId, cp->rvalArea.x1 - 2, cp->rvalArea.y1 - 2, cp->rvalArea.x2 + 2, cp->rvalArea.y2 + 2, WINDOW_COLOR_BACKGROUND, false);
			k_updateScreenByWindowArea(cp->windowId, &cp->rupdArea);
		}

		break;

	case COLORPICKER_FOCUS_GVALAREA:
		if (on == true) {
			k_drawRect(cp->windowId, cp->gvalArea.x1 - 1, cp->gvalArea.y1 - 1, cp->gvalArea.x2 + 1, cp->gvalArea.y2 + 1, RGB(0, 255, 0), false);
			k_drawRect(cp->windowId, cp->gvalArea.x1 - 2, cp->gvalArea.y1 - 2, cp->gvalArea.x2 + 2, cp->gvalArea.y2 + 2, RGB(0, 255, 0), false);
			k_updateScreenByWindowArea(cp->windowId, &cp->gupdArea);

		} else {
			k_drawRect(cp->windowId, cp->gvalArea.x1 - 1, cp->gvalArea.y1 - 1, cp->gvalArea.x2 + 1, cp->gvalArea.y2 + 1, COLORPICKER_COLOR_VALAREABORDER, false);
			k_drawRect(cp->windowId, cp->gvalArea.x1 - 2, cp->gvalArea.y1 - 2, cp->gvalArea.x2 + 2, cp->gvalArea.y2 + 2, WINDOW_COLOR_BACKGROUND, false);
			k_updateScreenByWindowArea(cp->windowId, &cp->gupdArea);
		}

		break;

	case COLORPICKER_FOCUS_BVALAREA:
		if (on == true) {
			k_drawRect(cp->windowId, cp->bvalArea.x1 - 1, cp->bvalArea.y1 - 1, cp->bvalArea.x2 + 1, cp->bvalArea.y2 + 1, RGB(0, 0, 255), false);
			k_drawRect(cp->windowId, cp->bvalArea.x1 - 2, cp->bvalArea.y1 - 2, cp->bvalArea.x2 + 2, cp->bvalArea.y2 + 2, RGB(0, 0, 255), false);
			k_updateScreenByWindowArea(cp->windowId, &cp->bupdArea);

		} else {
			k_drawRect(cp->windowId, cp->bvalArea.x1 - 1, cp->bvalArea.y1 - 1, cp->bvalArea.x2 + 1, cp->bvalArea.y2 + 1, COLORPICKER_COLOR_VALAREABORDER, false);
			k_drawRect(cp->windowId, cp->bvalArea.x1 - 2, cp->bvalArea.y1 - 2, cp->bvalArea.x2 + 2, cp->bvalArea.y2 + 2, WINDOW_COLOR_BACKGROUND, false);
			k_updateScreenByWindowArea(cp->windowId, &cp->bupdArea);
		}

		break;
	}
}
