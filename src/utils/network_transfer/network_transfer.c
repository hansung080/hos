#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#ifdef __APPLE__
#include <sys/uio.h>
#else
#include <io.h>
#endif
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

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 7984

int main(int argc, const char** argv) {
	char fileName[256];
	char dataBuffer[SERIAL_FIFOMAXSIZE];
	struct sockaddr_in sockAddr;
	int sockfd;
	byte ack;
	dword dataLen;
	dword sentSize;
	dword tempSize;
	FILE* file;
	
	//----------------------------------------------------------------------------------------------------
	// open file
	//----------------------------------------------------------------------------------------------------
	
	// check file name.
	if (argc < 2) {
		printf("input file name: ");
		gets(fileName);
		
	} else {
		strcpy(fileName, argv[1]);
	}
	
	// open file
	file = fopen(fileName, "rb");
	if (file == NULL) {
		fprintf(stderr, "%s opening failure\n", fileName);
		exit(8);
	}
	
	// measure file length, and move to the start of file.
	fseek(file, 0, SEEK_END);
	dataLen = ftell(file);
	fseek(file, 0, SEEK_SET);
	printf("file name: %s, data length: %d bytes\n", fileName, dataLen);
	
	//----------------------------------------------------------------------------------------------------
	// connect to network
	//----------------------------------------------------------------------------------------------------
	
	// set server into.
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(SERVER_PORT);
	sockAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	
	// create socket.
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	// connect to network.
	if (connect(sockfd, (struct sockaddr*)&sockAddr, sizeof(sockAddr)) == -1) {
		fprintf(stderr, "socket connection failure: address: %s, port: %d\n", SERVER_IP, SERVER_PORT);
		exit(8);
		
	} else {
		printf("socket connection success: address: %s, port: %d\n", SERVER_IP, SERVER_PORT);
	}
	
	//----------------------------------------------------------------------------------------------------
	// read file, and send data
	//----------------------------------------------------------------------------------------------------
	
	// send data length.
	if (send(sockfd, &dataLen, 4, 0) != 4) {
		fprintf(stderr, "data length sending failure: data length: %d bytes\n", dataLen);
		exit(8);
		
	} else {
		printf("data length sending success: data length: %d bytes\n", dataLen);
	}
	
	// wait until ACK will be received.
	if (recv(sockfd, &ack, 1, 0) != 1) {
		fprintf(stderr, "ACK receiving failure\n");
		exit(8);
	}
	
	// send data.
	printf("send data...");
	sentSize = 0;
	while (sentSize < dataLen) {
		// sending byte count = MIN(remained byte count, FIFO max size)
		tempSize = MIN(dataLen - sentSize, SERIAL_FIFOMAXSIZE);
		sentSize += tempSize;
		
		// read file.
		if (fread(dataBuffer, 1, tempSize, file) != tempSize) {
			fprintf(stderr, "%s reading failure\n", fileName);
			exit(8);
		}
		
		// send data which is read from file.
		if (send(sockfd, dataBuffer, tempSize, 0) != tempSize) {
			fprintf(stderr, "data sending failure\n");
			exit(8);
		}
		
		// wait until ACK will be received.
		if (recv(sockfd, &ack, 1, 0) != 1) {
			fprintf(stderr, "ACK receiving failure\n");
			exit(8);
		}
	}
	
	printf("success\n");
	
	// close file and socket.
	fclose(file);
	close(sockfd);
	
	// print send complete message and wait for Enter key.
	printf("sending complete: sent size: %d bytes\n", sentSize);
	printf("Press any key to exit.");
	getchar();
	
	return 0;
}
