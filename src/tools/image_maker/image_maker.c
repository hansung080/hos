#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef __APPLE__
#include <sys/uio.h>
#else
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

// argument
#define ARG_TARGET argv[1]
#define ARG_SRC1   argv[2]
#define ARG_SRC2   argv[3]
#define ARG_SRC3   argv[4]

// Unix-based OS such as linux, macOS has no difference between binary file and text file.
// Thus, O_BINARY option can be ignored on those OS.
#ifndef O_BINARY
#define O_BINARY 0
#endif

#define BYTES_OF_SECTOR 512

int copyFile(int srcfd, int targetfd);
int adjustBySectorSize(int fd, int srcSize);
void writeKernelInfo(int targetfd, int totalSectorCount, int kernel32SectorCount);

int main(int argc, const char** argv) {
	int targetfd;
	int srcfd;
	int bootLoaderSectorCount;
	int kernel32SectorCount;
	int kernel64SectorCount;
	int srcSize;
	
	// check parameter count.
	if (argc != 5) {
		fprintf(stderr, "[error] Usage) image-maker <target> <src1> <src2> <src3>\n");
		exit(8);
	}
		
	// open target file.
	if ((targetfd = open(ARG_TARGET, O_RDWR | O_CREAT | O_TRUNC | O_BINARY | S_IREAD | S_IWRITE)) == -1) {
		fprintf(stderr, "[error] %s opening failure\n", ARG_TARGET);
		exit(8);
	}
	
	// open source file 1, copy it, and adjust size.
	printf("[info] copy %s to %s\n", ARG_SRC1, ARG_TARGET);
	if ((srcfd = open(ARG_SRC1, O_RDONLY | O_BINARY)) == -1) {
		fprintf(stderr, "[error] %s opening failure\n", ARG_SRC1);
		exit(8);
	}
	
	srcSize = copyFile(srcfd, targetfd);
	close(srcfd);
	
	bootLoaderSectorCount = adjustBySectorSize(targetfd, srcSize);
	printf("[info] %s info: file size: %d, sector count: %d\n", ARG_SRC1, srcSize, bootLoaderSectorCount);
	
	// open source file 2, copy it, and adjust size.
	printf("[info] copy %s to %s\n", ARG_SRC2, ARG_TARGET);
	if ((srcfd = open(ARG_SRC2, O_RDONLY | O_BINARY)) == -1) {
		fprintf(stderr, "[error] %s opening failure\n", ARG_SRC2);
		exit(8);
	}
	
	srcSize = copyFile(srcfd, targetfd);
	close(srcfd);
	
	kernel32SectorCount = adjustBySectorSize(targetfd, srcSize);
	printf("[info] %s info: file size: %d, sector count: %d\n", ARG_SRC2, srcSize, kernel32SectorCount);
	
	// open source file 3, copy it, and adjust size.
	printf("[info] copy %s to %s\n", ARG_SRC3, ARG_TARGET);
	if ((srcfd = open(ARG_SRC3, O_RDONLY | O_BINARY)) == -1) {
		fprintf(stderr, "[error] %s opening failure\n", ARG_SRC3);
		exit(8);
	}
	
	srcSize = copyFile(srcfd, targetfd);
	close(srcfd);
	
	kernel64SectorCount = adjustBySectorSize(targetfd, srcSize);
	printf("[info] %s info: file size: %d, sector count: %d\n", ARG_SRC3, srcSize, kernel64SectorCount);
	
	// write sector counts to target file.
	printf("[info] write sector counts to %s\n", ARG_TARGET);
	writeKernelInfo(targetfd, kernel32SectorCount + kernel64SectorCount, kernel32SectorCount);
	printf("[info] image-maker complete\n");
	
	close(targetfd);
	
	return 0;
}

int copyFile(int srcfd, int targetfd) {
	int srcSize = 0;
	int readSize;
	int writeSize;
	char buffer[BYTES_OF_SECTOR];
	
	while (1) {
		readSize = read(srcfd, buffer, sizeof(buffer));
		writeSize = write(targetfd, buffer, readSize);
		
		if (readSize != writeSize) {
			fprintf(stderr, "[error] read size != write size\n");
			exit(8);
		}
		
		srcSize += readSize;
		
		if (readSize != sizeof(buffer)) {
			break;
		}
	}
	
	return srcSize;
}

int adjustBySectorSize(int fd, int srcSize) {
	int i;
	int ajustSize;
	char zero;
	int sectorCount;
	
	ajustSize = srcSize % BYTES_OF_SECTOR;
	zero = 0x00;
	
	if (ajustSize != 0) {
		ajustSize = BYTES_OF_SECTOR - ajustSize;
		printf("[info] adjust file size: file size: %d, fill size: %u\n", srcSize, ajustSize);
		
		for (i = 0; i < ajustSize; i++) {
			write(fd, &zero, 1);
		}
		
	} else {
		printf("[info] not adjust file size: file size (%d) has been already aligned with sector size.\n", srcSize);
	}
	
	sectorCount = (srcSize + ajustSize) / BYTES_OF_SECTOR;
	
	return sectorCount;
}

void writeKernelInfo(int targetfd, int totalSectorCount, int kernel32SectorCount) {
	unsigned short data;
	long pos;
	
	// move file pointer to the physical address (0x7C05) of boot-loader
	pos = lseek(targetfd, 5, SEEK_SET);
	if (pos == -1) {
		fprintf(stderr, "[error] file seeking failure: file pointer: %ld, errno: %d, seek option: %d\n", pos, errno, SEEK_SET);
		exit(8);
	}
	
	data = (unsigned short)totalSectorCount;
	write(targetfd, &data, 2);
	data = (unsigned short)kernel32SectorCount;
	write(targetfd, &data, 2);
	
	printf("[info] total sector count (except boot-loader): %d, kernel32 sector count: %d\n", totalSectorCount, kernel32SectorCount);
}
