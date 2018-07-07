#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define byte  unsigned char
#define dword unsigned int

#define MIN(x, y)          (((x) < (y)) ? (x) : (y))
#define SERIAL_FIFOMAXSIZE 16 // FIFO max size (16 bytes)

int main(int argc, const char** argv) {
	char vcFileName[256];
	char vcDataBuffer[SERIAL_FIFOMAXSIZE];
	struct sockaddr_in stSocketAddr;
	int iSocket;
	byte bAck;
	dword dwDataLen;
	dword dwSentSize;
	dword dwTempSize;
	FILE* fp;

	//----------------------------------------------------------------------------------------------------
	// open file
	//----------------------------------------------------------------------------------------------------

	// check file name.
	if (argc < 2) {
		fprintf(stderr, "Input file name:");
		gets(vcFileName);

	} else {
		strcpy(vcFileName, argv[1]);
	}

	// open file
	fp = fopen(vcFileName, "rb");
	if (fp == NULL) {
		fprintf(stderr, "'%s' File Open Fail~!!\n", vcFileName);
		return 0;
	}

	// measure file length, and move to the start of file.
	fseek(fp, 0, SEEK_END);
	dwDataLen = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fprintf(stderr, "File Name = [%s], Data Length = [%d] byte\n", vcFileName, dwDataLen);

	//----------------------------------------------------------------------------------------------------
	// connect to network
	//----------------------------------------------------------------------------------------------------

	// set server into.
	stSocketAddr.sin_family = AF_INET;
	stSocketAddr.sin_port = htons(4444);
	stSocketAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// create socket.
	iSocket = socket(AF_INET, SOCK_STREAM, 0);

	// connect to network.
	if (connect(iSocket, (struct sockaddr*)&stSocketAddr, sizeof(stSocketAddr)) == -1) {
		fprintf(stderr, "Socket Connect Fail~!! Server IP = [127.0.0.1], Server Port = [4444]\n");
		return 0;

	} else {
		fprintf(stderr, "Socket Connect Success~!! Server IP = [127.0.0.1], Server Port = [4444]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// read file, and send data
	//----------------------------------------------------------------------------------------------------

	// send data length.
	if (send(iSocket, &dwDataLen, 4, 0) != 4) {
		fprintf(stderr, "Data Length Send Fail~!! Data Length = [%d] byte\n", dwDataLen);
		return 0;

	} else {
		fprintf(stderr, "Data Length Send Success~!! Data Length = [%d] byte\n", dwDataLen);
	}

	// wait until ACK will be received.
	if (recv(iSocket, &bAck, 1, 0) != 1) {
		fprintf(stderr, "ACK Receive Fail~!!\n");
		return 0;
	}

	// send data.
	fprintf(stderr, "Data Send Start...");
	dwSentSize = 0;
	while (dwSentSize < dwDataLen) {
		// sending byte count = MIN(remained byte count, FIFO max size)
		dwTempSize = MIN(dwDataLen - dwSentSize, SERIAL_FIFOMAXSIZE);
		dwSentSize += dwTempSize;

		// read file.
		if (fread(vcDataBuffer, 1, dwTempSize, fp) != dwTempSize) {
			fprintf(stderr, "'%s' File Read Fail~!!\n", vcFileName);
			return 0;
		}

		// send data which is read from file.
		if (send(iSocket, vcDataBuffer, dwTempSize, 0) != dwTempSize) {
			fprintf(stderr, "Socket Send Fail~!!\n");
			return 0;
		}

		// wait until ACK will be received.
		if (recv(iSocket, &bAck, 1, 0) != 1) {
			fprintf(stderr, "ACK Receive Fail~!!\n");
			return 0;
		}

		// print progress.
		fprintf(stderr, "#");
	}

	// close file and socket.
	fclose(fp);
	close(iSocket);

	// print send complete message and wait for Enter key.
	fprintf(stderr, "\nSend Complete~!! Sent Size = [%d] byte\n", dwSentSize);
	fprintf(stderr, "Press Enter key to exit...\n");
	getchar();
	return 0;
}
