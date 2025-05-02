#include "protocol.h"
#include "crc.h"
#define MAX_SEQ_NUMBER 7
#define WINDOW_SIZE 4

void SendACK(int sockfd, struct sockaddr_in *cli, socklen_t len, int seq) {
    Frame ack = {.seq = seq, .ack = 1, .nack = 0};
    sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)cli, len);
    printf("ACK Sent for seq %d\n", seq);
}

void SendNACK(int sockfd, struct sockaddr_in *cli, socklen_t len, int seq) {
    Frame nack = {.seq = seq, .ack = 0, .nack = 1};
    sendto(sockfd, &nack, sizeof(nack), 0, (struct sockaddr *)cli, len);
    printf("NACK Sent for missing seq %d\n", seq);
}

int in_window(int expected, int seq) {
    if (expected <= (expected + WINDOW_SIZE - 1) % (MAX_SEQ_NUMBER + 1)) {
        return seq >= expected && seq <= (expected + WINDOW_SIZE - 1);
    } else {
        return seq >= expected || seq <= (expected + WINDOW_SIZE - 1) % (MAX_SEQ_NUMBER + 1);
    }
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

    while (1) {
        Frame f;
        int r = recvfrom(sockfd, &f, sizeof(f), 0, (struct sockaddr *)&cli, &len);
        if (r <= 0) continue;

        unsigned short crc_val = crc_16((unsigned char *)f.data, strlen(f.data));

        if (crc_val != f.crc) {
            printf("Corrupted frame discarded. Seq: %d\n", f.seq);
            SendNACK(sockfd, &cli, len, expected);
            continue;
        }

        if (f.seq == expected) {
            printf("In order Frame %d received: %s\n", f.seq, f.data);
            SendACK(sockfd, &cli, len, f.seq);
            expected = (expected + 1) % (MAX_SEQ_NUMBER + 1);

            while (received[expected]) {
                printf("Buffered and in order Frame %d delivered from buffer: %s\n", buffer[expected].seq, buffer[expected].data);
                received[expected] = 0;
                SendACK(sockfd, &cli, len, expected);
                expected = (expected + 1) % (MAX_SEQ_NUMBER + 1);
            }

        } else if (in_window(expected, f.seq)) {
            if (!received[f.seq]) {
                buffer[f.seq] = f;
                received[f.seq] = 1;
                printf("Out of order Frame %d buffered: %s\n", f.seq, f.data);
                SendACK(sockfd, &cli, len, f.seq);
            } else {
                printf("Duplicate Frame %d ignored (already buffered).\n", f.seq);
            }

            if (!received[expected]) {
                SendNACK(sockfd, &cli, len, expected);
            }

        } else {
            printf("Out of window range Frame %d ignored.\n", f.seq);
        }
    }

    close(sockfd);
    return 0;
}
