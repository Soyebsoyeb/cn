#include "protocol.h"
#include "crc.h"
#define MAX_SEQ_NUMBER 7

void MakeACK(Frame *f, int seq) {
    f->seq = seq;
    f->ack = 1;
    bzero(f->data, sizeof(f->data));
    f->crc = crc_16((unsigned char *)f->data, 0);
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
    int expected = 0;

    while (1) {
        Frame f;
        int n = recvfrom(sockfd, &f, sizeof(f), 0, (struct sockaddr *)&cli_addr, &cli_len);
        if (n <= 0) continue;

        unsigned short crc_val = crc_16((unsigned char *)f.data, strlen(f.data));

        if (f.seq == expected && crc_val == f.crc) {
            printf("Received frame seq %d: %s\n", f.seq, f.data);
            Frame ack;
            MakeACK(&ack, expected);
            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&cli_addr, cli_len);
            expected = (expected + 1) % (MAX_SEQ_NUMBER + 1);
        } else {
            Frame ack;
            MakeACK(&ack, (expected - 1 + MAX_SEQ_NUMBER + 1) % (MAX_SEQ_NUMBER + 1));
            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&cli_addr, cli_len);
        }
    }

    close(sockfd);
    return 0;
}
