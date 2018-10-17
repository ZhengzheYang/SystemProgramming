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
int totalPerfect = 0;
sharedMemory *shm;

void kill_handler(int signum);
void prepare_signal(struct sigaction signal);
void create_shm_msg();

int main(int argc, char *argv[]) {
	create_shm_msg();

	struct sigaction signal;
	prepare_signal(signal);

	message msg;
	while(1) {
		msgrcv(msqid, &msg, sizeof(msg.mtext), -3, 0);
		if(msg.type == GET_PROCESS) {
			for(int i = 0; i < PERFECT_SIZE; i++) {
				if(shm->data[i].pid == 0) {
					process_index = i;
				}
			}
			if(process_index == -1) {
				kill(msg.mtext, SIGKILL);
			} else {
				shm->data[process_index].pid = msg.mtext;
				msg.type = PROCESS_INDEX;
				msg.mtext = process_index;
				if(msgsnd(msqid, &msg, sizeof(msg.mtext), 0) == -1) {
					perror("Couldn't send process index to compute. Exitting.");
					exit(EXIT_FAILURE);
				}
			}
		} else if(msg.type == GET_MANAGE) {
			msg.type = MANAGE_PID;
			msg.mtext = getpid();
			if(msgsnd(msqid, &msg, sizeof(msg.mtext), 0) == -1) {
					perror("Couldn't send manage pid to report. Exitting.");
					exit(EXIT_FAILURE);
			}
		} else if(msg.type == PERFECT){
			int found = 0;
			for(int i = 0; i < totalPerfect; i++) {
				if(msg.mtext == shm->perfectFound[i]) {
					found = 1;
					break;
				}
			}
			if(!found) {
				shm->perfectFound[totalPerfect++] = msg.mtext;
			}
		}
	}

}

void create_shm_msg(){
	if((shmid = shmget(SHMKEY, sizeof(sharedMemory), IPC_CREAT | IPC_EXCL | 0666)) == -1) {
		perror("Shared Memory cannot be created");
		exit(EXIT_FAILURE);
	}
	shm = shmat(shmid, NULL, 0);
	memset(shm->bitmap, 0, sizeof(shm->bitmap));
	memset(shm->perfectFound, 0, sizeof(shm->perfectFound));
	memset(shm->data, 0, sizeof(shm->data));
	memset(shm->summary, 0, sizeof(shm->summary));

	if((msqid = msgget(MSQKEY, IPC_CREAT | IPC_EXCL | 0666)) == -1) {
		perror("Message queue cannot be created");
		exit(EXIT_FAILURE);
	}
}

void kill_handler(int signum) {
	for(int i = 0; i < PERFECT_SIZE; i++) {
		if(shm->data[i].pid != 0) {
			if (kill(shm->data[i].pid, SIGINT) != 0) {
				printf("Failed to kill process. PID: %d", shm->data[i].pid);
				exit(EXIT_FAILURE);
			}
		}
	}

	sleep(5);
	if (shmdt(shm) != 0) {
		perror("Failed to detach shared memory");
		exit(EXIT_FAILURE);
	}
	if (shmctl(shmid, IPC_RMID, 0)) {
		perror("Faield to remove shared memory");
		exit(EXIT_FAILURE);
	}
	if (msgctl(msqid, IPC_RMID, NULL)) {
		perror("Failed to remove message queue.");
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}

void prepare_signal(struct sigaction signal) {
	memset(&signal, 0, sizeof(signal));
	signal.sa_handler = kill_handler;
	if (sigaction(SIGINT, &signal, NULL) != 0) {
		perror("SIGINT failed");
		exit(1);
	}
	if (sigaction(SIGQUIT, &signal, NULL) != 0) {
		perror("SIGQUIT failed");
		exit(1);
	}
	if (sigaction(SIGHUP, &signal, NULL) != 0) {
		perror("SIGHUP failed");
		exit(1);
	}
}