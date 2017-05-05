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

unsigned int seed;
int p = 0;
struct timeval t1, t2;
FILE *file;

struct process_t {
	int p;
	char *gender;
	int dur;
};

void printInFile(int dur, char *gender) {
	gettimeofday(&t2,NULL);
	double elapsedTime = (t2.tv_sec - t1.tv_sec)*1000.00;
	elapsedTime += (t2.tv_usec - t1.tv_usec);
	p++;
	fprintf(file, "%.02f - %d - %d: %s - %d - \n", elapsedTime, getpid(), p, gender, dur);
}

void * generateRequests(void * arg) {
	int num = *((int *)arg);
	seed = seed + 1;
	int n = (rand_r(&seed) % num) + 1;
	int g = (rand_r(&seed) % 2);
	char *gender = malloc(sizeof(char));
	if(g == 0)
		*gender = 'F';
	else
		*gender = 'M';
	//write(fd,&n,sizeof(int));
	printInFile(n ,gender);
	return NULL;
}

void * receiveAnswers(void * arg) {
	printf("lel1");
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
	int fd;
	mkfifo("/tmp/entrada",0660);
	fd = open("/tmp/entrada",O_WRONLY);


	//THREADS
	int max = atoi(argv[1]);
	int count = 0;
	pthread_t tg[max]/*, tr*/;
	int tArg[max];
	seed = time(NULL);

	while(count < max){
		tArg[count] = atoi(argv[2]);
		pthread_create(&tg[count], NULL, (void *)generateRequests, &tArg[count]);
		//pthread_create(&tr, NULL, receiveAnswers, &argv[2]);
		count++;
	}
	count = 0;
	while(count < max){
		pthread_join(tg[count], NULL);
		//pthread_join(tr, NULL);
		count++;
	}

	return 0;
}