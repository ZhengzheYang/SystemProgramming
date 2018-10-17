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

void send_report();
void sig_handler(int signum);
void prepare_signal(struct sigaction signal);
void initialize_clients(clients_info clients_info_list[]);


struct pollfd fdinfo[MAXIMUM_CLIENTS];
char *client_arr[MAXIMUM_CLIENTS][50];
clients_info clients_info_list[MAXIMUM_CLIENTS];
int isKilledByReport = 0;
int perfect_count = 0;
int host_count = 0;
int report_idx = -1;
int fd;
int connected = 0;
int tested = 0;
char report_str[REPORT_SIZE];


int main(int argc, char *argv[]) {
	if(argc < 2) {
		printf("Not Enough Arguments.\n");
		exit(EXIT_FAILURE);
	}
	/*prepare signal handler*/
	struct sigaction signal;
	prepare_signal(signal);

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	int socket_fd;

	int clients[MAXIMUM_CLIENTS];
	struct hostent *h = malloc(sizeof(struct hostent));

	memset(&fdinfo, 0, sizeof(fdinfo));

	sin.sin_addr.s_addr= INADDR_ANY;
	sin.sin_port = atoi(argv[1]);

        /*bind the file descriptor to the host name and address*/
	if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("Assign socket fd");
			exit(EXIT_FAILURE);
	}

	if (bind(socket_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("Bind");
		exit(EXIT_FAILURE);
	}

        /*waiting for clients to connect*/
	listen(socket_fd, MAXIMUM_CLIENTS - 1);

	/* Accept Socket*/
	fdinfo[0].fd = socket_fd;     
	fdinfo[0].events = POLLIN;

	for(int i = 1; i < MAXIMUM_CLIENTS; i++) {
		fdinfo[i].fd = -1;
		fdinfo[i].events = POLLIN;
	}

	initialize_clients(clients_info_list);

	while(1) {
		poll(fdinfo, MAXIMUM_CLIENTS, -1);
		int i;
                
                /*initiate the first connection*/
		if(fdinfo[0].revents & POLLIN) {
			int len = sizeof(sin);
			if((fd = accept(fdinfo[0].fd, (struct sockaddr *)&sin, &len)) == -1) {
				perror("Accept");
				exit(EXIT_FAILURE);
			}

			for (i = 1;i < MAXIMUM_CLIENTS;i++) {
				if (fdinfo[i].fd == -1) 
					break;
			}
			fdinfo[i].fd = fd;
			clients[i] = 0;
			if((h = gethostbyaddr((char *) &sin.sin_addr.s_addr, sizeof(sin.sin_addr.s_addr), AF_INET)) == NULL) {
				perror("Host");
				exit(EXIT_FAILURE);
			}
			strcpy((char *) &clients_info_list[i].hostname, h->h_name);
		} else { 
			for(i = 1; i < MAXIMUM_CLIENTS; i++) {
				if((fdinfo[i].fd != -1) && fdinfo[i].revents) {
					char buf[BUF_SIZE];
					int read_count;
					if((read_count = read(fdinfo[i].fd, buf, BUF_SIZE)) > 0) {
						char *command = strtok(buf, "|");
						/*receive start request and send range to compute*/
						if(!strcmp(command, "start_req")) {
							snprintf(buf, sizeof(buf), "start_req");
							command = strtok(NULL, "|");
							long thread_num = atol(command);
							for(int thread = 0; thread < thread_num; thread++) {
								char range_buf[BUF_SIZE];
								int new_range_start = tested + 1;
								int new_range_end = new_range_start + DEFAULT_RANGE;
								snprintf(range_buf, sizeof(range_buf), "|%d|%d", new_range_start, DEFAULT_RANGE);
								strcat(buf, range_buf);

								clients_info_list[i].thread_num = thread_num;
								clients_info_list[i].tested_candidates += DEFAULT_RANGE + 1;
								clients_info_list[i].isCompute = 1;
								range tmpRange;
								tmpRange.range_start = new_range_start;
								tmpRange.range_end = new_range_end;
								clients_info_list[i].current_range[thread] = tmpRange;
								tested += DEFAULT_RANGE + 1;
							}
							write(fdinfo[i].fd, buf, strlen(buf) + 1);
							host_count++;
						}
						/*report request a report*/
						else if(!strcmp(command, "report")) {
							send_report(fdinfo[i].fd);
						}
						/*receive kill signal from report*/
						else if(!strcmp(command, "quit")) {
							printf("%s\n", "Manage received kill message");
							for(int j = 0; j < MAXIMUM_CLIENTS; j++) {
								if(clients_info_list[j].isCompute) {
									write(fdinfo[j].fd, "quit|", BUF_SIZE);
								}
							}
							isKilledByReport = 1;
						}
						/*found a perfect number*/
						else if(!strcmp(command, "perfect")) {
							command = strtok (NULL, "|");
							long perfect = atol(command);
							clients_info_list[i].perfect_found[perfect_count] = perfect;
							perfect_count++;
						}
						/*send the new range to compute*/
						else if(!strcmp(command, "new_range")) {
							command = strtok (NULL, "|");
							long new_range_start = atol(command);
							command = strtok(NULL, "|");
							long new_range_size = atol(command);
							range tmpRange;
							tmpRange.range_start = tested + 1;
							tmpRange.range_end = tmpRange.range_start + new_range_size;
							snprintf(buf, sizeof(buf), "new_range|%ld|%ld", tmpRange.range_start, new_range_size);
							write(fdinfo[i].fd, buf, strlen(buf) + 1);
							clients_info_list[i].tested_candidates += new_range_size + 1;
							for(int tmp = 0; tmp < clients_info_list[i].thread_num; tmp++) {
								if(clients_info_list[i].current_range[tmp].range_end == new_range_start) {
									clients_info_list[i].current_range[tmp] = tmpRange;
									break;
								}
							}
							tested += new_range_size + 1;
						}
						/*compute tell manage that it's terminating*/
						else if(!strcmp(command, "terminating")) {
							host_count--;
							if(isKilledByReport && (host_count == 0)) {
								send_report(fdinfo[i].fd);
								exit(EXIT_SUCCESS);
							}
						}
						
					} else {
						close(fdinfo[i].fd);
						fdinfo[i].fd = -1;
					}
				}
			}
		}
	}
}

