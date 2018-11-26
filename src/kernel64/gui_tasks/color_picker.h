#ifndef __GUITASKS_COLORPICKER_H__
#define __GUITASKS_COLORPICKER_H__

#include "../core/types.h"
#include "../core/2d_graphics.h"

// color
#define COLPKR_COLOR_VALAREABORDER RGB(167, 173, 186)

// size
#define COLPKR_WIDTH  450
#define COLPKR_HEIGHT 450

// focus
#define COLPKR_FOCUS_RGAREA   1
#define COLPKR_FOCUS_BAREA    2
#define COLPKR_FOCUS_RVALAREA 3
#define COLPKR_FOCUS_GVALAREA 4
#define COLPKR_FOCUS_BVALAREA 5
#define COLPKR_FOCUS_DEFAULT  COLPKR_FOCUS_RGAREA

#pragma pack(push, 1)

typedef struct k_ColorPicker {
	qword windowId;
	int red;
	int green;
	int blue;
	char value[4];
	int index;
	int rval;
	int gval;
	int bval;
	int focus;
	Rect selArea;
	Rect rgArea;
	Rect bArea;
	Rect rvalArea;
	Rect gvalArea;
	Rect bvalArea;
	Rect rupdArea;
	Rect gupdArea;
	Rect bupdArea;
} ColorPicker;

#pragma pack(pop)

void k_colorPickerTask(void);
static void k_initColorPicker(ColorPicker* cp);
static void k_drawRedGreenByBlue(const ColorPicker* cp, int blue);
static void k_drawBlue(const ColorPicker* cp);
static void k_updateSelectedColor(const ColorPicker* cp, Color color);
static void k_updateRGPicker(const ColorPicker* cp, int red, int green, int blue, bool new);
static void k_updateBPicker(const ColorPicker* cp, int blue, bool new);
static void k_updateRGValue(const ColorPicker* cp, int red, int green);
static void k_updateBValue(const ColorPicker* cp, int blue);
static void k_processRedGreenEvent(ColorPicker* cp, int red, int green);
static void k_processBlueEvent(ColorPicker* cp, int blue);
static void k_processFocus(const ColorPicker* cp, int focus, bool on);

#endif // __GUITASKS_COLORPICKER_H__