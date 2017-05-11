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
		fd[1] = open("/tmp/rejeitados",O_RDONLY) ;
		if(fd[1] == -1) sleep(1);

	} while(fd[1] == -1);
}

void makeRequest(process_t *process, int num) {
	gettimeofday(&t2,NULL);
	double elapsedTime = (t2.tv_sec - t1.tv_sec)*1000.00;
	elapsedTime += (t2.tv_usec - t1.tv_usec);
	seed = seed + 1;
	int dur = (rand_r(&seed) % num) + 1;
	int g = (rand_r(&seed) % 2);
	char *gender = malloc(sizeof(char));
	if(g == 0)
		*gender = 'F';
	else
		*gender = 'M';

	fprintf(file, "%.02f - %d - %d: %s - %d - \n", elapsedTime, getpid(), p, gender, dur);
	process->p = p;
	process->gender = *gender;
	process->dur = dur;
	process->rej = 0;

	p++;
}

void * generateRequests(void * arg) {
	process_t process;
	//pthread_mutex_lock(&lock);
	makeRequest(&process, *((int *)arg));
	//pthread_mutex_unlock(&lock);

	write(fd[0], &process, sizeof(process));
	if(errno == EAGAIN){
		printf("PIPE FULL\n");
	}
	return NULL;
}

void * receiveAnswers(void * arg) {
	process_t process;	
	read(fd[1], &process,sizeof(process));
	if(process.rej>=3)
	{				
		return NULL;
	}
	else 
	{
		write(fd[0], &process, sizeof(process));
		
	}	
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
	char filename[9];
	sprintf(filename,"ger.%d",getpid());
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

	if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("Mutex init failed\n");
        return 1;
    }

	while(count < max){

		int dur = atoi(argv[2]);
		tArg[count] = dur;
		pthread_create(&tg[count], NULL, (void *)generateRequests, &tArg[count]);
		pthread_create(&tr[count], NULL, receiveAnswers, NULL);
		count++;
	}
	count = 0;
	while(count < max){
		pthread_join(tg[count], NULL);
		pthread_join(tr[count], NULL);
		count++;
	}
	close(fd[0]);
	close(fd[1]);

	return 0;
}