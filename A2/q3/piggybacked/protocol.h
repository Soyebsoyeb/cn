#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include "crc.h"
#define MAX_SIZE 1024

typedef struct {
	int seq;
	char data[MAX_SIZE];
	unsigned short crc;
	int ack;
	int nack;
} Frame;

void GetAddress(char *s) {
	printf("Enter receiver's IP address: ");
	scanf("%s", s);
}

void GetPort(unsigned short *u) {
	printf("Enter receiver's port number: ");
	scanf("%hu", u);
}

void SocketTime(int sockfd, int sec) {
	struct timeval tv = {
		.tv_sec = sec,

		.tv_usec = 0

	};
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);
}

#endif
