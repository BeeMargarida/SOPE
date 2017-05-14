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
struct timeval t1, t2;
int numMaxCli;
FILE *file;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t nlock = PTHREAD_MUTEX_INITIALIZER;

int number;
char currGender = '\0';
int clientCount = 0;

int pedF = 0, pedM = 0, rejF = 0, rejM = 0, serF = 0, serM = 0;

typedef struct Process{
	int p;
	char gender;
	int dur;
	int rej;
} process_t;

int openFIFORead() {
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
		fd[0] = open("/tmp/entrada", O_RDONLY);	
	} while(fd[0] == -1);
	return 0;
}

int openFIFOWrite() {
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
		fd[1] = open("/tmp/rejeitados", O_WRONLY);	

	} while(fd[1] == -1);
	return 0;
}

void printInFile(process_t *process, int state) {
	gettimeofday(&t2,NULL);
	double elapsedTime = (t2.tv_sec - t1.tv_sec)*1000;
	elapsedTime += (t2.tv_usec - t1.tv_usec);
	if(state == 0){
		char *msg = "RECEBIDO";
		fprintf(file, "%-10.02f - %6d  %-5d: %-3c - %-5d - %-12s\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
	else if(state == 1){
		char *msg = "SERVIDO";
		fprintf(file, "%-10.02f - %6d  %-5d: %-3c - %-5d - %-12s\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
	else {
		char *msg = "REJEITADO";
		fprintf(file, "%-10.02f - %6d  %-5d: %-3c - %-5d - %-12s\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
}

void printStatisticsInFile() {
	fprintf(file, "\nSTATISTICS\n");
	fprintf(file, "Requests:\nTotal: %d\t F: %d\t M: %d\n", (pedM+pedF),pedF,pedM);
	fprintf(file, "Rejected:\nTotal: %d\t F: %d\t M: %d\n", (rejM+rejF),rejF,rejM);
	fprintf(file, "Served:\nTotal: %d\t F: %d\t M: %d\n", (serM+serF),serF,serM);
}

void * processRequests(void * arg){
	process_t *process = (process_t *)malloc(sizeof(process_t));
	process = (process_t *) arg;
	pthread_mutex_lock(&lock);
	printInFile(process,0);
	if(process->gender == 'F')
		pedF++;
	else
		pedM++;
	pthread_mutex_unlock(&lock);

	if(((currGender == process->gender) || (currGender == '\0')) && (clientCount != numMaxCli)){
		pthread_mutex_lock(&lock);
		clientCount++;
		currGender = process->gender;
		if(process->gender == 'F')
			serF++;
		else
			serM++;	
		printInFile(process,1);
		pthread_mutex_unlock(&lock);

		sleep(process->dur/1000);

		pthread_mutex_lock(&lock);
		clientCount--;		
		if(clientCount == 0)
			currGender = '\0';
		pthread_mutex_unlock(&lock);
	}
	else {
		process->rej++;
		pthread_mutex_lock(&nlock);
		if(process->rej < 3)
			number++;
		pthread_mutex_unlock(&nlock);

		pthread_mutex_lock(&lock);
		if(process->gender == 'F')
			rejF++;
		else
			rejM++;
		pthread_mutex_unlock(&lock);

		pthread_mutex_lock(&lock);
		printInFile(process,-1);
		pthread_mutex_unlock(&lock);

		write(fd[1],process,sizeof(*process));
	}
	free(process);
	return NULL;
}

int main(int argc, char const *argv[])
{
	gettimeofday(&t1,NULL);
	if(argc != 2){
		printf("Wrong number of arguments (2)\n");
		exit(1);
	}

	numMaxCli = atoi(argv[1]);

	char filename[14];
	sprintf(filename,"/tmp/bal.%d",getpid());
	file = fopen(filename, "w");

	openFIFORead();
	openFIFOWrite();	

	pthread_t tid[200000];

	process_t *process = (process_t *)malloc(sizeof(process_t));
	read(fd[0],process,sizeof(*process));
	number = process->dur;

	int i = 0;
	while(number > 0) {
		process_t *process = (process_t *)malloc(sizeof(process_t));
		read(fd[0], process, sizeof(*process));
		printf("%d - %c - %d\n", process->p, process->gender, process->dur);
		pthread_create(&tid[i], NULL, (void * ) processRequests, process);
		i++;
		pthread_mutex_lock(&nlock);
		number--;
		pthread_mutex_unlock(&nlock);
	}

	int c = 0;
	while(c < i){
		pthread_join(tid[c], NULL);
		c++;
	}

	process->p = -1;
	write(fd[1],process, sizeof(*process));

	printStatisticsInFile();

	exit(0);
}