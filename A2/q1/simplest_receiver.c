#include "protocol.h"
#include "crc.h"

void WaitForEvent() {
	printf("Receiver waiting for frame...\n");
}

void ReceiveFrame(int fd, Frame *f) {
	int r = read(fd, f, sizeof(Frame));
	printf("Received a frame...\n");
}

void ExtractData(Frame *f, char *c) {
	strncpy(c, f->data, strlen(f->data));
	if (crc_16((unsigned char *) f->data, strlen(f->data))!=f->crc) perror("CRC values don't match. Error!\n"), exit(1);
}

void DeliverData(char *c) {
	printf("Received data: %s", c);
}

int main() {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd<0) perror("Socket creation failed!\n"), exit(1);

	char rec_ip[INET_ADDRSTRLEN];
	unsigned short port;

	printf("Enter IP address of receiver: ");
	scanf("%s", rec_ip);

	printf("Enter port number of receiver: ");
	scanf("%hu", &port);

	struct sockaddr_in rec_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = inet_addr(rec_ip)
	}, cli_addr;

	bzero(&cli_addr, sizeof(cli_addr));
	
	int res = bind(sockfd, (struct sockaddr *)&rec_addr, sizeof(rec_addr));
	if (res<0) perror("Failed to bind to port!\n"), exit(1);

	listen(sockfd, 1);
	
	socklen_t clilen = sizeof(cli_addr);

	int connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

	while (1) {
		char buf[MAX_SIZE];
		bzero(buf, MAX_SIZE);
		Frame *f = (Frame *) malloc(sizeof(Frame));

		WaitForEvent();

		ReceiveFrame(connfd, f);

		ExtractData(f, buf);

		DeliverData(buf);
	}

	printf("Closing connection...\n");

	close(sockfd);
	close(connfd);

	return 0;
}

