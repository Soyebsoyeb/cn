#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define MAX_SIZE 1024

void err(const char * exp) {
	perror(exp);
	exit(1);
}

int main() {
	char serv_ip[INET_ADDRSTRLEN];
	unsigned short port;

	printf("Enter IP address of server: ");
	scanf("%s", serv_ip);

	printf("Enter port number of server: ");
	scanf("%hu", &port);
	
	getchar();

	struct sockaddr_in serv_addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = inet_addr(serv_ip),
		.sin_port = htons(port)
	}, cli_addr;

	bzero(&cli_addr, sizeof(cli_addr));

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockfd<0) err("Failed to create socket!\n");

	if (fork()==0) {
		//child: send
		while (1) {
			char buf[MAX_SIZE];
			bzero(buf, MAX_SIZE);
			fgets(buf, MAX_SIZE, stdin);
			sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
		}
	}
	else {
		//parent: receive
		while (1) {
			char msg[MAX_SIZE];
			bzero(msg, MAX_SIZE);
			recvfrom(sockfd, msg, MAX_SIZE, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
			printf("Server: %s\n", msg);
		}
	}

	printf("Connection closed...\n");
	close(sockfd);
	return 0;
}
