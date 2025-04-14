#include "headsock.h"

void str_ser(int sockfd, float error_prob, int file_id);  // receive function

int main(int argc, char *argv[])
{
    int sockfd, ret, file_id = 1;
    float error_prob = 0.0;

    struct sockaddr_in my_addr;

    if (argc == 2) {
        error_prob = atof(argv[1]);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        printf("error in socket\n");
        exit(1);
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MYUDP_PORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(my_addr.sin_zero), 8);
    ret = bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));

    if (ret == -1) { //bind socket
        printf("error in binding\n");
        exit(1);
    }

	printf("start receiving\n");

    // stop and wait implementation
    while (1) {
        printf("\nWaiting to receive file #%d...\n", file_id);
        str_ser(sockfd, error_prob, file_id);
        file_id++;
    }

    close(sockfd);
    exit(0);
}

void str_ser(int sockfd, float error_prob, int file_id)
{   
    FILE *fp;
    char recvs[DATALEN];
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    struct pack_so packet;
    struct ack_so ack;
    int n, expected_seq = 0, packet_num = 1;
    long total_bytes = 0;
    struct timeval start, end;

    sprintf(recvs, "payload/payload%d.txt", file_id);
    
    fp = fopen(recvs, "w");
    if (fp == NULL) {
        printf("File open error\n");
        return;
    }

    while (1) {
        // receive packet
        n = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&cli_addr, &cli_len);

        if (n == -1) {
            printf("Error in receiving\n");
            continue;
        }

        // simulate error
        int rand_val = rand() % 1000;
        int simulate_error = (rand_val < (int)(error_prob * 1000));

        if (!simulate_error && packet.len > 0 && packet.num == expected_seq) {
            if (packet_num) {
                gettimeofday(&start, NULL);
                packet_num = 0;
            }

            fwrite(packet.data, 1, packet.len, fp);
            total_bytes += packet.len;

            ack.num = expected_seq;
            ack.len = 2;
            ack.type = 0;
            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&cli_addr, cli_len);

            expected_seq = 1 - expected_seq;
        } else {
            // resend last ACK

            ack.num = packet.num; 
            ack.len = 2;           
            ack.type = 1;
            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&cli_addr, cli_len);
        }

        if (packet.len == 0) {
            break;
        }
    }

    gettimeofday(&end, NULL);
    float duration = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
    float throughput = total_bytes / duration;

    printf("File received: %ld bytes -> %s\n", total_bytes, recvs);
    printf("Transfer time: %.2f ms\n", duration);
    printf("Throughput: %.2f bytes/ms\n", throughput);
    
    fclose(fp);
}