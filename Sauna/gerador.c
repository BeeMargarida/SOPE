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
		fd[1] = open("/tmp/rejeitados",O_RDONLY | O_NONBLOCK) ;
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
	pthread_mutex_lock(&lock);
	p++;
	pthread_mutex_unlock(&lock);
}

void * generateRequests(void * arg) {
	process_t process;
	//pthread_mutex_lock(&lock);
	makeRequest(&process, *((int *)arg));
	//pthread_mutex_unlock(&lock);
	printInFile(&process,0);
	write(fd[0], &process, sizeof(process));
	if(errno == EAGAIN){
		printf("PIPE FULL\n");
	}
	return NULL;
}

void * receiveAnswers(void * arg){
	process_t process = *((process_t *)arg);
	if(process.rej >= 3){
		printInFile(&process,-1);
		return NULL;
	}
	else
		write(fd[0],&process,sizeof(process));
	return NULL;
}

int main(int argc, char const *argv[])
{
	if(argc != 3){
		fprintf(stderr,"Invalid number of arguments.\n");
		return -1;
	}
	//Get beggining time of the program
	gettimeofday(&t1,NULL);
	
	//FILE TO KEEP INFORMATION
	char filename[14];
	sprintf(filename,"/tmp/ger.%d",getpid());
	file = fopen(filename, "w");

	//FIFO
	openFIFOWrite();
	openFIFORead();

	//THREADS
	int max = atoi(argv[1]);
	int count = 0;
	pthread_t tg[max], tr[max];
	int tArg[max];
	seed = time(NULL);

	//MUTEX INITIALIZATION
	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("Mutex init failed\n");
		return 1;
	}

	while(count < max){
		int dur = atoi(argv[2]);
		tArg[count] = dur;
		pthread_create(&tg[count], NULL, (void *)generateRequests, &tArg[count]);
		count++;
	}
	int index = 0;
	int n = 1;
	while(n > 0){
		process_t process;	
		n = read(fd[1], &process,sizeof(process));
		printInFile(&process,1);
		if(n > 0){
			pthread_create(&tr[index],NULL,(void *) receiveAnswers, &process);
			index++;
		}
	}
	
	count = 0;
	while(count < max){
		pthread_join(tg[count], NULL);
		count++;
	}
	int i = 0;
	while(i < index){
		pthread_join(tr[i], NULL);
		i++;
	}
	close(fd[0]);
	close(fd[1]);

	pthread_mutex_destroy(&lock);

	/*if (unlink("/tmp/entrada")<0)
		printf("Error when destroying FIFO '/tmp/entrada'\n");
	else
		printf("FIFO '/tmp/entrada' has been destroyed\n"); 

	if (unlink("/tmp/rejeitados")<0)
		printf("Error when destroying FIFO '/tmp/rejeitados'\n");
	else
		printf("FIFO '/tmp/rejeitados' has been destroyed\n"); */
	exit(0);
}