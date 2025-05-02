#include "protocol.h"
#include "crc.h"
#define MAX_SEQ_NUMBER 7
#define WINDOW_SIZE 4

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
            printf("In order Frame %d received: %s\n", f.seq, f.data);
            Frame ack = {.seq = f.seq, .ack = 1};
            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&cli, len);
            expected = (expected + 1) % (MAX_SEQ_NUMBER + 1);

            while (received[expected]) {
                printf("Buffered and in order Frame %d delivered from buffer: %s\n", buffer[expected].seq, buffer[expected].data);
                received[expected] = 0;
                Frame ack2 = {.seq = expected, .ack = 1};
                sendto(sockfd, &ack2, sizeof(ack2), 0, (struct sockaddr *)&cli, len);
                expected = (expected + 1) % (MAX_SEQ_NUMBER + 1);
            }

        } else if ((f.seq > expected && f.seq < expected + WINDOW_SIZE) || (expected > MAX_SEQ_NUMBER - WINDOW_SIZE && f.seq < (expected + WINDOW_SIZE) % (MAX_SEQ_NUMBER + 1))) {
            if (!received[f.seq]) {
                printf("Out of order Frame %d received and buffered: %s\n", f.seq, f.data);
                buffer[f.seq] = f;
                received[f.seq] = 1;
                Frame ack = {.seq = f.seq, .ack = 1};
                sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&cli, len);
            } else {
                printf("Duplicate Frame %d ignored (already buffered).\n", f.seq);
            }
        } else {
            printf("Out of window range Frame %d ignored.\n", f.seq);
        }
    }

    close(sockfd);
    return 0;
}
