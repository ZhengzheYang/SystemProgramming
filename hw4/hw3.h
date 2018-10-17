#define BITMAP 1048576
#define TOTAL_NUMBER 33554432
#define SHMKEY 63848
#define MSQKEY 10086
#define GET_PROCESS 1
#define GET_MANAGE 2
#define PERFECT 3
#define PROCESS_INDEX 4
#define MANAGE_PID 5
#define SUMMARY_SIZE 3
#define PERFECT_SIZE 20

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

typedef struct {
	pid_t pid;
	int perfectNumFound;
	int candidatesTested;
	int candidatesNotTested;
} process;

typedef struct {
	int bitmap[BITMAP];
	int perfectFound[PERFECT_SIZE];
	process data[PERFECT_SIZE];
	int summary[SUMMARY_SIZE];
} sharedMemory;

typedef struct {
	long type;
	int mtext;
} message;

extern int shmid;
extern int msqid;
extern sharedMemory *shm;