#include "headsock.h"

#define BACKLOG 1000

void str_ser(int sockfd);

int main(int argc, char *argv[])
{
    // setting up socket
	// int sockfd, con_fd, ret;
    int sockfd, ret;

	struct sockaddr_in my_addr;
	// struct sockaddr_in their_addr;
	// int sin_size;

    // pid_t pid;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {			//create socket
		printf("error in socket");
		exit(1);
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYUDP_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(my_addr.sin_zero), 8);
    ret = bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));
	if (ret == -1) {           //bind socket
		printf("error in binding");
		exit(1);
	}


	printf("start receiving\n");

	while(1) {
        printf("waiting for data\n");
		str_ser(sockfd);                        // send and receive
	}
	close(sockfd);
	exit(0);
}


void str_ser(int sockfd)
{
	char buf[BUFSIZE];
	FILE *fp;
	char recvs[DATALEN];
	// struct ack_so ack;
	int end, n = 0;
	long lseek=0;
	end = 0;
	
	while(!end)
	{
		if ((n= recv(sockfd, &recvs, DATALEN, 0))==-1)                                   //receive the packet
		{
			printf("error when receiving\n");
			exit(1);
		}
		if (recvs[n-1] == '\0')									//if it is the end of the file
		{
			end = 1;
			n --;
		}
		memcpy((buf+lseek), recvs, n);
		lseek += n;
	}
	// ack.num = 1;
	// ack.len = 0;
	// if ((n = send(sockfd, &ack, 2, 0))==-1)
	// {
	// 		printf("send error!");								//send the ack
	// 		exit(1);
	// }
	if ((fp = fopen ("myUDPreceive.txt","wt")) == NULL)
	{
		printf("File doesn't exit\n");
		exit(0);
	}
	fwrite (buf , 1 , lseek , fp);					//write data into file
	fclose(fp);
	printf("a file has been successfully received!\nthe total data received is %d bytes\n", (int)lseek);
}
