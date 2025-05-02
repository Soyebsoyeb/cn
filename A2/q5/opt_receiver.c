#include "protocol.h"
#include "crc.h"
#define MAX_SEQ_NUMBER 7
#define WINDOW_SIZE 4
#define BUFFER_LIMIT 4

void SendACK(int sockfd, struct sockaddr_in *cli, socklen_t len, int seq, int buffer_flag) {
    Frame ack = {.seq = seq, .ack = 1, .nack = 0, .buffer_flag = buffer_flag};
    bzero(ack.data, sizeof(ack.data));
    ack.crc = crc_16((unsigned char *)ack.data, 0);
    sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)cli, len);
    printf("Sent for seq %d | buffer_flag = %d\n", seq, buffer_flag);
}

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) exit(1);

    char ip[INET_ADDRSTRLEN];
    unsigned short port;
    GetAddress(ip);
    GetPort(&port);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr(ip)
    }, cli;

    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    socklen_t len = sizeof(cli);

    int expected = 0;
    Frame buffer[MAX_SEQ_NUMBER + 1];
    int received[MAX_SEQ_NUMBER + 1] = {0};
    int buffer_count = 0;

    while (1) {
        Frame f;
        int r = recvfrom(sockfd, &f, sizeof(f), 0, (struct sockaddr *)&cli, &len);
        if (r <= 0) continue;

        unsigned short crc_val = crc_16((unsigned char *)f.data, strlen(f.data));
        if (crc_val != f.crc) {
            printf("Corrupted frame discarded. Seq: %d\n", f.seq);
            continue;
        }

        if (f.seq == expected) {
            printf("In order Frame %d: %s\n", f.seq, f.data);
            expected = (expected + 1) % (MAX_SEQ_NUMBER + 1);
            SendACK(sockfd, &cli, len, f.seq, buffer_count < BUFFER_LIMIT);
        } else if (!received[f.seq]) {
            if (buffer_count < BUFFER_LIMIT) {
                printf("Out of order Frame %d buffered\n", f.seq);
                buffer[f.seq] = f;
                received[f.seq] = 1;
                buffer_count++;
                SendACK(sockfd, &cli, len, f.seq, 1);
            } else {
                printf("Buffer full: Dropped frame %d\n", f.seq);
                SendACK(sockfd, &cli, len, f.seq, 0);
            }
        } else {
            printf("Duplicate Frame %d ignored\n", f.seq);
        }
    }

    close(sockfd);
    return 0;
}
