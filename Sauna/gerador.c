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
int p = 0;
struct timeval t1, t2;
FILE *file;
int fd;

typedef struct Process{
	int p;
	char gender;
	int dur;
} process_t;

void openFIFO() {
	if(mkfifo("/tmp/entrada",0660) < 0){
		if(errno == EEXIST){
			printf("FIFO '/tmp/entrada' exists already.\n");
		}
		else
			printf("Impossible to create FIFO.\n");
	}
	do {

		fd = open("/tmp/entrada",O_WRONLY/* | O_NONBLOCK*/) ;
		if(fd == -1) sleep(1);

	} while(fd == -1);
}

void printInFile(int dur, char *gender) {
	gettimeofday(&t2,NULL);
	double elapsedTime = (t2.tv_sec - t1.tv_sec)*1000.00;
	elapsedTime += (t2.tv_usec - t1.tv_usec);
	p++;
	fprintf(file, "%.02f - %d - %d: %s - %d - \n", elapsedTime, getpid(), p, gender, dur);

	/*process_t *process;
	process = (process_t *) malloc(sizeof(struct Process));
	process->p = p;
	process->gender = *gender;
	process->dur = dur;*/
	/*write(fd,&(process.p),sizeof(int));
	write(fd,process.gender,sizeof(char));
	write(fd,&(process.dur),sizeof(int));*/

	write(fd, &p, sizeof(int));
	write(fd, gender, sizeof(char *));
	write(fd, &dur, sizeof(int));
	
	//printf("HERE!\n");
	//openFIFO();
	//write(fd, process, sizeof(*process));
	if(errno == EAGAIN){
		printf("PIPE FULL\n");
	}
	//close(fd);
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
	/*if(mkfifo("/tmp/entrada",0660) < 0){
		if(errno == EEXIST){
			printf("FIFO '/tmp/entrada' exists already.\n");
		}
		else
			printf("Impossible to create FIFO.\n");
	}
	do {
		fd = open("/tmp/entrada",O_WRONLY) ;
		if(fd == -1) sleep(1);

	} while(fd == -1);*/
	/*if((fd = open("/tmp/entrada",O_WRONLY)) == -1){
	printf("Failed to open FIFO.\n");
	return -1;*/


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
	close(fd);
	count = 0;
	while(count < max){
		pthread_join(tg[count], NULL);
		//pthread_join(tr, NULL);
		count++;
	}

	return 0;
}