#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
int end = 0;

void sigint_handler(int signo)
{
    char answer[1];
    printf("Are you sure you want to terminate(Y/N)? ");
    scanf("%s",answer);
    if(*answer == 'Y' || *answer == 'y')
        end = 1;
}
void find_function(char const argv[]){
    DIR *d;
    struct dirent *dir;
    pid_t pid;
    d = opendir(argv);
    struct stat buf;
    if(d == NULL){
        exit(1);
    }
    while((dir = readdir(d)) != NULL){
        if(strcmp(dir->d_name,".") == 0 || strcmp(dir->d_name,"..") == 0){
            continue;
        }
        char name[200];
        sprintf(name, "%s/%s", argv, dir->d_name);
        if(lstat(name, &buf) == -1){
            fprintf(stderr, "lstat error.\n");
            exit(1);
        }
        if(S_ISREG(buf.st_mode)){
            write(STDOUT_FILENO,dir->d_name,strlen(dir->d_name));
            write(STDOUT_FILENO,"\n",1);
        }
        else if(S_ISDIR(buf.st_mode)){
            if((pid = fork()) < 0){
                fprintf(stderr, "Fork error.\n");
                exit(2);
            }
            else if(pid > 0) { //pai
                write(STDOUT_FILENO,dir->d_name,strlen(dir->d_name));
                write(STDOUT_FILENO,"\n",1);
                waitpid(pid, 0, 0);
            }
            else if(pid == 0){ //filho
                find_function(dir->d_name);
            }
        }
    }
    end = 1;
    if (dir == NULL) {
        exit(0);
    }
}

void find_function_name(char const argv[], char *str) {
    DIR *d;
    struct dirent *dir;
    pid_t pid;
    d = opendir(argv);
    struct stat buf;
    if(d == NULL){
        exit(1);
    }
    while((dir = readdir(d)) != NULL){
        if(strcmp(dir->d_name,".") == 0 || strcmp(dir->d_name,"..") == 0){
            continue;
        }
        char name[200];
        sprintf(name, "%s/%s", argv, dir->d_name);
        if(lstat(name, &buf) == -1){
            fprintf(stderr, "lstat error.\n");
            exit(1);
        }
        if(S_ISREG(buf.st_mode)){
            strcat(str,"/");
            strcat(str,dir->d_name);
            printf("%s\n",str);
        }
        else if(S_ISDIR(buf.st_mode)){
            if((pid = fork()) < 0){
                fprintf(stderr, "Fork error.\n");
                exit(2);
            }
            else if(pid > 0) { //pai
                strcat(str,"/");
                strcat(str,dir->d_name);
                //printf("%s\n",str);
                waitpid(pid, 0, 0);
            }
            else if(pid == 0){ //filho
                find_function_name(dir->d_name,str);
            }
        }
    }
    end = 1;
    if (dir == NULL) {
        exit(0);
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
    
    //              NÃO É BEM ASSIM, VAI TER DE SER UM CICLO...EU DEPOIS TRATO DISTO 
    while(end != 1){ 
        if(strcmp(argv[1],"sfind") == 0){
            if(argc == 3){
                find_function(argv[2]);
            }
            else { //fazer os -name, -type.....
                if(strcmp(argv[3],"-name") == 0){
                    char *str = (char *)malloc(200*sizeof(char));
                    //printf("fazer -name");
                    find_function_name(argv[2],str);
                }
                else if(strcmp(argv[3],"-type") == 0){
                    printf("fazer -type");
                }
                else if(strcmp(argv[3],"-perm") == 0){
                    printf("fazer -perm");
                }
                else if(strcmp(argv[3],"-print") == 0){
                    printf("fazer -print");
                }
                else if(strcmp(argv[3],"-delete") == 0){
                    printf("fazer -delete");
                }
                else if(strcmp(argv[3],"-exec") == 0){
                    printf("fazer -exec");
                }
                else {
                    fprintf(stderr,"Function not valid.\n");
                    exit(1);
                }
            }
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
