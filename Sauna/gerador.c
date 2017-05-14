#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/file.h> 
#include <time.h>
#include <fcntl.h>
#include <errno.h>

int fd[2];
pthread_t tid[2];
struct timespec start, end;
FILE *file;

unsigned int seed;
int numMaxRequests;
int p = 0;

int gerM = 0, gerF = 0, rejM = 0, rejF = 0, descM = 0, descF = 0;

typedef struct Process{
	int p;
	char gender;
	int dur;
	int rej;
} process_t;


int openFIFOWrite() {
	if(mkfifo("/tmp/entrada",0660) < 0){
		if(errno == EEXIST){
			printf("FIFO '/tmp/entrada' exists already.\n");
		}
		else {
			printf("Impossible to create FIFO.\n");
			return -1;
		}
	}
	do {
		fd[0] = open("/tmp/entrada", O_WRONLY);	

	} while(fd[0] == -1);
	return 0;
}

int openFIFORead() {
	if(mkfifo("/tmp/rejeitados",0660) < 0){
		if(errno == EEXIST){
			printf("FIFO '/tmp/rejeitados' exists already.\n");
		}
		else {
			printf("Impossible to create FIFO.\n");
			return -1;
		}
	}
	do {
		fd[1] = open("/tmp/rejeitados", O_RDONLY);	

	} while(fd[1] == -1);
	return 0;
}

void printInFile(process_t *process, int state) {
	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
	double elapsedTime = (end.tv_sec - start.tv_sec) * 1000000.00 + (end.tv_nsec - start.tv_nsec) / 1000.00;
	if(state == 0){
		char *msg = "PEDIDO";
		fprintf(file, "%-10.02f - %6d -  %-5d: %-3c - %-5d - %-12s\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
	else if(state == 1){
		char *msg = "REJEITADO";
		fprintf(file, "%-10.02f - %6d - %-5d: %-3c - %-5d - %-12s\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
	else {
		char *msg = "DESCARTADO";
		fprintf(file, "%-10.02f - %6d - %-5d: %-3c - %-5d - %-12s\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
}

void printStatisticsInFile() {
	fprintf(file, "\nSTATISTICS\n");
	fprintf(file, "Requests:\nTotal: %d\t F: %d\t M: %d\n", (gerM+gerF),gerF,gerM);
	fprintf(file, "Rejected:\nTotal: %d\t F: %d\t M: %d\n", (rejM+rejF),rejF,rejM);
	fprintf(file, "Discarded:\nTotal: %d\t F: %d\t M: %d\n", (descM+descF),descF,descM);
}

void printStatisticsInSTD() {
    printf("\nSTATISTICS\n");
    printf("Requests:\nTotal: %d\t F: %d\t M: %d\n", (gerM+gerF),gerF,gerM);
    printf("Rejected:\nTotal: %d\t F: %d\t M: %d\n", (rejM+rejF),rejF,rejM);
    printf("Discarded:\nTotal: %d\t F: %d\t M: %d\n", (descM+descF),descF,descM);
}

void makeRequest(process_t *process, int num) {
	seed = seed + 1;
	int dur = (rand_r(&seed) % num) + 1;
	int g = (rand_r(&seed) % 2);
	char *gender = malloc(sizeof(char));
	if(g == 0)
		*gender = 'F';
	else
		*gender = 'M';

	process->p = p;
	process->gender = *gender;
	process->dur = dur;
	process->rej = 0;
	if(process->gender == 'F')
		gerF++;
	else
		gerM++;
	p++;
}

void handleRejected(process_t *process){
	if(process->rej == 3){
		if(process->gender == 'F')
			descF++;
		else
			descM++;
		printInFile(process,-1);
		return;
	}
	else{
		if(process->gender == 'F')
			rejF++;
		else
			rejM++;
		write(fd[0],process,sizeof(*process));
	}
}

void * receiveAnswers(void * arg) {
	do {
		process_t *process = (process_t *) malloc(sizeof(process_t));
		read(fd[1], process, sizeof(*process));
		if(process->p == -1){
			return NULL;
		}
		handleRejected(process);
		printInFile(process,1);
	} while(1);
	return NULL;
}


void * generateRequests(void * arg){
	int num = *((int *) arg);
	int countRequests = 0;
	while(countRequests < numMaxRequests){
		process_t *process = (process_t *) malloc(sizeof(process_t));
		makeRequest(process, num);
		printInFile(process,0);
		write(fd[0], process, sizeof(*process));
		countRequests++;
	}
	return NULL;
}

int main(int argc, char const *argv[])
{
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	if(argc != 3){
		printf("Wrong number of arguments (3)\n");
		exit(1);
	}

	seed = time(NULL);

	char filename[14];
	sprintf(filename,"/tmp/ger.%d",getpid());
	file = fopen(filename, "w");

	openFIFOWrite();
	openFIFORead();

	numMaxRequests = atoi(argv[1]);
	int maxDur[1];
	maxDur[0] = atoi(argv[2]);

	process_t *process = (process_t *)malloc(sizeof(process_t));
	process->p = -1;
	process->dur = numMaxRequests;
	write(fd[0],process,sizeof(*process));

	pthread_create(&tid[0],NULL,(void *) generateRequests, &maxDur[0]);
	pthread_create(&tid[1],NULL,(void *) receiveAnswers, NULL);

	pthread_join(tid[0],NULL);
	pthread_join(tid[1],NULL);

	close(fd[0]);
	close(fd[1]);

	printStatisticsInFile();
	printStatisticsInSTD();

	if (unlink("/tmp/rejeitados")<0)
		printf("\nError when destroying FIFO '/tmp/rejeitados'\n");
	else
		printf("\nFIFO '/tmp/rejeitados' has been destroyed\n"); 

	if (unlink("/tmp/entrada")<0)
		printf("Error when destroying FIFO '/tmp/entrada'\n");
	else
		printf("FIFO '/tmp/entrada' has been destroyed\n"); 

	exit(0);
}