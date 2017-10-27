#include <stdio.h>
#include <stdlib.h>
#include </usr/include/ar.h>
#include <unistd.h>
#include <string.h>
#include <utime.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>

#define BLOCK 1024
#define PERM 9
#define TIME 25
#define HEADER sizeof(struct ar_hdr)
#define STRING 255

int append(int argc, char *argv[]);
void create_archive(int fd);
int append_file(int argc, char *argv[], int fd);

int is_archive(int fd)
{
	char temp[SARMAG];

	if (lseek(fd, 0, SEEK_SET) != 0)
		return -1;

	if (read(fd, temp, SARMAG) != SARMAG)
		return -1;;

	if (strncmp(temp, ARMAG, sizeof(temp)) != 0)
		return -1;

	return 1;
}

int append(int argc, char *argv[]) {
	int fd = open(argv[2], O_APPEND | O_WRONLY, 0666);
	if(fd == -1) {
		fd = open(argv[2], O_CREAT | O_APPEND | O_WRONLY, 0666);
		printf("Couldn't find any archive named '%s', creating one right now...\n", argv[2]);
		if(fd == -1) {
			printf("%s\n", "Error creating archive. Return error");
			return -1;
		}
		create_archive(fd);
		append_file(argc, argv, fd);
	} else {
		append_file(argc, argv, fd);
	}
	close(fd);
	return 1;
}

void create_archive(int fd) {
	write(fd, ARMAG, SARMAG);
}

int append_file(int argc, char *argv[], int fd) {
	int file_fd;
	char buffer[BLOCK];
	struct stat file_stat;

	for(int i = 3; i < argc; i++) {
		//make sure new header start from an even index 
		if ((lseek(fd, 0, SEEK_END) % 2) == 1){
			write(fd, "\n", 1);
		}

		if((file_fd = open(argv[i], O_RDONLY)) == -1) {
			printf("No file named %s. Skipping.\n", argv[i]);
			continue;
		} else if(stat(argv[i], &file_stat) == -1){
			printf("Error getting the file info. Skipping\n");
			continue;
		} else if(!S_ISREG(file_stat.st_mode)) {	//skip if the target is not a regular file
			printf("%s is no regular file. Skipping.\n", argv[i]);
			continue;
		} else {
			char file_name[16];
			char mtime[12];
			char uid[6];
			char gid[6];
			char mode[8];
			char size[10];

			memset(file_name, 0, 16);
			memset(mtime, 0, 12);
			memset(uid, 0, 6);
			memset(gid, 0, 6);
			memset(mode, 0, 8);
			memset(size, 0, 10);

			sprintf(file_name, "%-16s", argv[i]);
    		file_name[strlen(argv[i])] = '/';
			sprintf(mtime, "%-12ld", file_stat.st_mtime);
			sprintf(uid, "%-6d", file_stat.st_uid);
			sprintf(gid, "%-6d", file_stat.st_gid);
			sprintf(mode, "%-8o", file_stat.st_mode);
			sprintf(size, "%-12ld", file_stat.st_size);

			write(fd, (char *)file_name, 16);
			write(fd, (char *)mtime, 12);
			write(fd, (char *)uid, 6);
			write(fd, (char *)gid, 6);
			write(fd, (char *)mode, 8);
			write(fd, (char *)size, 10);
			write(fd, ARFMAG, 2);

			int bytes = 0;
			while((bytes = read(file_fd, buffer, BLOCK)) != 0){
                if((write(fd, buffer, bytes) == -1)){
                    printf("Error writing %s to the archive. Exitting.", argv[i]);
                    return -1;
                }
            }
		}
		close(file_fd);
	}
	return 1;
}

char *getString(char *str, int file_name) {

	char *temp = str;
	int index = 0;
	int i = 0;
	while(i < strlen(str)){
		if (str[i] == ' ' && !file_name){
			temp[index] = '\0';
			break;
		}
		if (str[i] == '/'){
			temp[index] = '\0';
			break;
		}
		temp[index] = str[i];
		index++;
		i++;
	}
	return temp;

	// int index = 0;
	// int i = 0;
	// while(i < strlen(str)){
	// 	if (str[i] == ' ' && !file_name){
	// 		str[index] = '\0';
	// 		return str;
	// 	}
	// 	if (str[i] == '/'){
	// 		str[index] = '\0';
	// 		return str;
	// 	}
	// 	str[index] = str[i];
	// 	index++;
	// 	i++;
	// }
	// char * res = str;
	// int ptr = 0;
	// for (int i = 0; i < strlen(str); i++){
	// 	if (str[i] == ' ' && !file_name){
	// 		res[ptr] = 0;
	// 		break;
	// 	}
	// 	if (str[i] == '/'){
	// 		res[ptr] = 0;
	// 		break;
	// 	}
	// 	res[ptr++] = str[i];
	// }
	// return res;
}

