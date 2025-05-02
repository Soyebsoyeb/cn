#include "protocol.h"
#include "crc.h"
#include <time.h>

void MakeFrame(Frame *f, char *buf, int seq, int ack) {
    f->seq = seq;
    f->ack = ack;
    if (!ack) strncpy(f->data, buf, strlen(buf));
    f->crc = crc_16((unsigned char *)buf, strlen(buf));
}

void CorruptFrame(Frame *f) {
    int pos = rand() % strlen(f->data);
    f->data[pos] ^= (1 << (rand() % 8));
}

int main() {
    srand(time(0));  // Seed randomness

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (sockfd < 0) perror("Socket creation failed!\n"), exit(1);

    char rec_ip[INET_ADDRSTRLEN];
    unsigned short port;
    GetAddress(rec_ip);
    GetPort(&port);

    int prob = 0;
    printf("Enter probability of losing a frame in transit (in %%): ");
    scanf("%d", &prob);

    struct sockaddr_in rec_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr(rec_ip)
    };

    printf("Sender ready to send to %s:%hu\n", rec_ip, port);

    int seq = 0;
    getchar();  // Clear newline
    while (1) {
        char buf[MAX_SIZE];
        bzero(buf, MAX_SIZE);

        printf("Enter message: ");
        fgets(buf, MAX_SIZE, stdin);

        if (strlen(buf) <= 1) continue;

        char tmp[MAX_SIZE];
        bzero(tmp, MAX_SIZE);
        strncpy(tmp, buf, strlen(buf));

        Frame f;
        bzero(&f, sizeof(f));
        MakeFrame(&f, buf, seq, 0);

    send:
        if (rand() % 100 < prob) {   
            printf("Corrupting frame intentionally!\n");
            CorruptFrame(&f);
        } else {
            strncpy(f.data, tmp, strlen(tmp));
            f.crc = crc_16((unsigned char *)f.data, strlen(f.data));
        }

        sendto(sockfd, &f, sizeof(f), 0, (struct sockaddr *)&rec_addr, sizeof(rec_addr)); 

        Frame ack;
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);

        SocketTime(sockfd, 2);

        if (recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&from_addr, &from_len) > 0) {  
            if (ack.seq == seq) {
                printf("Successfully sent frame!\n");
                seq = !seq;  // Toggle sequence number
            } else {
                printf("Frame out of order! Resending...\n");
                goto send;
            }
        } else {
            printf("Timeout occurred. Resending!\n");
            goto send;
        }
    }
}
