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
int process_index = -1;
sharedMemory *shm;

void prepare_shm_msg();
void send_rcv(message msg);

int main(int argc, char *argv[]) {
	int perfectNum, totalTested;
	int tested, skipped, found;
	int currentPID;

	prepare_shm_msg();

	message msg;

	if(argc > 1) {
		if(!strcmp(argv[1], "-k")) {
			msg.type = GET_MANAGE;
			msg.mtext = 0;
			send_rcv(msg);
		}
                
	}

	for(int i = 0; i < PERFECT_SIZE; i++) {
		if(shm->perfectFound[i] != 0) {
			printf("Perfect number found: ");
			printf("%d\n", shm->perfectFound[i]);
		}
		
		
	}

	for(int i = 0; i < PERFECT_SIZE; i++) {
		if(shm->data[i].pid != 0) {
			currentPID = shm->data[i].pid;
			found = shm->data[i].perfectNumFound;
			tested = shm->data[i].candidatesTested;
			skipped = shm->data[i].candidatesNotTested;
			printf("Process id: %d ", currentPID);
			printf("found: %d ", found);
			printf("tested: %d ", tested);
			printf("skipped: %d\n", skipped);
		}
	}

	totalTested = 0;
	for(int i = 0; i < PERFECT_SIZE; i++) {
		totalTested += shm->data[i].candidatesTested;
	}
	totalTested += shm->summary[1];

	printf("Total tested: %d\n", totalTested);

	printf("Closed processes total found: %d\n", shm->summary[0]);
	printf("Closed processes total tested: %d\n", shm->summary[1]);
	printf("Closed processes total skipped: %d\n", shm->summary[2]);
}

void prepare_shm_msg() {
	if((shmid = shmget(SHMKEY, sizeof(sharedMemory), 0)) == -1) {
		perror("Shared Memory doesn't exist. Exitting.");
		exit(EXIT_FAILURE);
	}
	if((shm = shmat(shmid, NULL, 0)) == (void *)-1) {
		perror("Couldn't attach shared memory. Exitting.");
		exit(EXIT_FAILURE);
	}
	if((msqid = msgget(MSQKEY, 0)) == -1) {
		perror("Couldn't get message queue. Exitting.");
		exit(EXIT_FAILURE);
	}
}

void send_rcv(message msg) {
	if(msgsnd(msqid, &msg, sizeof(msg.mtext), 0) == -1) {
			perror("Couldn't send request to manage. Exitting.");
			exit(EXIT_FAILURE);
	}
                
	msgrcv(msqid, &msg, sizeof(msg.mtext), MANAGE_PID, 0);
	if (kill(msg.mtext, SIGINT) != 0) {
		perror("Failed to interrupt manage");
		exit(EXIT_FAILURE);
	}
}
