#include "protocol.h"
#include "crc.h"
#include <time.h>

#define WINDOW_SIZE 4
#define CHUNK_SIZE 8
#define MAX_FRAMES 100
#define MAX_SEQ_NUMBER 7

int seq_counter = 0;

void MakeFrame(Frame *f, char *buf, int seq, int ack) {
    f->seq = seq;
    f->ack = ack;
    f->nack = 0;
    if (!ack) strncpy(f->data, buf, strlen(buf));
    f->crc = crc_16((unsigned char *)f->data, strlen(f->data));
}

void CorruptFrame(Frame *f) {
    int pos = rand() % strlen(f->data);
    f->data[pos] ^= (1 << (rand() % 8));
}

int chunk_input(char *input, Frame frames[], int *frame_count) {
    int len = strlen(input);
    int idx;
    int current_pos = 0;
    for (idx = 0; current_pos < len; idx++) {
        Frame f;
        bzero(&f, sizeof(f));
        strncpy(f.data, input + current_pos, CHUNK_SIZE);
        MakeFrame(&f, f.data, seq_counter, 0);
        frames[idx] = f;
        seq_counter = (seq_counter + 1) % (MAX_SEQ_NUMBER + 1);
        current_pos += CHUNK_SIZE;
    }
    *frame_count = idx;
    return 0;
}

int main() {
    srand(time(0));

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) perror("Socket creation failed!\n"), exit(1);

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

    printf("Sender ready to send to %s:%hu\n", rec_ip, port);

    getchar();

    while (1) {
        char input[1024];
        bzero(input, 1024);

        printf("Enter full message to send: ");
        fgets(input, 1024, stdin);
        if (strlen(input) <= 1) continue;

        Frame frames[MAX_FRAMES];
        int total_frames;
        chunk_input(input, frames, &total_frames);

        int base = 0, next_seq_num = 0;

        while (base < total_frames) {
            while (next_seq_num < base + WINDOW_SIZE && next_seq_num < total_frames) {
                Frame temp = frames[next_seq_num];
                if (rand() % 100 < prob) {
		    printf("Corrupting frame seq %d!\n", temp.seq);
                    CorruptFrame(&temp);

                }
                sendto(sockfd, &temp, sizeof(temp), 0, (struct sockaddr *)&rec_addr, sizeof(rec_addr));
                printf("Sent frame seq %d: \"%s\"\n", temp.seq, temp.data);
                next_seq_num++;
            }

            SocketTime(sockfd, 2);

            Frame ack;
            struct sockaddr_in from_addr;
            socklen_t from_len = sizeof(from_addr);

            if (recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&from_addr, &from_len) > 0) {
                if (ack.nack) {
                    printf("Received NACK for seq %d\n", ack.seq);
                    int i;
		    int start_resend = -1;
                    for (i = base; i < next_seq_num; i++) {
                        if (frames[i].seq == ack.seq) {
				/*
                            Frame temp = frames[i];
                            if (rand() % 100 < prob) {
                                CorruptFrame(&temp);
                            }
                            sendto(sockfd, &temp, sizeof(temp), 0, (struct sockaddr *)&rec_addr, sizeof(rec_addr));
                            printf("Resent frame seq %d: \"%s\"\n", temp.seq, temp.data);*/
				start_resend = i;
                            break;
                        }
                    }
		    for (i=start_resend; i<next_seq_num; i++) {
			Frame temp = frames[i];
                            if (rand() % 100 < prob) {
                                CorruptFrame(&temp);
                            }
                            sendto(sockfd, &temp, sizeof(temp), 0, (struct sockaddr *)&rec_addr, sizeof(rec_addr));
                            printf("Resent frame seq %d: \"%s\"\n", temp.seq, temp.data);
		    }
                }
                else if (ack.ack && ack.seq >= frames[base].seq) {
                    printf("Received ACK for seq %d\n", ack.seq);
                    while (base < total_frames && frames[base].seq != (ack.seq + 1) % (MAX_SEQ_NUMBER + 1)) {
                        base++;
                    }
                }
            } else {
                printf("Timeout! Resending frames from base %d\n", frames[base].seq);
                int i;
                for (i = base; i < next_seq_num; i++) {
                    Frame temp = frames[i];
                    if (rand() % 100 < prob) {
                        CorruptFrame(&temp);
                    }
                    sendto(sockfd, &temp, sizeof(temp), 0, (struct sockaddr *)&rec_addr, sizeof(rec_addr));
                    printf("Resent frame seq %d: \"%s\"\n", temp.seq, temp.data);
                }
            }
        }

        printf("Message completely sent and acknowledged!\n");
    }

    close(sockfd);
    return 0;
}
