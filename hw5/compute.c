/*THIS CODE IS MY OWN WORK. IT WAS WRITTEN WITHOUT CONSULTING A TUTOR OR CODE WRITTEN BY OTHER STUDENTS --ZHENGZHE YANG*/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "hw4.h"

void *start_test(void *temp);
void prepare_signal(struct sigaction signal);
void sig_handler(int signum);
void prepare_timer(struct itimerval itv, struct sigaction act);

long cur_num;
int socket_fd;
long rangein15sec;
long rangein15secCounter;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*test if perfect*/
int test_perfect(int n) {
	int sum = 0;
	for(int i = 1; i < n; i++) {
		if((n % i) == 0) {
			sum += i;
		}
	}

	if(sum == n) {
		return 1;
	} else {
		return 0;
	}
}

int main(int argc, char *argv[]) {
	if(argc < 4) {
		printf("Not Enough Arguments.\n");
		exit(EXIT_FAILURE);
	}
	struct sigaction signal;
	prepare_signal(signal);

	struct sockaddr_in sin;
	long address;

	address = *(long *)(gethostbyname(argv[1])->h_addr);
	sin.sin_addr.s_addr = address;
	sin.sin_family= AF_INET;
	sin.sin_port = atoi(argv[2]);

        /*waiting to connect to server*/
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

	char buf[BUF_SIZE];
	long range_size;
	long start_point;
	int thread_num = atoi(argv[3]);
	pthread_t tid[thread_num];
	snprintf(buf, sizeof(buf), "start_req|%d", thread_num);

/*Start request*/
	write(socket_fd, buf, strlen(buf) + 1);
	read(socket_fd, buf, sizeof(buf));
	char *pch = strtok(buf, "|");
	if(strcmp(pch, "start_req") == 0) {	
		for(int i = 0; i < thread_num; i++) {
			pch = strtok(NULL, "|");	
			long start_point = atol(pch);
			pch = strtok (NULL, "|");
			long range_size = atol(pch);
			long end_point = start_point + range_size;

			range *tmpRange = malloc(sizeof(range));
			tmpRange->range_start = start_point;
			tmpRange->range_end = end_point;
                
                        /*create new thread to test*/
			pthread_t tmp_thread;
			pthread_create(&tmp_thread, NULL, start_test, (void *)tmpRange);

			tid[i] = tmp_thread;
		}
	}

        /*infinitely detecting if being told to die*/
	while(1) {
		pthread_mutex_lock(&mutex);
		if(recv(socket_fd, buf, sizeof(buf), MSG_DONTWAIT)) {
			char *cmd = strtok(buf, "|");
			if(!strcmp(cmd,"quit")) {
				printf("kill signal received\n");
				snprintf(buf, sizeof(buf), "terminating|");
				write(socket_fd, buf, strlen(buf) + 1);
				exit(EXIT_SUCCESS);
			}
			pthread_mutex_unlock(&mutex);
		}
	}
}

void *start_test(void *temp) {
	char buf[BUF_SIZE];
	range *rg;
	rg  = (range *) temp;
	int start = rg->range_start;
	int end = rg->range_end; 	
	int cur = pthread_self();
	printf("Thread %d new range starts from %d to %d\n", cur, start, end);
	

	
	/*receive the updated range and continue to work*/
	while(1) {	
		int i = 0;
		time_t begin = time(NULL);
		for(i= start; i <= end; i++)
		{
			cur_num = i;
			if(test_perfect(i))
			{

				snprintf(buf, sizeof(buf), "perfect|%ld", (long)i);
				
				write(socket_fd, buf, strlen(buf) + 1);

			}
			rangein15secCounter++;
		}

		time_t finish = time(NULL);
		int time_elapse = (int)(finish - begin);
        
                /*avoid divider to be 0*/
		if(time_elapse == 0) {
			time_elapse = 15;
		} 

		int new_range = (end - start) * 15 / time_elapse;

                /*avoid a range that is too big to compute*/
		if(new_range > DEFAULT_RANGE) {
			new_range = DEFAULT_RANGE;
		}

                /*lock to request new range*/
		pthread_mutex_lock(&mutex);
		snprintf(buf, sizeof(buf), "new_range|%d|%d", end, new_range);
		write(socket_fd, buf, strlen(buf) + 1);

		read(socket_fd, buf, sizeof(buf));
		pthread_mutex_unlock(&mutex);

		char *pch = strtok(buf, "|");
		if(!strcmp(pch, "new_range")) {
			pch = strtok (NULL, "|");
			long new_start_point = atol(pch); 
			pch = strtok (NULL, "|");
			long new_range_size = atol(pch);
			long new_end_point = new_start_point + new_range_size;
			start = new_start_point;
			end = new_end_point;
			printf("Thread %d new range starts from %d to %d\n", cur, start, end);
		} else if(strcmp(pch, "quit")) {        //edge case when quit happen to come with new range
				printf("kill signal received\n");
				snprintf(buf, sizeof(buf), "terminating|");
				write(socket_fd, buf, strlen(buf) + 1);
				exit(EXIT_SUCCESS);
		}
	}
}

/*prepare signal handler and actions*/
void prepare_signal(struct sigaction signal) {
	memset(&signal, 0, sizeof(signal));
	signal.sa_handler = sig_handler;
	if (sigaction(SIGINT, &signal, NULL) != 0) {
		perror("Interruption Error");
		exit(EXIT_FAILURE);
	}
	if (sigaction(SIGQUIT, &signal, NULL) != 0) {
		perror("Quit Error");
		exit(EXIT_FAILURE);
	}
	if (sigaction(SIGHUP, &signal, NULL) != 0) {
		perror("Hangup Error");
		exit(EXIT_FAILURE);
	}
}

void sig_handler(int signum)
{
	char buf[BUF_SIZE];
	snprintf(buf, sizeof(buf), "terminating|");
	if(write(socket_fd, buf, strlen(buf) + 1) == -1) {
		perror("Write to socket");
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}

