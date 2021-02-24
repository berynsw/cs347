#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fts.h>
#include <math.h>

int main(int argc, char* argv[]){
    printf("  %d\n", (!0));

    //++argv[0] ++argv


    pid_t waitpid;
    int status;

    pid_t cpid = fork();
    if(cpid == 0){
        argv++;

        printf("%s\n", argv[0]);
        printf("%s , %s\n", argv[1], argv[2]);

        execvp(argv[0], argv);
        exit(status);
    }
    else{
        waitpid = wait(&status);
        if(WIFEXITED(status)){
            printf("status return:  %d\n", (!WEXITSTATUS(status)));
        }
        else{
            printf("exec error");
            exit(EXIT_FAILURE);
        }
    }
}
