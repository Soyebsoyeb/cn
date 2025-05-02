#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_SIZE 1024

int main() {
    int port;
	char serv_ip[INET_ADDRSTRLEN];

    printf("Enter server IP: ");
    scanf("%s", serv_ip);
    printf("Enter port: ");
    scanf("%d", &port);
    getchar(); // consume newline

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) perror("socket"), exit(1);

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr(serv_ip)
    };

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        perror("bind"), exit(1);

    listen(sockfd, 1);
    printf("Waiting for connection...\n");

    int connfd = accept(sockfd, NULL, NULL);
    if (connfd < 0) perror("accept"), exit(1);

    printf("Client connected!\n");

    if (fork() == 0) {
        // child: sending
        char msg[MAX_SIZE];
        while (1) {
            bzero(msg, MAX_SIZE);
			// printf("Enter message: ");
            fgets(msg, MAX_SIZE, stdin);
            write(connfd, msg, strlen(msg));
            if (strncmp(msg, "exit", 4) == 0) break;
        }
    } else {
        // parent: receiving
        char buf[MAX_SIZE];
        while (1) {
            bzero(buf, MAX_SIZE);
            int n = read(connfd, buf, MAX_SIZE);
            if (n <= 0) break;
            printf("\nClient: %s\n", buf);
            if (strncmp(buf, "exit", 4) == 0) break;
        }
    }

    close(connfd);
    close(sockfd);
    printf("Connection closed.\n");
    return 0;
}
