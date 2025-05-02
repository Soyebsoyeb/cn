#include "protocol.h"
#include "crc.h"
#define WINDOW_SIZE 1
#define MAX_SEQ_NUMBER 7

void MakeFrame(Frame *f, char *buf, int seq, int ack) {
    f->seq = seq;
    f->ack = ack;
    if (!ack) strncpy(f->data, buf, strlen(buf));
    f->crc = crc_16((unsigned char *)buf, strlen(buf));
}

void SendNACK(int sockfd, struct sockaddr_in *cli_addr, socklen_t cli_len, int seq) {
    Frame nack = {.seq = seq, .ack = 0, .nack = 1};
    sendto(sockfd, &nack, sizeof(nack), 0, (struct sockaddr *)cli_addr, cli_len);
    printf("Sent NACK for seq %d\n", seq);
}

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) perror("Socket creation failed!\n"), exit(1);

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

    if (bind(sockfd, (struct sockaddr *)&rec_addr, sizeof(rec_addr)) < 0)
        perror("Failed to bind to port!\n"), exit(1);

    printf("Receiver ready on port %hu\n", port);

    socklen_t cli_len = sizeof(cli_addr);
    int expected_seq = 0;

    while (1) {
        Frame f;
        int n = recvfrom(sockfd, &f, sizeof(f), 0, (struct sockaddr *)&cli_addr, &cli_len);
        if (n <= 0) continue;

        unsigned short crc_c = crc_16((unsigned char *)f.data, strlen(f.data));

        if (crc_c == f.crc && f.seq == expected_seq) {
            printf("Correct frame received! Seq: %d, Data: %s\n", f.seq, f.data);
            Frame ack = {.seq = expected_seq, .ack = 1, .nack = 0};
            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&cli_addr, cli_len);
            printf("Sent ACK for seq %d\n", expected_seq);
            expected_seq = (expected_seq + 1) % (MAX_SEQ_NUMBER + 1);
        } else {
		if (f.seq != expected_seq) {
			printf("Out of order frame received! Expected: %d Received: %d\n", expected_seq, f.seq);
		}
		else {
			printf("Computed (%hu) and expected (%hu) CRC values do not match!\n", crc_c, f.crc);
		}
            //printf("Frame error or wrong seq. Expected %d, got %d. Sending NACK.\n", expected_seq, f.seq);
            SendNACK(sockfd, &cli_addr, cli_len, expected_seq);
        }
    }

    close(sockfd);
    return 0;
}
