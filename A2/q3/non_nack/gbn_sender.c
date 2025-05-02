#include "protocol.h"
#include "crc.h"
#include <time.h>

#define WINDOW_SIZE 4
#define CHUNK_SIZE 8
#define MAX_FRAMES 100
#define MAX_SEQ_NUMBER 7

int seq_counter = 0;

void MakeFrame(Frame *f, char *buf, int seq) {
    f->seq = seq;
    f->ack = 0;
    strncpy(f->data, buf, strlen(buf));
    f->crc = crc_16((unsigned char *)f->data, strlen(f->data));
}

void CorruptFrame(Frame *f) {
    int pos = rand() % strlen(f->data);
    f->data[pos] ^= (1 << (rand() % 8));
}

int chunk_input(char *input, Frame frames[], int *frame_count) {
    int len = strlen(input);
    int idx = 0, pos = 0;
    while (pos < len) {
        Frame f;
        bzero(&f, sizeof(f));
        strncpy(f.data, input + pos, CHUNK_SIZE);
        MakeFrame(&f, f.data, seq_counter);
        frames[idx++] = f;
        seq_counter = (seq_counter + 1) % (MAX_SEQ_NUMBER + 1);
        pos += CHUNK_SIZE;
    }
    *frame_count = idx;
    return 0;
}

int main() {
    srand(time(0));
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) exit(1);

    char rec_ip[INET_ADDRSTRLEN];
    unsigned short port;
    GetAddress(rec_ip);
    GetPort(&port);

    int prob;
    printf("Enter corruption probability (%%): ");
    scanf("%d", &prob);

    struct sockaddr_in rec_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr(rec_ip)
    };

    getchar();

    while (1) {
        char input[1024];
        bzero(input, 1024);
        printf("Enter message: ");
        fgets(input, 1024, stdin);
        if (strlen(input) <= 1) continue;

        Frame frames[MAX_FRAMES];
        int total;
        chunk_input(input, frames, &total);

        int base = 0, next_seq = 0;

        while (base < total) {
            while (next_seq < base + WINDOW_SIZE && next_seq < total) {
                Frame temp = frames[next_seq];
                if (rand() % 100 < prob) {
                    CorruptFrame(&temp);
                    printf("Corrupted frame seq %d\n", temp.seq);
                } else {
                    printf("Sent frame seq %d\n", temp.seq);
                }
                sendto(sockfd, &temp, sizeof(Frame), 0, (struct sockaddr *)&rec_addr, sizeof(rec_addr));
                next_seq++;
            }

            SocketTime(sockfd, 2);
            Frame ack;
            bzero(&ack, sizeof(Frame));
            struct sockaddr_in from;
            socklen_t len = sizeof(from);
            int r = recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&from, &len);

            if (r > 0 && ack.ack) {
                printf("Received ACK for seq %d\n", ack.seq);
                while (base < total && frames[base].seq != (ack.seq + 1) % (MAX_SEQ_NUMBER + 1)) base++;
            } else {
                printf("Timeout, resending from base %d\n", frames[base].seq);
                
		int i;
		for (i = base; i < next_seq; i++) {
                    Frame temp = frames[i];
                    if (rand() % 100 < prob) {
                        CorruptFrame(&temp);
                        printf("Corrupted resend seq %d\n", temp.seq);
                    } else {
                        printf("Resent frame seq %d\n", temp.seq);
                    }
                    sendto(sockfd, &temp, sizeof(Frame), 0, (struct sockaddr *)&rec_addr, sizeof(rec_addr));
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
