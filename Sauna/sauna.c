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
#include <semaphore.h>


int fd[2];
struct timeval t1, t2;
FILE *file;
pthread_t tid[10000];
int clientCount = 0;
static int numMaxCli;
char currGender = '\0';
pthread_mutex_t lock;
//STATISTICS VARIABLES
int pedF = 0, pedM = 0, rejF = 0, rejM = 0, serF = 0, serM = 0;

int number = 0;
sem_t *sem1;
//int rejected = 0;

//STRUCT
typedef struct Process{
	int p;
	char gender;
	int dur;
	int rej;
} process_t;

void openFIFOWrite() {
	if(mkfifo("/tmp/rejeitados",0660) < 0){
		if(errno == EEXIST){
			printf("FIFO '/tmp/rejeitados' exists already.\n");
		}
		else
			printf("Impossible to create FIFO.\n");
	}
	/*if((fd[1] = open("/tmp/rejeitados",O_WRONLY | O_NONBLOCK)) == -1)
		printf("Error opening the FIFO.\n");*/
	while((fd[1] = open("/tmp/rejeitados",O_WRONLY | O_NONBLOCK)) == -1){
		//printf("Error opening the FIFO.\n");
	}
}

void openFIFORead() {
	if(mkfifo("/tmp/entrada",0660) < 0){
		if(errno == EEXIST){
			printf("FIFO '/tmp/entrada' exists already.\n");
		}
		else
			printf("Impossible to create FIFO.\n");
	}
	if((fd[0] = open("/tmp/entrada",O_RDONLY)) == -1)
		printf("Error opening the FIFO.\n");
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

void * processRequests(void * pro){
	pthread_mutex_lock(&lock);
	process_t process = *((process_t *) pro);
	printInFile(&process, 0);
	if(process.gender == 'F')
		pedF++;
	else
		pedM++;
	pthread_mutex_unlock(&lock);

	pthread_mutex_lock(&lock);
	if((currGender == process.gender) || (currGender == '\0')){
		pthread_mutex_unlock(&lock);
		while(clientCount == numMaxCli){	
			sleep(0.1);
		}
		pthread_mutex_lock(&lock);
		clientCount++;
		currGender = process.gender;
		if(process.gender == 'F')
			serF++;
		else
			serM++;
		printInFile(&process, 1);
		pthread_mutex_unlock(&lock);
		sleep(process.dur/1000);

		pthread_mutex_lock(&lock);
		clientCount--;		
		if(clientCount == 0)
			currGender = '\0';
		pthread_mutex_unlock(&lock);
	}
	else {
		pthread_mutex_unlock(&lock);
		pthread_mutex_lock(&lock);
		process.rej++;
		if(process.rej < 3)
			number++;

		if(process.gender == 'F')
			rejF++;
		else
			rejM++;
		printInFile(&process, -1);

		write(fd[1],&process, sizeof(process));
		pthread_mutex_unlock(&lock);
	}
	printf("PESSOAS: %d\n",clientCount);
	return NULL;
}

int main(int argc, char const *argv[])
{
	gettimeofday(&t1,NULL);
	if(argc != 2){
		fprintf(stderr,"Invalid number of arguments.\n");
		return -1;
	}

	numMaxCli = atoi(argv[1]);

	//FILE TO KEEP INFORMATION
	char filename[14];
	sprintf(filename,"/tmp/bal.%d",getpid());
	file = fopen(filename, "w");

	//FIFO READ
	openFIFORead();
	//FIFO WRITE
	openFIFOWrite();

	//MUTEX INITIALIZATION
	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("Mutex init failed\n");
		return 1;
	}

	//SEMAPHORE
	/*sem1 = sem_open("/sem1",O_CREAT, 0600,0);
	if(sem1 == SEM_FAILED){
		fprintf(stderr, "Semaphore opening failed.\n");
		return -1;
	}*/

	//READ REQUEST
	int index = 0;
	int count = 0;

	process_t process[10000];
	read(fd[0], &process[0], sizeof(process_t));
	if(process[0].p == -1)
		number = process[0].dur;
	index++;
	while(number > 0){
		pthread_mutex_lock(&lock);
		number--;
		pthread_mutex_unlock(&lock);

		read(fd[0], &process[index], sizeof(process_t));
		printf("%d - %c - %d\n", process[index].p, process[index].gender, process[index].dur);

		pthread_create(&tid[index],NULL,(void *) processRequests, (void *) &process[index]);
		count++;
		index++;
		printf("N: %d\n", number);
	}
	int c = 1;
	while(c <= count){
		pthread_join(tid[c], NULL);
		c++;
	}

	//pthread_mutex_lock(&lock);
	printStatisticsInFile();
	//pthread_mutex_unlock(&lock);

	process[index].p = -1;
	process[index].gender = '\0';
	process[index].dur = -1;
	int n = -1;
	do {
		printf("IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII\n");
		n = write(fd[1], &process[index], sizeof(process_t));
	} while(n <= 0 && errno == EAGAIN);
	//sem_post(sem1);
	//sleep(2);

	//sem_wait(sem1);
	printf("LEL\n");
	/*close(fd[0]);
	close(fd[1]);*/
	
	//sem_close(sem1);
	pthread_mutex_destroy(&lock);
	printf("LEKEKEJABNDIONBF\n");
	exit(0);
}