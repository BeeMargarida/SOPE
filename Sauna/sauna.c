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
FILE *file;
pthread_t tid[2000];
int count = 0;
int clientCount = 0;
static int numMaxCli;
char currGender = '\0';
pthread_mutex_t lock;

int pedF = 0, pedM = 0, rejF = 0, rejM = 0, serF = 0, serM = 0;

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
	if((fd[1] = open("/tmp/rejeitados",O_WRONLY)) == -1)
		printf("Error opening the FIFO.\n");
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
		//char m[200];
		//sprintf(m, "%.02f\t-\t%d\t-\t%d\t:\t%c\t-\t%d\t-\t%s\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
		//fprintf(file, "%-25s", m);
		fprintf(file, "\t%.02f\t-\t%d\t-\t%d\t:\t%c\t-\t%d\t-\t%s\t\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
	else if(state == 1){
		char *msg = "SERVIDO";
		fprintf(file, "\t%.02f\t-\t%d\t-\t%d\t:\t%c\t-\t%d\t-\t%s\t\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
	else {
		char *msg = "REJEITADO";
		fprintf(file, "\t%.02f\t-\t%d\t-\t%d\t:\t%c\t-\t%d\t-\t%s\t\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
}

void printStatisticsInFile() {
	fprintf(file, "\nSTATISTICS\n");
	fprintf(file, "Requests:\nTotal: %d\t F: %d\t M: %d\n", (pedM+pedF),pedF,pedM);
	fprintf(file, "Rejected:\nTotal: %d\t F: %d\t M: %d\n", (rejM+rejF),rejF,rejM);
	fprintf(file, "Served:\nTotal: %d\t F: %d\t M: %d\n", (serM+serF),serF,serM);
}

void * processRequests(void * pro){
	process_t process = *((process_t *) pro);
	printInFile(&process, 0);
	pthread_mutex_lock(&lock);
	if(process.gender == 'F')
			pedF++;
		else
			pedM++;
		pthread_mutex_unlock(&lock);
	if(currGender == '\0'){
		pthread_mutex_lock(&lock);
		clientCount++;
		currGender = process.gender;
		if(process.gender == 'F')
			serF++;
		else
			serM++;
		pthread_mutex_unlock(&lock);
		printInFile(&process, 1);
		sleep(process.dur/1000);

		pthread_mutex_lock(&lock);
		clientCount--;
		//printf("A: %d\n", process.p);
		
		if(clientCount == 0)
			currGender = '\0';
		pthread_mutex_unlock(&lock);
	}
	else if((currGender == process.gender) && (clientCount != numMaxCli)){
		pthread_mutex_lock(&lock);
		clientCount++;
		if(process.gender == 'F')
			serF++;
		else
			serM++;
		pthread_mutex_unlock(&lock);

		printInFile(&process, 1);
		sleep(process.dur/1000);

		pthread_mutex_lock(&lock);
		clientCount--;			
		if(clientCount == 0)
			currGender = '\0';
		pthread_mutex_unlock(&lock);
	}
	else {
		//send through FIFO
		process.rej++;
		pthread_mutex_lock(&lock);
		if(process.gender == 'F')
			rejF++;
		else
			rejM++;
		pthread_mutex_unlock(&lock);
		printInFile(&process, -1);
		//printf("C: %d\n", process.p);
		//openFIFOWrite();
		write(fd[1],&process, sizeof(process));
		//close(fd[1]);
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

	//process_t process;
	/*process_t *process = malloc(sizeof(process_t));
	while(read(fd[0], process, sizeof(*process)) > 0){
		//printf("%d - %c - %d\n", process.p, process.gender, process.dur);
		pthread_create(&tid[count],NULL,(void *) processRequests, (void *) process);
		count++;
	}*/

	//READ REQUEST
	process_t process[2000];
	int index = 0;
	int n = 1;
	while(n > 0){
		//process_t process = malloc(sizeof(process_t));
		n = read(fd[0], &process[index], sizeof(process_t));
		if(n > 0){
			pthread_create(&tid[index],NULL,(void *) processRequests, (void *) &process[index]);
			count++;
			index++;
		}
	}

	int c = 0;
	while(c < count){
		pthread_join(tid[count], NULL);
		c++;
	}

	printStatisticsInFile();
	pthread_mutex_destroy(&lock);

	sleep(4);
	close(fd[0]);
	close(fd[1]);

	/*int t = fork();
	if(t > 0){ //pai
		close(fd[0]);
	} 
	else{
		close(fd[1]);
	}*/

	if (unlink("/tmp/rejeitados")<0)
		printf("Error when destroying FIFO '/tmp/rejeitados'\n");
	else
		printf("FIFO '/tmp/rejeitados' has been destroyed\n"); 

	if (unlink("/tmp/entrada")<0)
		printf("Error when destroying FIFO '/tmp/entrada'\n");
	else
		printf("FIFO '/tmp/entrada' has been destroyed\n"); 
	
	exit(0);
}