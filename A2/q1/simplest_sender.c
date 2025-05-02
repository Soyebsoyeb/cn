#include "protocol.h"
#include "crc.h"

void WaitForEvent() {
	printf("Sender waiting for data input...\n");
}

void GetData(char *buf) {
	printf("Enter message: ");
	fgets(buf, MAX_SIZE, stdin);
}

void MakeFrame(Frame *f, char *data, int seq) {
	memcpy(f->data, data, strlen(data));
	f->seq = seq;
	f->crc = crc_16((unsigned char *)data, strlen(data));
}

void SendFrame(int fd, Frame *f) {
	write(fd, f, sizeof(*f));
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
	};

	connect(sockfd, (struct sockaddr *)&rec_addr, sizeof(rec_addr));
	int seq = 0;
	while (1) {
		char buf[MAX_SIZE];
		bzero(buf, MAX_SIZE);
		Frame *f = (Frame *) malloc(sizeof(Frame));

		WaitForEvent();

		GetData(buf);

		MakeFrame(f, buf, seq++);

		SendFrame(sockfd, f);
	}

	printf("Closing connection...\n");

	close(sockfd);

	return 0;
}

