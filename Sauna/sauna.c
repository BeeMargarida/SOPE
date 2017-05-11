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
pthread_t tid[200];
int count = 0;
int clientCount = 0;
static int numMaxCli;
char currGender = '\0';
pthread_mutex_t lock;

typedef struct Process{
	int p;
	char gender;
	int dur;
	int rej;
} process_t;

void printInFile(process_t *process, int state) {
	gettimeofday(&t2,NULL);
	double elapsedTime = (t2.tv_sec - t1.tv_sec)*1000.00;
	elapsedTime += (t2.tv_usec - t1.tv_usec);
	if(state == 0){
		char *msg = "RECEBIDO";
		fprintf(file, "%.02f - %d - %d: %c - %d - %s\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
	else if(state == 1){
		char *msg = "SERVIDO";
		fprintf(file, "%.02f - %d - %d: %c - %d - %s\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
	else {
		char *msg = "REJEITADO";
		fprintf(file, "%.02f - %d - %d: %c - %d - %s\n", elapsedTime, getpid(), process->p, process->gender, process->dur, msg);
	}
}

void * processRequests(void * pro){
	/*process_t *process = ((process_t *) pro);
	printf("FACK: %d - %c - %d\n", process->p, process->gender, process->dur);*/
	process_t process = *((process_t *) pro);
	printInFile(&process, 0);
	printf("FACK: %d - %c - %d\n", process.p, process.gender, process.dur);
	if(currGender == '\0'){
		//pthread_mutex_lock(&lock);
		clientCount++;
		//pthread_mutex_unlock(&lock);
		currGender = process.gender;
		printInFile(&process, 1);
		sleep(process.dur/1000);

		//pthread_mutex_lock(&lock);
		clientCount--;
		printf("A: %d\n", process.p);
		//pthread_mutex_unlock(&lock);

		if(clientCount == 0)
			currGender = '\0';
	}
	else if((currGender == process.gender) && (clientCount != numMaxCli)){
			//pthread_mutex_lock(&lock);
			clientCount++;
			//pthread_mutex_unlock(&lock);

			printInFile(&process, 1);
			sleep(process.dur/1000);

			//pthread_mutex_lock(&lock);
			clientCount--;
			printf("B: %d\n", process.p);
			//pthread_mutex_unlock(&lock);

			if(clientCount == 0)
				currGender = '\0';
	}
	else {
		//send through FIFO
		process.rej++;
		printInFile(&process, -1);
		printf("C: %d\n", process.p);
		write(fd[1],&process, sizeof(process));
	}
	printf("PESSOAS: %d\n",clientCount);
	//free(pro);
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
	char filename[9];
	sprintf(filename,"bal.%d",getpid());
	file = fopen(filename, "w");

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
	if(mkfifo("/tmp/rejeitados",0660) < 0){
		if(errno == EEXIST){
			printf("FIFO '/tmp/rejeitados' exists already.\n");
		}
		else
			printf("Impossible to create FIFO.\n");
	}
	if((fd[1] = open("/tmp/rejeitados",O_WRONLY)) == -1)
		printf("Error opening the FIFO.\n");

	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("Mutex init failed\n");
		return 1;
	}
	//READ REQUEST
	//process_t process;
	/*process_t *process = malloc(sizeof(process_t));
	while(read(fd[0], process, sizeof(*process)) > 0){
		//printf("%d - %c - %d\n", process.p, process.gender, process.dur);
		pthread_create(&tid[count],NULL,(void *) processRequests, (void *) process);
		count++;
	}*/
	process_t process[200];
	int index = 0;
	int n = 1;
	while(n > 0){
		//process_t process = malloc(sizeof(process_t));
		n = read(fd[0], &process[index], sizeof(struct Process));
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
	close(fd[0]);
	close(fd[1]);


	return 0;
}