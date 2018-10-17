#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_LEN 100

//parse character by character from stdin
int parser(int total_pipes, int fd1[][2], int fd2[][2]) {
	char buffer[MAX_LEN];
	FILE* parse_result[total_pipes];

	//close unused pipes' ends
	for(int i = 0; i < total_pipes; i++) {
		if(close(fd1[i][0]) == -1) {
			printf("Error when closing the parse read pipe\n");
			return -1;
		}
		if(close(fd2[i][0]) == -1) {
			printf("Error when closing the sort read pipe\n");
			return -1;
		}
		if(close(fd2[i][1]) == -1) {
			printf("Error when closing the sort write pipe\n");
			return -1;
		}
		parse_result[i] = fdopen(fd1[i][1], "w");
	}

	int pipe_count = 0;
	int buf_count = 0;
	int temp = scanf("%*[^a-zA-Z]");

	while(temp != EOF) {
		temp = fgetc(stdin);
		if(isalpha(temp)) {
			buffer[buf_count++] = tolower(temp);

			//truncate words after 40 characters
			if(buf_count == 40) {
				buffer[40] = '\0';
				fputs(buffer, parse_result[pipe_count % total_pipes]);
	        	fputs("\n", parse_result[pipe_count % total_pipes]);
	        	pipe_count++;
	        	buf_count = 0;
	        	buffer[0] = '\0';
	        	scanf("%*[a-zA-Z]");
			}
		} else {
			//meet a delimiter, put the word to the file stream
			buffer[buf_count] = '\0';
			if(strlen(buffer) > 3) {
				fputs(buffer, parse_result[pipe_count % total_pipes]);
	        	fputs("\n", parse_result[pipe_count % total_pipes]);
	        	pipe_count++;
	        }
        	buffer[0] = '\0';
        	buf_count = 0;
		}
	}

	for(int i = 0; i < total_pipes; i++){
        fclose(parse_result[i]);
    }

	return 0;
}

int find_next(int total_pipes, char words[][MAX_LEN]) {
	int idx = -1;
	//find the start of a valid word
	for (int i = 0; i < total_pipes; i++) {
		if (words[i][0] != '\0') {
			idx = i;
			break;
		}
	}

	//find the first word alphabetically
	for (int i = idx + 1; i < total_pipes; i++) {
		if ((words[i][0] != '\0') && (strcmp(words[i], words[idx]) < 0))
			idx = i;
	}

	return idx;
}

int suppressor(int total_pipes, int fd1[][2], int fd2[][2]) {
	char buffer[total_pipes][MAX_LEN];
	FILE* sort_result[total_pipes];
	int next_word;
	char current[MAX_LEN];
	
	int empty_count = 0;
	int repeat;

	//close the unused pipes
	for(int i = 0; i < total_pipes; i++) {
		if(close(fd1[i][0]) == -1) {
			printf("Error when closing the parse read pipe\n");
			return -1;
		}
		if(close(fd1[i][1]) == -1) {
			printf("Error when closing the sort read pipe\n");
			return -1;
		}
		if(close(fd2[i][1]) == -1) {
			printf("Error when closing the sort write pipe\n");
			return -1;
		}
	}

	//prepare the intial value for read pipe
	for(int i = 0; i < total_pipes; i++) {
		sort_result[i] = fdopen(fd2[i][0], "r");
		if(fgets(buffer[i], MAX_LEN, sort_result[i]) == NULL) {
			buffer[i][0] = '\0';
			empty_count++;
		}
	}

	next_word = find_next(total_pipes, buffer);
	strcpy(current, buffer[next_word]);
	repeat = 1;

	//put all the words from the pipe to stdout skipping repeating words and keeping track of multiplicity
	while(empty_count < total_pipes) {
		if (fgets(buffer[next_word], MAX_LEN, sort_result[next_word]) == NULL){
			empty_count++;
			buffer[next_word][0] = '\0';
		}

		next_word = find_next(total_pipes, buffer);

		if(next_word == -1) break;
		if(!strcmp(current, buffer[next_word])) {
			repeat++;
		} else {
			printf("%5d %s", repeat, current);
			strcpy(current, buffer[next_word]);
			repeat = 1;
		}
	}

	//print out the last word
	printf("%5d %s", repeat, current);

	for(int i = 0; i < total_pipes; i++){
        fclose(sort_result[i]);
    }

    return 0;
}

//the sort process
int sorter(int total_pipes, int fd1[], int fd2[]) {
	char buffer[MAX_LEN];
	
	//use parsed fd's read as stdin
	if(dup2(fd1[0], STDIN_FILENO) == -1) {
		printf("Fail to dup the read from parse\n");
		return -1;
	}

	//use sort fd's write as stdout
	if(dup2(fd2[1], STDOUT_FILENO) == -1) {
		printf("Fail to dup the write to sort\n");
		return -1;
	}

	execlp("sort", "sort", NULL);

	if(close(fd1[0]) == -1) {
		printf("Error when closing the parse read pipe\n");
		return -1;
	}

	if(close(fd2[1]) == -1) {
		printf("Error when closing the sort write pipe\n");
		return -1;
	}
	return 0;
}

int main(int argc, char *argv[]) {
	if(argc < 2) {
		printf("not enough arguments\n");
		return -1;
	}
	int total_pipes = atoi(argv[1]);
	if(total_pipes < 1) {
		printf("%s\n", "Not enough number of pipes. Error.");
		return -1;
	}
	int fd1[total_pipes][2];
	int fd2[total_pipes][2];
	
	pid_t pid[total_pipes];

	//creating pipes 
	for(int i = 0; i < total_pipes; i++) {
		if(pipe(fd1[i]) == -1) {
			printf("Error when creating pipe from parse to sort\n");
			return -1;
		}

		if(pipe(fd2[i]) == -1) {
			printf("Error when creating pipe from sort to suppress\n");
			return -1;
		}
	}

	//fork the sorting. Children will work on sorts and parent will move on to parse
	for(int i = 0; i < total_pipes; i++) {
		pid[i] = fork();
		if(pid[i] == -1) {
			printf("Error when forking\n");
			return -1;
		}
		else if(pid[i] == 0) {

			//close the pipe except the one that is being worked on
			for(int j = 0; j < total_pipes; j++) {
				if(j != i) {
					if(close(fd1[j][0]) == -1) {
						printf("Error when closing the parse read pipe\n");
						printf("%d\n", j);
						return -1;
					}

					if(close(fd2[j][1]) == -1) {
						printf("Error when closing the sort write pipe\n");
						return -1;
					}
				}
			}

			//close all the used ends
			for(int j = 0; j < total_pipes; j++) {
				if(close(fd1[j][1]) == -1) {
					printf("Error when closing the parse write pipe\n");
					return -1;
				}
			
				if(close(fd2[j][0]) == -1) {
					printf("Error when closing the sort read pipe\n");
					return -1;
				}
			}
			//sort the pipe
			sorter(total_pipes, fd1[i], fd2[i]);
			return 0;
		}
	}

	//fork the parent processes
	int temp = fork();
	if(temp == -1) {
		printf("Error when forking\n");
		return -1;
	} else if(temp == 0) {
		//children will work on this one
		suppressor(total_pipes, fd1, fd2);
		return 0;
	} else {
		//parent process
		parser(total_pipes, fd1, fd2);
	}
	//wait for the children
	while(wait(NULL) != -1);
	return 0;
}
