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


int fd;
int tid[200];
int count = 0;
int numMaxCli = 0;

typedef struct Process{
	int p;
	char gender;
	int dur;
} process_t;

void * processRequests(void *process){
	write(STDOUT_FILENO,"FACK\n", 5);
	return NULL;
}

int main(int argc, char const *argv[])
{
	if(argc != 2){
		fprintf(stderr,"Invalid number of arguments.\n");
		return -1;
	}

	numMaxCli = atoi(argv[1]);

	if((fd = open("/tmp/entrada",O_RDONLY)) == -1)
		printf("Error opening the FIFO.\n");

	int n = 0;
	do {
		process_t process;

		n = read(fd, &process, sizeof(process));

		pthread_create(&td,NULL,(void *) processRequests, process);
		tid[count] = td;
		count++;

		printf("%d - %c - %d\n", process.p, process.gender, process.dur);
	} while(errno != EAGAIN && n > 0);
	

	/*int c = 0;
	while(c < count){
		pthread_join(tid[c],NULL);
	}*/


	return 0;
}