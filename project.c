#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
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

int getPerm(const char *path){
    struct stat ret;
   
    if (stat(path, &ret) == -1) {
        write(STDOUT_FILENO,"Can't Open\n", 11);
        return -1;
    }
   
    return (ret.st_mode & S_IRUSR)|(ret.st_mode & S_IWUSR)|(ret.st_mode & S_IXUSR)|
    (ret.st_mode & S_IRGRP)|(ret.st_mode & S_IWGRP)|(ret.st_mode & S_IXGRP)|
    (ret.st_mode & S_IROTH)|(ret.st_mode & S_IWOTH)|(ret.st_mode & S_IXOTH);
}

void find_function(char const argv[]){
    DIR *d;
    struct dirent *dir;
    pid_t pid;
    d = opendir(argv);
    struct stat buf;
    char name[200];
    if(d == NULL){
        exit(1);
    }
    while((dir = readdir(d)) != NULL){
        if(strcmp(dir->d_name,".") == 0 || strcmp(dir->d_name,"..") == 0){
            continue;
        }
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


int remove_dir(char const path[]){
    DIR *d;
    struct dirent *dir;
    pid_t pid;
    char *name = (char *)malloc(200 * sizeof(char));
    d = opendir(path);  
    if(d == NULL){
        fprintf(stderr,"Open Dir error.\n");
        exit(1);
    }
    struct stat buf;
    while((dir = readdir(d)) != NULL){

        sprintf(name, "%s/%s", path, dir->d_name);
        if(lstat(name, &buf) == -1){
            perror("Error in lstat");
            return -1;
        }
        if(strcmp(dir->d_name,".") == 0 || strcmp(dir->d_name,"..") == 0 || (dir->d_name[0] == '.')){
            continue;
        }
        if(S_ISDIR(buf.st_mode)){
            if((pid = fork()) < 0){
                fprintf(stderr, "Fork error.\n");
                exit(2);
            }
            else if(pid > 0) { //pai
                waitpid(pid, 0, 0);
                remove(name);
            }
            else if(pid == 0){ //filho
                if(remove_dir(name) == 1)
                    return 1;
            }
        }
        else if(S_ISREG(buf.st_mode)){
            printf("ISREF: %s\n",name);
            remove(name);
            return 1;
        }
    }
    end = 1;
    if (dir == NULL) {
        exit(0);
    }
    return 1;
}

int function_exec(char const argv[], char const filename[], char const cmd[]){
    DIR *d;
    struct dirent *dir;
    pid_t pid;
    char *name = (char *)malloc(200 * sizeof(char));
    d = opendir(argv);  
    if(d == NULL){
        fprintf(stderr,"Open Dir error.\n");
        exit(1);
    }

    struct stat buf;
    while((dir = readdir(d)) != NULL){

        sprintf(name, "%s/%s", argv, dir->d_name);
        if(lstat(name, &buf) == -1){
            perror("Error in lstat");
            return -1;
        }

        if(strcmp(dir->d_name,".") == 0 || strcmp(dir->d_name,"..") == 0 || (dir->d_name[0] == '.')){
            continue;
        }
        if(S_ISDIR(buf.st_mode)){
            if((pid = fork()) < 0){
                fprintf(stderr, "Fork error.\n");
                exit(2);
            }
            else if(pid > 0) { //pai
                waitpid(pid, 0, 0);
            }
            else if(pid == 0){ //filho
                function_exec(name,filename,cmd);
                return 1;
            }
        }
        else if(S_ISREG(buf.st_mode)){
            if(strcmp(dir->d_name,filename) == 0){
                execlp(cmd,cmd,dir->d_name,NULL);
            }
        }
    }
    end = 1;
    if (dir == NULL) {
        exit(0);
    }
    return 1;
}

int find_function_type(char const argv[], char const c[]) {
    DIR *d;
    struct dirent *dir;
    pid_t pid;
    char name[200];
    d = opendir(argv);  
    if(d == NULL){
        fprintf(stderr,"Open Dir error.\n");
        exit(1);
    }

    struct stat buf;
    while((dir = readdir(d)) != NULL){

        sprintf(name, "%s/%s", argv, dir->d_name);
        if(lstat(name, &buf) == -1){
            perror("Error in lstat");
            return -1;
        }

        if(strcmp(dir->d_name,".") == 0 || strcmp(dir->d_name,"..") == 0 || (dir->d_name[0] == '.')){
            continue;
        }
        if(S_ISDIR(buf.st_mode)){
            if((strcmp(c,"d") == 0))
                printf("%s\n",name);
            if((pid = fork()) < 0){
                fprintf(stderr, "Fork error.\n");
                exit(2);
            }
            else if(pid > 0) { //pai
                waitpid(pid, 0, 0);
            }
            else if(pid == 0){ //filho
                if(find_function_type(name,c) == 1)
                    return 1;
                exit(0);
            }
        }
        else if(S_ISREG(buf.st_mode)){
            if((strcmp(c,"f") == 0))
                printf("%s\n",name);
            return 1;
        }
        else if(S_ISLNK(buf.st_mode)){
            if((strcmp(c,"l") == 0))
                printf("%s\n",name);
            return 1;
        }
    }
    end = 1;
    if (dir == NULL) {
        exit(0);
    }
    return 1;
}

int find_function_name(char const argv[], char const filename[], char const func[]) {
    DIR *d;
    struct dirent *dir;
    pid_t pid;
    char *name = (char *)malloc(200 * sizeof(char));
    d = opendir(argv);  
    if(d == NULL){
        fprintf(stderr,"Open Dir error.\n");
        exit(1);
    }

    struct stat buf;
    while((dir = readdir(d)) != NULL){

        sprintf(name, "%s/%s", argv, dir->d_name);
        if(lstat(name, &buf) == -1){
            perror("Error in lstat");
            return -1;
        }

        if(strcmp(dir->d_name,".") == 0 || strcmp(dir->d_name,"..") == 0 || (dir->d_name[0] == '.')){
            continue;
        }
        if(S_ISDIR(buf.st_mode)){
            if(strcmp(dir->d_name,filename) == 0){
                if(strcmp(func,"-print") == 0){    
                    printf("%s\n",name);
                    return 1;
                }
                else if(strcmp(func,"-delete") == 0){
                    remove_dir(name);
                    remove(name);
                    return 1;
                }
            }
            if((pid = fork()) < 0){
                fprintf(stderr, "Fork error.\n");
                exit(2);
            }
            else if(pid > 0) { //pai
                waitpid(pid, 0, 0);
            }
            else if(pid == 0){ //filho
                if(find_function_name(name,filename,func) == 1)
                    return 1;
                exit(0);
            }
        }
        else if(S_ISREG(buf.st_mode)){
            if(strcmp(dir->d_name,filename) == 0){
                if(strcmp(func,"-print") == 0){    
                    printf("%s\n",name);
                    return 1;
                }
                else if(strcmp(func,"-delete") == 0){
                    remove(name);
                    return 1;
                }
                /*else if(strcmp(func,"-perm")==0){
                    char *path=(char *)malloc(strlen(name));
                    strcpy(path, name);
                    printf("0%o\n", getPerm(path));
                    return 1;
                }*/
            }
        }
    }
    end = 1;
    if (dir == NULL) {
        exit(0);
    }
    return 1;
}

int main(int argc, char const *argv[])
{
    if(argc == 0){
        fprintf(stderr,"Invalid number of parameters.\n");
        return -1;
    }
    struct sigaction action;
    action.sa_handler = sigint_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if(sigaction(SIGINT,&action,NULL) < 0)
    {
        fprintf(stderr,"Unable to install SIGINT handler\n");
        return -1;
    }

    //              NÃO É BEM ASSIM, VAI TER DE SER UM CICLO...EU DEPOIS TRATO DISTO 
    while(end != 1){ 
        if(strcmp(argv[1],"sfind") == 0){
            if(argc == 3){
                find_function(argv[2]);
            }
            else {
                if((strcmp(argv[3],"-name") == 0) && (argc != 7)){
                    find_function_name(argv[2],argv[4],argv[5]);
                    end = 1;
                }
                else if(strcmp(argv[3],"-type") == 0){
                    find_function_type(argv[2],argv[4]);
                    end = 1;
                }
                else if(strcmp(argv[3],"-perm") == 0){
                    find_function_name(argv[2],argv[4],argv[5]);
                    end = 1;
                }
                else if (strcmp(argv[5],"-exec") == 0){
                    if(argc != 7){
                        fprintf(stderr,"Few arguments for exec.\n");
                    }
                    else
                        function_exec(argv[2],argv[4],argv[6]);
                    end = 1;
                }
                else {
                    fprintf(stderr,"Function not valid.\n");
                    return -1;
                }
            }
        }
        else {
            fprintf(stderr,"Function not valid.\n");
            return -1;
        }
        if(end == 1)
            break;
    }
    return 0;
}
