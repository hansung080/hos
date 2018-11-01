#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"

// argument
#define ARG_FILE argv[1]
#define ARG_PATH argv[2]

void getChangedFileName(const char* oldName, const char* prefix, char* newName);
int findChar(const char* str, char ch);

int main(int argc, const char** argv) {
	FILE* orgFile;
	FILE* tempFile;
	char tempFileName[256];
	char line[1024];
	char modifiedLine[1024];

	if (argc != 3) {
		fprintf(stderr, "[error] Usage) dep-modifier <file> <path>\n");
		exit(8);
	}

	orgFile = fopen(ARG_FILE, "r");
	if (orgFile == NULL) {
		fprintf(stderr, "[error] %s opening failure\n", ARG_FILE);
		exit(8);
	}

	getChangedFileName(ARG_FILE, "temp_", tempFileName);

	tempFile = fopen(tempFileName, "w");
	if (tempFile == NULL) {
		fprintf(stderr, "[error] %s opening failure\n", tempFileName);
		exit(8);	
	}

	while (fgets(line, sizeof(line), orgFile) != NULL) {
		if (findChar(line, ':') >= 0) {
			strcpy(modifiedLine, ARG_PATH);
			strcat(modifiedLine, line);

			if (fputs(modifiedLine, tempFile) == EOF) {
				break;
			}

		} else {
			if (fputs(line, tempFile) == EOF) {
				break;
			}
		}
	}

	fclose(orgFile);
	fclose(tempFile);

	if (remove(ARG_FILE) == -1) {
		fprintf(stderr, "[error] %s removing failure\n", ARG_FILE);
		exit(8);
	}

	if (rename(tempFileName, ARG_FILE) == -1) {
		fprintf(stderr, "[error] %s to %s renaming failure\n", tempFileName, ARG_FILE);
		exit(8);
	}

	printf("[info] dep-modifier complete: %s modified with %s\n", ARG_FILE, ARG_PATH);

	return 0;
}

void getChangedFileName(const char* oldName, const char* prefix, char* newName) {
	int i, j, k;
	int oldNameLen;
	int prefixLen;
	bool done = false;

	oldNameLen = strlen(oldName);
	prefixLen = strlen(prefix);
	k = oldNameLen + prefixLen - 1;

	for (i = oldNameLen - 1; i >= 0; i--) {
		if ((oldName[i] == '/') && (done == false))  {
			done = true;
			for (j = prefixLen - 1; j >= 0; j--) {
				newName[k--] = prefix[j];
			}
		}

		newName[k--] = oldName[i];
	}

	if (done == false) {
		for (j = prefixLen - 1; j >= 0; j--) {
			newName[k--] = prefix[j];
		}
	}

	newName[oldNameLen + prefixLen] = '\0';
}

int findChar(const char* str, char ch) {
	int i;

	for (i = 0; str[i] != '\0'; i++) {
		if (str[i] == ch) {
			return i;
		}
	}

	return -1;
}