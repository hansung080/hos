#ifndef __TEXTVIEWER_H__
#define __TEXTVIEWER_H__

#include <hanslib.h>

#define MAXLINECOUNT 262144 // 256 * 1024 lines
#define MARGIN       5      // 5 pixels
#define TABSIZE      4      // 4 spaces

#define COLOR_FILEINFO RGB(55, 215, 47)

typedef struct __TextInfo {
	byte* fileBuffer;  // file buffer
	dword fileSize;    // file size
	int columns;       // column (character) count in a line
	int rows;          // row (line) count in a page
	dword* fileOffset; // file offset of lines
	int maxLines;      // max line count in a file
	int currentLine;   // current line index in a file: Current line means first line in a current page.
	char fileName[FS_MAXFILENAMELENGTH]; // file name
} TextInfo;

bool readFile(const char* fileName, TextInfo* info);
void calcFileOffset(int width, int height, TextInfo* info);
bool drawTextBuffer(qword windowId, const TextInfo* info);

#endif // __TEXTVIEWER_H__