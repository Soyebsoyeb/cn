#include "protocol.h"
#include "crc.h"
#define MAX_SEQ_NUMBER 7
#define WINDOW_SIZE 4

void MakeFrame(Frame *f, char *data, int ack_seq) {
    f->ack = 1;
    f->seq = ack_seq;
    strncpy(f->data, data, strlen(data));
    f->crc = crc_16((unsigned char *)f->data, strlen(f->data));
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
            continue;
        }

        if (f.seq == expected) {
            printf("In order Frame %d received: %s\n", f.seq, f.data);
            char reply[64];
            bzero(reply, sizeof(reply));
            printf("Enter piggyback message: ");
            fgets(reply, sizeof(reply), stdin);
            Frame ack;
            bzero(&ack, sizeof(ack));
            MakeFrame(&ack, reply, f.seq);
            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&cli, len);
            expected = (expected + 1) % (MAX_SEQ_NUMBER + 1);

            while (received[expected]) {
                printf("Delivering buffered frame %d: %s\n", expected, buffer[expected].data);
                received[expected] = 0;
                Frame ack2;
                MakeFrame(&ack2, reply, expected);
                sendto(sockfd, &ack2, sizeof(ack2), 0, (struct sockaddr *)&cli, len);
                expected = (expected + 1) % (MAX_SEQ_NUMBER + 1);
            }

        } else if (in_window(expected, f.seq)) {
            if (!received[f.seq]) {
                printf("Out of order Frame %d buffered: %s\n", f.seq, f.data);
                buffer[f.seq] = f;
                received[f.seq] = 1;
                Frame ack;
                MakeFrame(&ack, "buffered", f.seq);
                sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&cli, len);
            }
        } else {
            printf("Out of window range Frame %d ignored.\n", f.seq);
        }
    }

    close(sockfd);
    return 0;
}
