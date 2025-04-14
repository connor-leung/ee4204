#include "headsock.h"

float str_cli(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, long *total_bytes);
void tv_sub(struct timeval *out, struct timeval *in);

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in ser_addr;
    struct hostent *sh;
    struct in_addr **addrs;
    FILE *fp;
    float time, throughput;
    long total_bytes;
    char **pptr;

    if (argc != 3)
    {
        printf("Usage: %s <server_ip> <file_path>\n", argv[0]);
        exit(0);
    }

	if ((sh=gethostbyname(argv[1]))==NULL) {             //get host's information
        printf("error when gethostbyname\n");
        exit(0);
    }

    if (argv[2] == NULL) {             //get file information
        printf("error when getting file\n");
        exit(0);
    }

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);             //create socket
    if (sockfd < 0)
    {
        printf("error in socket\n");
        exit(1);
    }

    addrs = (struct in_addr **)sh->h_addr_list;
    printf("canonical name: %s\n", sh->h_name);
	for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
		printf("the aliases name is: %s\n", *pptr);			//printf socket information
	switch(sh->h_addrtype)
	{
		case AF_INET:
			printf("AF_INET\n");
		break;
		default:
			printf("unknown addrtype\n");
		break;
	}

    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(MYUDP_PORT);
    memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
    bzero(&(ser_addr.sin_zero), 8);

    fp = fopen(argv[2], "r");
    if (fp == NULL)
    {
        printf("File not found\n");
        exit(1);
    }

    time = str_cli(fp, sockfd, (struct sockaddr *)&ser_addr, sizeof(ser_addr), &total_bytes);
    int return_time = (total_bytes/(float)time);


    // calculations 
    throughput = total_bytes / time;

    printf("Transfer time: %.2f ms\n", time);
    printf("Data sent: %ld bytes\n", total_bytes);
    printf("Throughput: %.2f bytes/ms\n", throughput);
    printf("Average transmission rate: %d bytes/ms\n", return_time);

    fclose(fp);
    close(sockfd);
    return 0;
}

float str_cli(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, long *total_bytes)
{
    struct pack_so packet;
    struct ack_so ack;
    struct timeval start, end;
    char *buffer;
    long lsize, ci = 0;
    int chunk, seq = 0;

    fseek(fp, 0, SEEK_END);
    lsize = ftell(fp);
    rewind(fp);
    printf("The file length is %d bytes\n", (int)lsize);
	printf("the packet length is %d bytes\n",DATALEN);

    //allocate memory to contain the whole file
    buffer = (char *)malloc(lsize);
    if (buffer == NULL) exit(2);

    fread(buffer, 1, lsize, fp);

    buffer[lsize] = '\0'; // null terminate the string
    gettimeofday(&start, NULL);

    while (ci < lsize)
    {
        // int chunk = (lsize - ci > DATALEN) ? DATALEN : (lsize - ci);

        if ((lsize + 1 - ci) < DATALEN)
        {
            chunk = lsize - ci;
        } else {
            chunk = DATALEN;
        }
        memcpy(packet.data, buffer + ci, chunk);

        // stop and wait implementation
        packet.len = chunk;
        packet.num = seq;

        sendto(sockfd, &packet, sizeof(packet), 0, addr, addrlen); //send first packet

        while (1) {
            sendto(sockfd, &packet, sizeof(packet), 0, addr, addrlen);
            printf("Sent packet %d\n", seq);
        
            int n = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);
            if (n > 0) {
                if (ack.type == 0 && ack.num == seq) {
                    printf("Received ACK for packet %d, len %d\n", ack.num, ack.len);
                    ci += chunk;
                    seq = 1 - seq;
                    break;
                } else if (ack.type == 1) {
                    printf("Received NAK for packet %d, len %d. Resending...\n", ack.num, ack.len);
                }
            }
        }
    }

    packet.len = 0;
    packet.num = seq;
    sendto(sockfd, &packet, sizeof(packet), 0, addr, addrlen);

    gettimeofday(&end, NULL);
    tv_sub(&end, &start);

    *total_bytes = lsize;
    return end.tv_sec * 1000.0 + end.tv_usec / 1000.0;
}

void tv_sub(struct timeval *out, struct timeval *in)
{
    if ((out->tv_usec -= in->tv_usec) < 0)
    {
        --out->tv_sec;
        out->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}