char *getPermString(char *ar_mode) {
	static char str[PERM];
	strcpy(str, "---------");

	// Parse the octal string to binary number
	int perm = (int)strtol(ar_mode, NULL, 8);

	//3 bits for user
	if(perm & S_IRUSR) str[0] = 'r';
	if(perm & S_IWUSR) str[1] = 'w';
	if(perm & S_IXUSR) str[2] = 'x';

	//3 bits for group
	if(perm & S_IRGRP) str[3] = 'r';
	if(perm & S_IWGRP) str[4] = 'w';
	if(perm & S_IXGRP) str[5] = 'x';

	//3 bits for others
	if(perm & S_IROTH) str[6] = 'r';
	if(perm & S_IWOTH) str[7] = 'w';
	if(perm & S_IXOTH) str[8] = 'x';

	return str;
}

char *getTimeString(char *time) {
	time_t time_stamp = atoi(time);
    static char res[TIME];
    strftime(res, TIME, "%b %d %H:%M %Y", localtime(&time_stamp));
    return res;
}

int print_table(char *argv[], int v) {
	int fd = open(argv[2], O_RDONLY);
	if(fd == -1) {
		printf("Error opening archive name %s. Exit with error.\n", argv[2]);
		return -1;
	}

	if(is_archive(fd) == -1) {
		printf("Malformed archive: %s. Exit with error.\n", argv[2]);
		return -1;
	}
	lseek(fd, SARMAG, SEEK_SET);
	struct ar_hdr cur;
	while(read(fd, &cur, HEADER)){
		if(v){
			printf("%s %s/%s %6s %s ", getPermString(cur.ar_mode), getString(cur.ar_uid, 0), getString(cur.ar_gid, 0), getString(cur.ar_size, 0), getTimeString(cur.ar_date));
		}
		printf("%.*s\n", 16, getString(cur.ar_name, 1));
		// Make sure every new header starts with an even index
		int next_size = atoi(cur.ar_size);
		if(next_size % 2 == 1) next_size++;
		lseek(fd, next_size, SEEK_CUR);

	}
	close(fd);
}

int extract_file(int fd, int file_fd, char *file_name, int file_size, mode_t file_mode, time_t file_mtime, int o) {
	char buffer[BLOCK];
    int bytes = 0;
    // while (bytes < file_size){
    // 	int tmp;
    // 	if(file_size - bytes < BLOCK) tmp = file_size - bytes;
    // 	else tmp = BLOCK;
    // 	read(fd, buffer, tmp);
    // 	if (write(file_fd, buffer, tmp) == -1){
    // 		printf("Error writing into %s. Exitting with error.\n", file_name);
    // 		return -1;
    // 	}
    // 	bytes += tmp;
    // }
    while (bytes < file_size){
    	int tmp;
    	if(bytes + BLOCK > file_size) {
    		tmp = read(fd, buffer, file_size - bytes);
	    	if (write(file_fd, buffer, tmp) == -1){
    			printf("Error writing into %s. Exitting with error.\n", file_name);
    			return -1;
    		}
		} else {
			tmp = read(fd, buffer, BLOCK);
			if (write(file_fd, buffer, BLOCK) == -1){
    			printf("Error writing into %s. Exitting with error.\n", file_name);
    			return -1;
    		}
		}
		bytes += tmp;
    }
    close(file_fd);

    if(o){
    	struct utimbuf newTime;
    	newTime.actime = file_mtime;
    	newTime.modtime = file_mtime;
    	if (utime(file_name, &newTime) == -1){
    		printf("Error restoring file %s. Exitting with error.\n", file_name);
    		return -1;
    	}
	}
}

int extract(int argc, char *argv[], int o){
	int fd = open(argv[2], O_RDONLY);
	if (fd == -1){
		printf("Error reading archive file %s. Exitting with error.\n", argv[1]);
		return -1;
	}

	if(is_archive(fd) == -1) {
		printf("Malformed archive: %s. Exit with error.\n", argv[2]);
		return -1;
	}

	lseek(fd, SARMAG, SEEK_SET);
	struct ar_hdr cur;
	int extracted[argc];
	memset(extracted, 0, sizeof(extracted));
	while(read(fd, &cur, HEADER)){
		char* file_name = getString(cur.ar_name, 1);
		int file_size = atoi(cur.ar_size);
		mode_t file_mode = strtol(cur.ar_mode, NULL, 8);
		time_t file_mtime = atoi(cur.ar_date);	

		for(int i = 3; i < argc; i++){
			if(!strcmp(argv[i], file_name) && extracted[i] == 0) {
				extracted[i] = 1;
				int file_fd;
				if ((file_fd = open(file_name, O_TRUNC | O_WRONLY | O_CREAT, file_mode)) == -1){
					printf("Error creating file %s. Exitting with error.\n", file_name);
					return -1;
				}
				extract_file(fd, file_fd, file_name, file_size, file_mode, file_mtime, o);
	    	}	
		}
		if(file_size % 2 == 1) file_size++;
		lseek(fd, file_size, SEEK_CUR);
	}
	close(fd);
}

