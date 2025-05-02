#include "protocol.h"
#include "crc.h"
#include <stdio.h>
#define MAX_SEQ_NUMBER 7

void MakeFrame(Frame *f, char *buf, int seq, int ack) {
    f->seq = seq;
    f->ack = ack;
    f->nack = 0;
    strncpy(f->data, buf, strlen(buf));
    f->crc = crc_16((unsigned char *)f->data, strlen(f->data));
}

void SendNACK(int sockfd, struct sockaddr_in *cli_addr, socklen_t cli_len, int seq) {
    Frame nack = {.seq = seq, .ack = 0, .nack = 1};
    sendto(sockfd, &nack, sizeof(nack), 0, (struct sockaddr *)cli_addr, cli_len);
}

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) exit(1);

    char rec_ip[INET_ADDRSTRLEN];
    unsigned short port;
    GetAddress(rec_ip);
    GetPort(&port);

    struct sockaddr_in rec_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr(rec_ip)
    }, cli_addr;

    bzero(&cli_addr, sizeof(cli_addr));

    if (bind(sockfd, (struct sockaddr *)&rec_addr, sizeof(rec_addr)) < 0) exit(1);

    socklen_t cli_len = sizeof(cli_addr);
    int expected_seq = 0;

    getchar();

    while (1) {
        Frame f;
        int n = recvfrom(sockfd, &f, sizeof(f), 0, (struct sockaddr *)&cli_addr, &cli_len);
        if (n <= 0) continue;

        unsigned short crc_c = crc_16((unsigned char *)f.data, strlen(f.data));

        if (crc_c == f.crc && f.seq == expected_seq) {
            printf("Received frame seq %d: %s\n", f.seq, f.data);
            char msg[128];
            bzero(msg, 128);
            printf("Enter message: ");
            fgets(msg, 128, stdin);
            Frame response;
            MakeFrame(&response, msg, expected_seq, 1);
            sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *)&cli_addr, cli_len);
            expected_seq = (expected_seq + 1) % (MAX_SEQ_NUMBER + 1);
        } else {
            SendNACK(sockfd, &cli_addr, cli_len, expected_seq);
        }
    }

    close(sockfd);
    return 0;
}
