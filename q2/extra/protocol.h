[be2365@localhost q1]$ cd ..
[be2365@localhost A2]$ cd q2
[be2365@localhost q2]$ ls
extra  noiseless  noisy
[be2365@localhost q2]$ cd extra
[be2365@localhost extra]$ ls
stop_receiver  stop_receiver.c  stop_sender  stop_sender.c
[be2365@localhost extra]$ gcc stop_sender.c -o stop_sender
stop_sender.c:1:22: fatal error: protocol.h: No such file or directory
 #include "protocol.h"
                      ^
compilation terminated.
[be2365@localhost extra]$ ls
stop_receiver  stop_receiver.c  stop_sender  stop_sender.c
[be2365@localhost extra]$ 
[be2365@localhost extra]$ clear






























[be2365@localhost extra]$ cd ..
[be2365@localhost q2]$ ls
extra  noiseless  noisy
[be2365@localhost q2]$ cd noiseless
[be2365@localhost noiseless]$ ls
a.out       stop_noiseless_receiver    stop_noiseless_sender.c
crc.h       stop_noiseless_receiver.c
protocol.h  stop_noiseless_sender
[be2365@localhost noiseless]$ vi protocol.h





































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
                                                                               
                                                                               
                                                                               
                                                                               
                                                                    
