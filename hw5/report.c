/*THIS CODE IS MY OWN WORK. IT WAS WRITTEN WITHOUT CONSULTING A TUTOR OR CODE WRITTEN BY OTHER STUDENTS --ZHENGZHE YANG*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "hw4.h"

int socket_fd;

int main(int argc, char *argv[]) {
	if(argc < 3) {
		printf("%s\n", "Not Enough Arguments.");
		exit(EXIT_FAILURE);
	}
	struct sockaddr_in sin; /* socket address for destination */
	long address;
	long start_point;
	int i;
	
	long range_size = -1;

	/* Fill in Manager's Address */
	address = *(long *) gethostbyname(argv[1])->h_addr;
	sin.sin_addr.s_addr= address;
	sin.sin_family= AF_INET;
	sin.sin_port = atoi(argv[2]);

	while(1) {
		if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("Assign socket fd");
			exit(EXIT_FAILURE);
		}

		if(connect(socket_fd, (struct sockaddr *) &sin, sizeof (sin)) == -1) {
			printf("Waiting to connect\n");
			close(socket_fd);
			sleep(10);
			continue;
		}

		break;
	}

	printf("Sucessfully connected\n");

	if(argc == 4 && !strcmp(argv[3], "-k")) {
		printf("%s\n", "Shutting down all computes and manage");
		write(socket_fd, "quit|", BUF_SIZE);
	} else {
		write(socket_fd, "report|", BUF_SIZE);
	}

	int nread;
	char buf[REPORT_SIZE];
	memset(&buf, 0, sizeof(buf));
	read(socket_fd, buf, REPORT_SIZE);
	printf("%s", buf);
}
