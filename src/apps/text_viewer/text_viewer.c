#include "text_viewer.h"

bool readFile(const char* fileName, TextInfo* info) {
	dword fileSize;
	Dir* dir;
	dirent* entry;
	File* file;

	/* get file size */
	fileSize = 0;
	dir = opendir("/");
	while (true) {
		entry = readdir(dir);
		if (entry == null) {
			break;
		}

		if (strcmp(entry->d_name, fileName) == 0) {
			fileSize = entry->d_size;
			break;
		}
	}

	closedir(dir);

	if (fileSize == 0) {
		printf("[text viewer error] %s does not exist or is zero-sized.\n", fileName);
		return false;
	}

	info->fileSize = fileSize;
	strcpy(info->fileName, fileName);

	/* allocate file offset and file buffer */
	info->fileOffset = (dword*)malloc(sizeof(dword) * MAXLINECOUNT);
	if (info->fileOffset == null) {
		printf("[text viewer error] file offset allocation failure\n");
		return false;
	}

	info->fileBuffer = (byte*)malloc(fileSize);
	if (info->fileBuffer == null) {
		printf("[text viewer error] file buffer allocation failure\n");
		free(info->fileOffset);
		return false;
	}

	/* read file */
	file = fopen(fileName, "r");
	if (file == null) {
		printf("[text viewer error] %s opening failure\n", fileName);	
		free(info->fileBuffer);
		free(info->fileOffset);
		return false;
	}

	if (fread(info->fileBuffer, 1, fileSize, file) != fileSize) {
		printf("[text viewer error] %s reading failure\n", fileName);	
		fclose(file);
		free(info->fileBuffer);
		free(info->fileOffset);
		return false;
	}

	fclose(file);

	return true;
}

void calcFileOffset(int width, int height, TextInfo* info) {
	int line;
	int column;
	dword i;

	info->columns = (width - (MARGIN * 2)) / FONT_DEFAULT_WIDTH;
	info->rows = (height - (WINDOW_TITLEBAR_HEIGHT * 2) - (MARGIN * 2)) / FONT_DEFAULT_HEIGHT;

	line = 0;
	column = 0;
	info->fileOffset[0] = 0;
	for (i = 0; i < info->fileSize; i++) {
		if (info->fileBuffer[i] == '\r') {
			continue;

		} else if (info->fileBuffer[i] == '\t') {
			column = column + TABSIZE;
			column -= column % TABSIZE;

		} else {
			column++;
		}

		if ((column >= info->columns) || (info->fileBuffer[i] == '\n')) {
			line++;
			column = 0;

			if (i + 1 < info->fileSize) {
				info->fileOffset[line] = i + 1;
			}

			if (line >= MAXLINECOUNT) {
				break;
			}
		}
	}

	info->maxLines = line;
}

bool drawTextBuffer(qword windowId, const TextInfo* info) {
	int xOffset, yOffset;
	Rect windowArea;
	int width;
	char buffer[100];
	int len;	
	int printRows, printColumns;
	dword i, j;
	dword fileOffset;
	int column;
	byte ch;

	xOffset = MARGIN;
	yOffset = WINDOW_TITLEBAR_HEIGHT;
	getWindowArea(windowId, &windowArea);

	/* draw file info */
	width = getRectWidth(&windowArea);
	drawRect(windowId, 2, yOffset, width - 3, WINDOW_TITLEBAR_HEIGHT * 2, COLOR_FILEINFO, true);
	sprintf(buffer, "file: %s, line: %d/%d\n", info->fileName, info->currentLine + 1, info->maxLines);
	len = strlen(buffer);
	drawText(windowId, (width - (FONT_DEFAULT_WIDTH * len)) / 2, WINDOW_TITLEBAR_HEIGHT + 2, RGB(255, 255, 255), COLOR_FILEINFO, buffer, len);

	/* draw file content area */
	yOffset = (WINDOW_TITLEBAR_HEIGHT * 2) + MARGIN;
	drawRect(windowId, xOffset, yOffset, xOffset + (FONT_DEFAULT_WIDTH * info->columns), yOffset + (FONT_DEFAULT_HEIGHT * info->rows), RGB(255, 255, 255), true);

	/* draw file content looping by a row */
	printRows = MIN(info->rows, info->maxLines - info->currentLine);
	for (i = 0; i < printRows; i++) {
		fileOffset = info->fileOffset[info->currentLine + i];

		/* draw file content looping by a column */
		printColumns = MIN(info->columns, info->fileSize - fileOffset);
		column = 0;
		for (j = 0; (j < printColumns) && (column < info->columns); j++) {
			ch = info->fileBuffer[fileOffset + j];

			if (ch == '\n') {
				break;

			} else if (ch == '\t') {
				column = column + TABSIZE;
				column -= column % TABSIZE;

			} else if (ch == '\r') {
			} else {
				drawText(windowId, xOffset + (FONT_DEFAULT_WIDTH * column), yOffset + (FONT_DEFAULT_HEIGHT * i), RGB(0, 0, 0), RGB(255, 255, 255), &ch, 1);
				column++;
			}
		}
	}

	showWindow(windowId, true);
}
