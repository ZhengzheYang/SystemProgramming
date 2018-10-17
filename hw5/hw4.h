/*THIS CODE IS MY OWN WORK. IT WAS WRITTEN WITHOUT CONSULTING A TUTOR OR CODE WRITTEN BY OTHER STUDENTS --ZHENGZHE YANG*/
#define MAXIMUM_CLIENTS 10
#define HOST_NAME 40
#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
#define DEFAULT_RANGE 20000
#define BUF_SIZE 100
#define REPORT_SIZE 1000

typedef struct {
	long range_start;
	long range_end;
} range;

typedef struct {
	int isCompute;
	int thread_num;
	long perfect_found[10];
	int perfectnum_counter;
	char hostname[HOST_NAME];
	long tested_candidates;
	range current_range[MAXIMUM_CLIENTS];
} clients_info;

extern clients_info clients_info_list[MAXIMUM_CLIENTS];