int delete(int argc, char *argv[]) {
	int old_fd;
	int new_fd;
	struct ar_hdr hdr;
	char buffer[BLOCK];
	if((old_fd = open(argv[2], O_RDONLY)) == -1) {
		printf("Error opening archive named %s. Exitting with error.", argv[2]);
		return -1;
	}

	if(is_archive(old_fd) == -1) {
		printf("Malformed archive: %s. Exit with error.\n", argv[2]);
		return -1;
	}

	struct stat old_stat;
	struct ar_hdr tmp_hdr;

	fstat(old_fd, &old_stat);
	unlink(argv[2]);
	if((new_fd = open(argv[2], O_CREAT | O_WRONLY | O_APPEND, old_stat.st_mode)) == -1) {
		printf("Error creating temp archive %s. Exitting with error.", argv[2]);
		return -1;
	}
	write(new_fd, ARMAG, SARMAG);
	lseek(old_fd, SARMAG, SEEK_SET);
	int deleted[argc];
	memset(deleted, 0, sizeof(deleted));
	while(read(old_fd, &hdr, HEADER)) {
		tmp_hdr = hdr;
		char* file_name = getString(hdr.ar_name, 1);
		int file_size = atoi(hdr.ar_size);

		int skipped = 0;
		for(int i = 3; i < argc; i++){
			if(!strcmp(argv[i], file_name) && deleted[i] == 0) {
				deleted[i] = 1;
				skipped = 1;
				break;
	    	}
		}
		if(skipped == 0) {
			//printf("%s\n", "genjiiswithyou");
			write(new_fd, &tmp_hdr, HEADER);
			int bytes = 0;
			while (bytes < file_size){
		    	int tmp;
		    	if(bytes + BLOCK > file_size) {
		    		tmp = read(old_fd, buffer, file_size - bytes);
			    	if (write(new_fd, buffer, tmp) == -1){
		    			printf("Error writing into %s. Exitting with error.\n", file_name);
		    			return -1;
		    		}
	    		} else {
	    			tmp = read(old_fd, buffer, BLOCK);
	    			if (write(new_fd, buffer, BLOCK) == -1){
		    			printf("Error writing into %s. Exitting with error.\n", file_name);
		    			return -1;
		    		}
	    		}
    			bytes += tmp;

    		}
    		if (lseek(old_fd, 0, SEEK_CUR) % 2 == 1){
				lseek(old_fd, 1, SEEK_CUR);
			}
			if (lseek(new_fd, 0, SEEK_CUR) % 2 == 1){
				write(new_fd, "\n", 1);
			}
		} else {
			if(file_size % 2 == 1)
				file_size++;
			lseek(old_fd, file_size, SEEK_CUR);
		}
	}
	close(new_fd);
	close(old_fd);
}

int main(int argc, char *argv[]){
    int i;
    int opt;
    int flag;
    if(argc >= 3) {
        switch (argv[1][0]) {
            case 't':
            	printf("flag %s has been set \n",argv[1]);
            	if(argv[1][1] == 'v')
            		print_table(argv, 1);
            	else 
            		print_table(argv, 0);
                break;
            case 'q':
            	if(argc < 4) {
            		printf("%s\n", "Not enough arguments. Error.");
            		return -1;
            	}
                printf("flag %s has been set \n",argv[1]);
                append(argc, argv);
                break;
            case 'x':
		if(argc < 4) {
            		printf("%s\n", "Not enough arguments. Error.");
            		return -1;
            	}
            	printf("flag %s has been set \n",argv[1]);
            	if(argv[1][1] == 'o')
                	extract(argc, argv, 1);
                else
                	extract(argc, argv, 0);
                break;
            case 'd':
		if(argc < 4) {
            		printf("%s\n", "Not enough arguments. Error.");
            		return -1;
            	}
                printf("flag %s has been set \n",argv[1]);
                delete(argc, argv);
                break;
            case 'A':
		if(argc > 3) {
            		printf("%s\n", "Too many arguments. Error.");
            		return -1;
            	}
                printf("flag %s has been set \n",argv[1]);
                DIR *cur_dir;
                struct dirent *dir_ent;
                static char *temp[STRING];
                int index = 3;
                cur_dir = opendir(".");
                if(cur_dir == NULL) return -1;
                temp[2] = argv[2];
                while((dir_ent = readdir(cur_dir)) != NULL) {
                	if(strcmp(argv[2], dir_ent->d_name) != 0 && dir_ent->d_type == DT_REG) {
                		temp[index] = dir_ent->d_name;
                		index++;
                	}
                }
                append(index, temp);
                break;
            default: /* '?' */
                fprintf(stderr, "No option exisits\n");
                return -1;
        }
    } else {
    	printf("%s\n", "Not enough arguments. Error.");
    	return -1;
    }
}
