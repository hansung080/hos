#include "text_viewer.h"

int main(const char* args) {
	ArgList argList;
	char fileName[ARG_MAXLENGTH] = {'\0', };
	TextInfo info;
	Rect screenArea;
	int width, height;
	int x, y;
	qword windowId;
	Event event;
	WindowEvent* windowEvent;
	KeyEvent* keyEvent;
	int moveLines;
	dword fileOffset;

	/* check graphic mode */
	if (isGraphicMode() == false) {
		printf("[text viewer error] not graphic mode\n");
		return -1;
	}

	/* get file name from argument string */
	initArgs(&argList, args);

	if (getNextArg(&argList, fileName) <= 0) {
		printf("Usage) run text-viewer.elf <file>\n");
		printf("  - file: file name\n");
		printf("  - example: run text-viewer.elf a.txt\n");
		return -1;
	}

	/* read file */
	if (readFile(fileName, &info) == false) {
		printf("[text viewer error] file reading failure\n");
		return -1;
	}

	/* create window */
	getScreenArea(&screenArea);
	width = 500;
	height = 500;
	x = (getRectWidth(&screenArea) - width) / 2;
	y = (getRectHeight(&screenArea) - height) / 2;

	windowId = createWindow(x, y, width, height, WINDOW_FLAGS_DEFAULT | WINDOW_FLAGS_RESIZABLE | WINDOW_FLAGS_BLOCKING, TITLE, WINDOW_COLOR_BACKGROUND, null, null, WINDOW_INVALIDID);
	if (windowId == WINDOW_INVALIDID) {
		printf("[text viewer error] window creation failure\n");
		return -1;
	}

	/* calculate file offset and draw text buffer */
	calcFileOffset(width, height, &info);
	info.currentLine = 0;
	drawTextBuffer(windowId, &info);

	/* event processing loop */
	while (true) {
		if (recvEventFromWindow(&event, windowId) == false) {
			sleep(0);
			continue;
		}

		switch (event.type) {
		case EVENT_WINDOW_RESIZE:
			windowEvent = &event.windowEvent;

			width = getRectWidth(&windowEvent->area);
			height = getRectHeight(&windowEvent->area);

			fileOffset = info.fileOffset[info.currentLine];
			calcFileOffset(width, height, &info);
			info.currentLine = fileOffset / info.columns;
			drawTextBuffer(windowId, &info);
			break;

		case EVENT_WINDOW_CLOSE:
			deleteWindow(windowId);
			// [NOTE] Not closing window, but killing task causes memory leak.
			free(info.fileBuffer);
			free(info.fileOffset);
			return 0;

		case EVENT_KEY_DOWN:
			keyEvent = &event.keyEvent;

			switch (keyEvent->asciiCode) {
			case KEY_UP:
				moveLines = -1;
				break;

			case KEY_DOWN:
				moveLines = 1;
				break;

			case KEY_PAGEUP:
				moveLines = -info.rows;
				break;

			case KEY_PAGEDOWN:
				moveLines = info.rows;
				break;

			default:
				moveLines = 0;
				break;
			}

			if (info.currentLine + moveLines < 0) {
				moveLines = -info.currentLine;

			} else if (info.currentLine + moveLines >= info.maxLines) {
				moveLines = info.maxLines - info.currentLine - 1;
			}

			if (moveLines == 0) {
				break;
			}

			info.currentLine += moveLines;
			drawTextBuffer(windowId, &info);

			break;

		default:
			break;
		}
	}

	return 0;
}