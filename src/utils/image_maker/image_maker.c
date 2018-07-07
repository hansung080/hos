#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#define MY_O_BINARY     0x10000 // O_BINARY causes symbol error on Eclipse, so re-define it by myself.
#define BYTES_OF_SECTOR 512

int copyFile(int iSrcFd, int iTargetFd);
int adjustInSectorSize(int iFd, int iSrcSize);
void writeKernelInfo(int iTargetFd, int iTotalSectorCount, int iKernel32SectorCount);

int main(int argc, const char** argv) {
	int iSrcFd;
	int iTargetFd;
	int iBootLoaderSectorCount;
	int iKernel32SectorCount;
	int iKernel64SectorCount;
	int iSrcSize;

	// check parameter count.
	if (argc != 5) {
		fprintf(stderr, "[ERROR] Usage: ImageMaker.exe <TargetFile> <SourceFile1> <SourceFile2> <SourceFile3>\n");
		exit(-1);
	}

	const char* pcTargetFile  = argv[1];
	const char* pcSrcFile1 = argv[2];
	const char* pcSrcFile2 = argv[3];
	const char* pcSrcFile3 = argv[4];

	// open target file.
	if ((iTargetFd = open(pcTargetFile, O_RDWR | O_CREAT | O_TRUNC | MY_O_BINARY | S_IREAD | S_IWRITE)) == -1) {
		fprintf(stderr, "[ERROR] Disk.img open fail.\n");
		exit(-1);
	}

	// open source file 1, copy it, and adjust size.
	printf("[INFO] Copy %s to %s\n", pcSrcFile1, pcTargetFile);
	if ((iSrcFd = open(pcSrcFile1, O_RDONLY | MY_O_BINARY)) == -1) {
		fprintf(stderr, "[ERROR] %s open fail.\n", pcSrcFile1);
		exit(-1);
	}

	iSrcSize = copyFile(iSrcFd, iTargetFd);
	close(iSrcFd);

	iBootLoaderSectorCount = adjustInSectorSize(iTargetFd, iSrcSize);
	printf("[INFO] %s: file_size=[%d], sector_count=[%d]\n", pcSrcFile1, iSrcSize, iBootLoaderSectorCount);

	// open source file 2, copy it, and adjust size.
	printf("[INFO] Copy %s to %s\n", pcSrcFile2, pcTargetFile);
	if ((iSrcFd = open(pcSrcFile2, O_RDONLY | MY_O_BINARY)) == -1) {
		fprintf(stderr, "[ERROR] %s open fail.\n", pcSrcFile2);
		exit(-1);
	}

	iSrcSize = copyFile(iSrcFd, iTargetFd);
	close(iSrcFd);

	iKernel32SectorCount = adjustInSectorSize(iTargetFd, iSrcSize);
	printf("[INFO] %s: file_size=[%d], sector_count=[%d]\n", pcSrcFile2, iSrcSize, iKernel32SectorCount);

	// open source file 3, copy it, and adjust size.
	printf("[INFO] Copy %s to %s\n", pcSrcFile3, pcTargetFile);
	if ((iSrcFd = open(pcSrcFile3, O_RDONLY | MY_O_BINARY)) == -1) {
		fprintf(stderr, "[ERROR] %s open fail.\n", pcSrcFile3);
		exit(-1);
	}

	iSrcSize = copyFile(iSrcFd, iTargetFd);
	close(iSrcFd);

	iKernel64SectorCount = adjustInSectorSize(iTargetFd, iSrcSize);
	printf("[INFO] %s: file_size=[%d], sector_count=[%d]\n", pcSrcFile3, iSrcSize, iKernel64SectorCount);

	// write sector count to target file.
	printf("[INFO] Start to write kernel information.\n");
	writeKernelInfo(iTargetFd, iKernel32SectorCount + iKernel64SectorCount, iKernel32SectorCount);
	printf("[INFO] %s create complete.\n", pcTargetFile);

	close(iTargetFd);

	return 0;
}

int copyFile(int iSrcFd, int iTargetFd) {
	int iSrcSize = 0;
	int iReadSize;
	int iWriteSize;
	char vcBuffer[BYTES_OF_SECTOR];

	while (1) {
		iReadSize = read(iSrcFd, vcBuffer, sizeof(vcBuffer));
		iWriteSize = write(iTargetFd, vcBuffer, iReadSize);

		if (iReadSize != iWriteSize) {
			fprintf(stderr, "[ERROR] Read size is not equal to write size.\n");
			exit(-1);
		}

		iSrcSize += iReadSize;

		if (iReadSize != sizeof(vcBuffer)) {
			break;
		}
	}

	return iSrcSize;
}

int adjustInSectorSize(int iFd, int iSrcSize) {
	int i;
	int iAjustSize;
	char cZero;
	int iSectorCount;

	iAjustSize = iSrcSize % BYTES_OF_SECTOR;
	cZero = 0x00;

	if (iAjustSize != 0) {
		iAjustSize = BYTES_OF_SECTOR - iAjustSize;
		printf("[INFO] Adjust Size(YES): file size=[%lu], fill size=[%u]\n", iSrcSize, iAjustSize);

		for (i = 0; i < iAjustSize; i++) {
			write(iFd, &cZero, 1);
		}

	} else {
		printf("[INFO] Adjust Size(NO): File size(%lu) is already aligned with a sector size.\n", iSrcSize);
	}

	iSectorCount = (iSrcSize + iAjustSize) / BYTES_OF_SECTOR;

	return iSectorCount;
}

void writeKernelInfo(int iTargetFd, int iTotalSectorCount, int iKernel32SectorCount) {
	unsigned short usData;
	long lPos;

	// move file pointer to the physical address (0x7C05) of boot-loader
	lPos = lseek(iTargetFd, 5, SEEK_SET);
	if (lPos == -1) {
		fprintf(stderr, "[ERROR] lseek fail. file_pointer=[%d], errno=[%d], lseek_option=[%d]\n", lPos, errno, SEEK_SET);
		exit(-1);
	}

	usData = (unsigned short)iTotalSectorCount;
	write(iTargetFd, &usData, 2);
	usData = (unsigned short)iKernel32SectorCount;
	write(iTargetFd, &usData, 2);

	printf("[INFO] TOTAL_SECTOR_COUNT(except BootLoader)=[%d]\n", iTotalSectorCount);
	printf("[INFO] KERNEL32_SECTOR_COUNT=[%d]\n", iKernel32SectorCount);
}
