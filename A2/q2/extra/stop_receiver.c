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
    }, cli_addr;

    bzero(&cli_addr, sizeof(cli_addr));

    if (bind(sockfd, (struct sockaddr *)&rec_addr, sizeof(rec_addr)) < 0)
        perror("Failed to bind"), exit(1);

    listen(sockfd, 1);
    socklen_t cli_len = sizeof(cli_addr);
    int connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_len);
    if (connfd < 0) perror("Accept failed"), exit(1);

    int seq = 0;

    while (1) {
        // 1. Receive frame
        Frame recv_frame;
        printf("[Receiver] Waiting for frame...\n");

        int n = read(connfd, &recv_frame, sizeof(recv_frame));
        if (n <= 0) {
            printf("[Receiver] Connection closed or error!\n");
            break;
        }

        if (recv_frame.crc == crc_16(recv_frame.data, strlen(recv_frame.data))) {
            printf("[Receiver] Received: %s", recv_frame.data);

            Frame ack = {.seq = recv_frame.seq, .ack = 1};
            write(connfd, &ack, sizeof(ack));
            printf("[Receiver] ACK sent for seq %d\n", recv_frame.seq);
        } else {
            printf("[Receiver] CRC error. Discarding frame.\n");
            continue; // Wait for correct frame
        }

        // 2. Ask user if they want to send
        printf("Send message? (y/n): ");
        char choice;
        scanf(" %c", &choice);

        if (choice == 'y' || choice == 'Y') {
            char buf[MAX_SIZE];
            getchar(); // Clear newline
            printf("Enter message: ");
            fgets(buf, MAX_SIZE, stdin);

            Frame out;
            MakeFrame(&out, buf, seq, 0);

        resend:
            write(connfd, &out, sizeof(out));
            printf("[Receiver] Frame sent with seq %d.\n", out.seq);

            Frame ack;
            SocketTime(connfd, 20);
            if (read(connfd, &ack, sizeof(ack)) > 0 && ack.seq == seq) {
                printf("[Receiver] ACK received for sent frame!\n");
                seq = 1 - seq;
            } else {
                printf("[Receiver] Timeout or wrong ACK. Resending...\n");
                goto resend;
            }
        }
    }

    close(connfd);
    close(sockfd);
    return 0;
}
