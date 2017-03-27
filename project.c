#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
int end = 0;

void sigint_handler(int signo)
{
	char answer[1];
	printf("Are you sure you want to terminate(Y/N)? ");
	scanf("%s",answer);
	if(*answer == 'Y' || *answer == 'y')
		end = 1;	
}
/* FAZER FORK E DENTRO DE CADA FORK FAZER OPENDIR DO RESPECTIVO DIRETÓRIO*/
void find_function(int argc, char const *argv[]){
	if(argc == 1){ //go through all the directiories in the actual directory

	}
	else {
		DIR *d = opendir("~");
		struct dirent *dir(d); 
		printf("BOTA\n");
		
		printf("BOTA2\n");
		//readdir_r(d, dir, &dir);
		/*printf("BOTA3\n");
		while(readdir_r(d, dir, &dir)){
			printf("BOTA4\n");
			if(strcmp(dir->d_name, argv[2]) != 0){
				execlp("ls","ls",NULL);
			}
		}
		closedir(d);*/

	}

}

int main(int argc, char const *argv[])
{
	if(argc == 0){
		fprintf(stderr,"Invalid number of parameters.\n");
		exit(1);
	}
	struct sigaction action;
	action.sa_handler = sigint_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	if(sigaction(SIGINT,&action,NULL) < 0)
	{
		fprintf(stderr,"Unable to install SIGINT handler\n");
		exit(1);
	}
	while(end != 1){ //while there hasn't been an interruption
		if(strcmp(argv[1],"sfind") == 0){
			find_function(argc, argv);
		}
		else {
			fprintf(stderr,"Function not valid.\n");
			exit(1);
		}
		if(end == 1)
			break;
	}
	return 0;
}