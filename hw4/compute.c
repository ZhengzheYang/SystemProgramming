#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "hw3.h"

int shmid;
int msqid;
int process_index;
sharedMemory *shm;


int test_perfect(int n);
void sigquit_handler(int signum);
void prepare_shm_msg();
void send_rcv(message msg);
int whichBit(int num);
int whichInt(int num);
void prepare_signal(struct sigaction signal);

int main(int argc, char *argv[]) {
	if(argc < 2) {
		printf("Not Enough Arguments.\n");
		exit(EXIT_FAILURE);
	}
	int start_num = atoi(argv[1]);
	if(start_num < 2 || start_num >= TOTAL_NUMBER) {
		perror("Out of range.");
		exit(EXIT_FAILURE);
	}

	struct sigaction signal;
	prepare_signal(signal);

	int whichint = 0;
	int whichbit = 0;
	int wrap = 0;
	int test_num;
	
	prepare_shm_msg();

	message msg;
	send_rcv(msg);

	test_num = start_num;
	while(1) {
		if(test_num > TOTAL_NUMBER) {
			test_num = 2;
			wrap = 1;
		}

		if(test_num == start_num && wrap == 1) {
			sigquit_handler(SIGQUIT);
		}

		whichbit = whichBit(test_num);
		whichint = whichInt(test_num);

		if(!(shm->bitmap[whichint] & (1 << whichbit))) {
			if(test_perfect(test_num)) {
				msg.type = PERFECT;
				msg.mtext = test_num;
				if(msgsnd(msqid, &msg, sizeof(msg.mtext), 0) == -1) {
					perror("Couldn't send perfect number");
					exit(EXIT_FAILURE);
				}
				shm->data[process_index].perfectNumFound++;
			}
			shm->bitmap[whichint] |= (1 << whichbit);
			shm->data[process_index].candidatesTested++;
		} else {
			shm->data[process_index].candidatesNotTested++;
		}
		test_num++;
	}
}

int whichBit(int num) {
	return num % 32;
}

int whichInt(int num) {
	return num / 32;
}

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

void sigquit_handler(int signum) {
	shm->summary[0] += shm->data[process_index].perfectNumFound;
	shm->summary[1] += shm->data[process_index].candidatesTested;
	shm->summary[2] += shm->data[process_index].candidatesNotTested;
	if(process_index != -1) {
        shm->data[process_index].pid = 0;
        shm->data[process_index].perfectNumFound = 0;
        shm->data[process_index].candidatesTested = 0;
        shm->data[process_index].candidatesNotTested = 0;
	}
	exit(EXIT_SUCCESS);
}

void prepare_shm_msg() {
	if((shmid = shmget(SHMKEY, sizeof(sharedMemory), 0)) == -1) {
		perror("Shared Memory doesn't exist");
		exit(EXIT_FAILURE);
	}
	if((shm = shmat(shmid, NULL, 0)) == (void *)-1) {
		perror("Couldn't attach shared memory");
		exit(EXIT_FAILURE);
	}
	if((msqid = msgget(MSQKEY, 0)) == -1) {
		perror("Couldn't get message queue");
		exit(EXIT_FAILURE);
	}
}

void send_rcv(message msg) {
	int curPID;
	curPID = getpid();
	msg.type = GET_PROCESS;
	msg.mtext = curPID;

	if(msgsnd(msqid, &msg, sizeof(msg.mtext), 0) == -1) {
		perror("Couldn't send message");
		exit(EXIT_FAILURE);
	}

	msgrcv(msqid, (void *)&msg, sizeof(msg.mtext), PROCESS_INDEX, 0);
	process_index = msg.mtext;
	if(shm->data[process_index].pid != curPID) {
		perror("Process ID does not match");
		exit(EXIT_FAILURE);
	}
}

void prepare_signal(struct sigaction signal) {
	memset(&signal, 0, sizeof(signal));
	signal.sa_handler = sigquit_handler;
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
