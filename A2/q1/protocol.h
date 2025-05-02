#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAX_SIZE 1024

typedef struct {
	int seq;
	char data[MAX_SIZE];
	unsigned short crc;
} Frame;

void WaitForEvent();
void GetData(char *);
void MakeFrame(Frame *, char *, int);
void SendFrame(int, Frame *);
void ReceiveFrame(int, Frame *);
void ExtractData(Frame *, char *);
void DeliverData(char *);

#endif
