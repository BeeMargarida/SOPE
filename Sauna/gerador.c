#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h> 
#include <time.h>

int reqNum = 0;
unsigned int seed;

/*int makeRequest(char *max){ //ERRO NO MAX...DÁ NUMEROS MARADOS POR CAUSA DO VOID POINTER EM BAIXO
	srand(time(NULL));		//AINDA NAO CONSEGUI RESOLVER
	unsigned int seed;
	int n = (rand_r(&seed) % *max) + 1;
	printf("%d\n", n);
	return n;
}*/

void * generateRequests(void * arg) {
	int num = *((int *)arg);
	seed = seed + 1;
	int n = (rand_r(&seed) % num) + 1;
	printf("%d\n", n);
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
	
	/*int fd;
	mkfifo("entrada",0660);
	fd=open("entrada",O_WRONLY);*/

	FILE *ger;
	ger = fopen("ger.0000","w+");

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