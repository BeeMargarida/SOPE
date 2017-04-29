#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h> 
#include <time.h>

int reqNum = 0;

int makeRequest(char *max){ //ERRO NO MAX...DÁ NUMEROS MARADOS POR CAUSA DO VOID POINTER EM BAIXO
	srand(time(NULL));		//AINDA NAO CONSEGUI RESOLVER
	unsigned int seed;
	int n = (rand_r(&seed) % *max) + 1;
	printf("%d\n", n);
	return n;
}

void * generateRequests(void * arg) {
	makeRequest((char *)arg);
	//srand(time(NULL));
	//printf("%d\n", *(int *)arg);
	/*unsigned int seed;
	int n = (rand_r(&seed) % max) + 1;
	printf("%d\n", n);*/
	return NULL;
}

void * receiveAnswers(void * arg) {
	printf("lel1");
	return NULL;
}

int main(int argc, char const *argv[])
{
	if(argc != 4){
		fprintf(stderr,"Invalid number of arguments.\n");
		return -1;
	}
	
	/*int fd;
	mkfifo("entrada",0660);
	fd=open("entrada",O_WRONLY);*/

	pthread_t tg, tr;
	int count = 0;
	int max = atoi(argv[1]);

	while(count < max){
		pthread_create(&tg, NULL, (void *)generateRequests, &argv[2]);
		//pthread_create(&tr, NULL, receiveAnswers, &argv[2]);
		count++;
	}
	count = 0;
	while(count < max){
		pthread_join(tg, NULL);
		//pthread_join(tr, NULL);
		count++;
	}

	return 0;
}