void sig_handler(int signum)
{
	int j;
	for(j =1; j < MAXIMUM_CLIENTS; j++){
		if(clients_info_list[j].isCompute == 1){
			write(fdinfo[j].fd, "quit|", BUF_SIZE);
		}
		isKilledByReport = 0;
	}
	exit(EXIT_SUCCESS);
}

/*prepare signal handler*/
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

/*query the report with the required info and send to report*/
void send_report(int fd) {
	int j;
	char report[REPORT_SIZE];
	memset(&report, 0, sizeof(report));
	for(j = 0; j < MAXIMUM_CLIENTS; j++){
		if(clients_info_list[j].isCompute == 1){
			for(int temp = 0; temp < 10; temp++) {
				if(clients_info_list[j].perfect_found[temp] != 0) {
					strcat(report, "Perfect number: ");
					char perf[BUF_SIZE];
					snprintf(perf, sizeof(perf), "%ld", clients_info_list[j].perfect_found[temp]);
					strcat(report, perf);
					strcat(report, " Found by host: ");
					strcat(report, clients_info_list[j].hostname);
					strcat(report, "\n");
				}
			}
		}
		
	}
	char temp[REPORT_SIZE];
	for(j = 0; j < MAXIMUM_CLIENTS; j++) {
		if(clients_info_list[j].isCompute == 1) {
			snprintf(temp, sizeof(temp), "Hostname: %s\nThis host has tested: %ld many numbers\nCurrently running ranges: ", clients_info_list[j].hostname, clients_info_list[j].tested_candidates);
			for(int tmp = 0; tmp < clients_info_list[j].thread_num; tmp++) {
				char range_buf[BUF_SIZE];
				snprintf(range_buf, sizeof(range_buf), "%ld->%ld ", clients_info_list[j].current_range[tmp].range_start, clients_info_list[j].current_range[tmp].range_end);
				strcat(temp, range_buf);
			}
			strcat(report, temp);
			strcat(report, "\n");
		}
	}
	
	write(fd, report, strlen(report) + 1);
}

void initialize_clients(clients_info clients_info_list[]) {
	for(int i = 0; i < MAXIMUM_CLIENTS; i++) {
		memset(&clients_info_list[i], 0, sizeof(clients_info));
	}
}
