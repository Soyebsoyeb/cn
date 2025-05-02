#include "protocol.h"
#include "crc.h"

void MakeFrame(Frame *f, char *buf, int seq, int ack) {
    f->seq = seq;
    f->ack = ack;
    if (!ack) strncpy(f->data, buf, strlen(buf));
    f->crc = crc_16(buf, strlen(buf));
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) perror("Socket creation failed"), exit(1);

    char rec_ip[INET_ADDRSTRLEN];
    unsigned short port;

    GetAddress(rec_ip);
    GetPort(&port);

    struct sockaddr_in rec_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr(rec_ip)
    };

    if (connect(sockfd, (struct sockaddr *)&rec_addr, sizeof(rec_addr)) < 0)
        perror("Connection failed"), exit(1);

    int seq = 0;

    while (1) {
        // 1. User enters message
        char buf[MAX_SIZE];
        bzero(buf, MAX_SIZE);

        printf("[Sender] Enter message: ");
        fgets(buf, MAX_SIZE, stdin);

        if (strlen(buf) <= 1) {
            printf("[Sender] Empty input ignored.\n");
            continue;
        }

        Frame out;
        MakeFrame(&out, buf, seq, 0);

    resend:
        write(sockfd, &out, sizeof(out));
        printf("[Sender] Frame sent with seq %d\n", out.seq);

        Frame ack;
        SocketTime(sockfd, 20);

        if (read(sockfd, &ack, sizeof(ack)) > 0 && ack.seq == seq) {
            printf("[Sender] ACK received for seq %d!\n", seq);
            seq = 1 - seq;
        } else {
            printf("[Sender] Timeout or wrong ACK. Resending...\n");
            goto resend;
        }

        // 2. Wait to receive a message from receiver
        Frame recv_frame;
        printf("[Sender] Waiting to receive...\n");

        int n = read(sockfd, &recv_frame, sizeof(recv_frame));
        if (n <= 0) {
            printf("[Sender] Connection closed or error!\n");
            break;
        }

        if (recv_frame.crc == crc_16(recv_frame.data, strlen(recv_frame.data))) {
            printf("[Sender] Received: %s", recv_frame.data);

            Frame response_ack = {.seq = recv_frame.seq, .ack = 1};
            write(sockfd, &response_ack, sizeof(response_ack));
            printf("[Sender] ACK sent for received frame.\n");
        } else {
            printf("[Sender] CRC error. Discarding received frame.\n");
        }
    }

    close(sockfd);
    return 0;
}
