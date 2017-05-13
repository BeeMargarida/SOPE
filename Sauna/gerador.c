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

unsigned int seed;
static int p = 0;
pthread_mutex_t lock;
struct timeval t1, t2;
FILE *file;
int fd[2];
int numRequests;
int countRequests = 0;
//STATISTICS VARIABLES
int gerM = 0, gerF = 0, rejM = 0, rejF = 0, descM = 0, descF = 0;

typedef struct Process{
	int p;
	char gender;
	int dur;
	int rej;
} process_t;

void openFIFOWrite() {
	do {
		fd[0] = open("/tmp/entrada",O_WRONLY) ;
		if(fd[0] == -1) sleep(1);

	} while(fd[0] == -1);
}

void openFIFORead() {
	do {
		fd[1] = open("/tmp/rejeitados",O_RDONLY/* | O_NONBLOCK*/) ;
		if(fd[1] == -1) sleep(1);

	} while(fd[1] == -1);
}

void printInFile(process_t *process, int state) {
	gettimeofday(&t2,NULL);
	double elapsedTime = (t2.tv_sec - t1.tv_sec)*1000.00;
	elapsedTime += (t2.tv_usec - t1.tv_usec);
	if(state == 0){
		char *msg = "PEDIDO";
		fprintf(file, "%.02f - %d - %d: %c - %d - %s\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
	else if(state == 1){
		char *msg = "REJEITADO";
		fprintf(file, "%.02f - %d - %d: %c - %d - %s\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
	else {
		char *msg = "DESCARTADO";
		fprintf(file, "%.02f - %d - %d: %c - %d - %s\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
}

void printStatisticsInFile() {
	fprintf(file, "\nSTATISTICS\n");
	fprintf(file, "Requests:\nTotal: %d\t F: %d\t M: %d\n", (gerM+gerF),gerF,gerM);
	fprintf(file, "Rejected:\nTotal: %d\t F: %d\t M: %d\n", (rejM+rejF),rejF,rejM);
	fprintf(file, "Discarded:\nTotal: %d\t F: %d\t M: %d\n", (descM+descF),descF,descM);
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
	pthread_mutex_lock(&lock);
	if(process->gender == 'F')
		gerF++;
	else
		gerM++;
	p++;
	pthread_mutex_unlock(&lock);
}

void * generateRequests(void * arg) {
	while(countRequests < numRequests){
		process_t process;
		makeRequest(&process, *((int *)arg));
		printInFile(&process,0);
		write(fd[0], &process, sizeof(process));
		if(errno == EAGAIN){
			printf("PIPE FULL\n");
		}
		pthread_mutex_lock(&lock);
		countRequests++;
		pthread_mutex_unlock(&lock);
	}
	return NULL;
}

void handleRejected(process_t *process){
	pthread_mutex_lock(&lock);
	if(process->rej >= 3){
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
	pthread_mutex_unlock(&lock);
}

void * receiveAnswers(void * arg){
	int n = 1;
	do{
		process_t process;
		n = read(fd[1], &process,sizeof(process));
		printf("REJ: %d - %c - %d\n", process.p, process.gender, process.dur);
		if(process.p == -1){
			close(fd[0]);
			close(fd[1]);
			return NULL;
		}
		handleRejected(&process);
		printInFile(&process,1);	
	} while(1);
	return NULL;
}

int main(int argc, char const *argv[])
{
	//Get beggining time of the program
	gettimeofday(&t1,NULL);
	if(argc != 3){
		fprintf(stderr,"Invalid number of arguments.\n");
		return -1;
	}

	//FILE TO KEEP INFORMATION
	char filename[14];
	sprintf(filename,"/tmp/ger.%d",getpid());
	file = fopen(filename, "w");

	//FIFO
	openFIFOWrite();
	openFIFORead();

	//MUTEX INITIALIZATION
	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("Mutex init failed\n");
		return 1;
	}

	//THREADS
	numRequests = atoi(argv[1]);
	pthread_t tg, tr;
	int tArg[1];
	seed = time(NULL);

	//THREADS
	int dur = atoi(argv[2]);
	tArg[0] = dur;

	process_t process;
	process.p = -1;
	process.dur = numRequests;
	write(fd[0], &process, sizeof(process));
	
	pthread_create(&tg,NULL,(void *)generateRequests, &tArg[0]);
	pthread_create(&tr,NULL,(void *)receiveAnswers, NULL);

	pthread_join(tg, NULL);
	pthread_join(tr, NULL);
	/*close(fd[0]);
	close(fd[1]);*/

	printStatisticsInFile();
	pthread_mutex_destroy(&lock);

	exit(0);
}