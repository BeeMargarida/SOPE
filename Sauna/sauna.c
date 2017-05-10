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
pthread_t tid[200];
int count = 0;
int numMaxCli;
char *currGender;

typedef struct Process{
	int p;
	char gender;
	int dur;
	int rej;
} process_t;

void * processRequests(void * pro){
	process_t process = *((process_t *) pro);
	
	if(currGender == NULL){
		numMaxCli--;
		*currGender = process.gender;
		//SEND through fifo as accepted
		printf("A: %d - %c - %d\n", process.p, process.gender, process.dur);
		sleep(process.dur);
		numMaxCli++;
		if(numMaxCli == 0)
			currGender = NULL;
	}
	else if(*currGender == process.gender){
		if(numMaxCli != 0){
			numMaxCli--;
		//SEND through fifo as accepted
			printf("B: %d - %c - %d\n", process.p, process.gender, process.dur);
			sleep(process.dur);
			numMaxCli++;
			if(numMaxCli == 0)
				currGender = NULL;
		}
	}
	else {
		//send through FIFO
		process.rej++;
		//write(fd[1],&pro, sizeof(pro));
	}
	return NULL;
}

int main(int argc, char const *argv[])
{
	if(argc != 2){
		fprintf(stderr,"Invalid number of arguments.\n");
		return -1;
	}

	numMaxCli = atoi(argv[1]);

	//FIFO READ
	if(mkfifo("/tmp/entrada",0660) < 0){
		if(errno == EEXIST){
			printf("FIFO '/tmp/entrada' exists already.\n");
		}
		else
			printf("Impossible to create FIFO.\n");
	}
	if((fd[0] = open("/tmp/entrada",O_RDONLY)) == -1)
		printf("Error opening the FIFO.\n");

	//FIFO WRITE
	/*if(mkfifo("/tmp/rejeitados",0660) < 0){
		if(errno == EEXIST){
			printf("FIFO '/tmp/rejeitados' exists already.\n");
		}
		else
			printf("Impossible to create FIFO.\n");
	}
	if((fd[1] = open("/tmp/rejeitados",O_WRONLY)) == -1)
		printf("Error opening the FIFO.\n");*/

	//READ REQUEST
	int n = 1;
	process_t process;
	while(read(fd[0], &process, sizeof(process)) > 0){
		printf("%d - %c - %d\n", process.p, process.gender, process.dur);
		pthread_create(&tid[count],NULL,(void *) processRequests, &process);
		count++;
	}


	int c = 0;
	while(c < count){
		pthread_join(tid[count], NULL);
		c++;
	}
	close(fd[0]);
	//close(fd[1]);


	return 0;
}