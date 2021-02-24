#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

//makefile.am

int main(){
    DIR* d = opendir(".");
    struct dirent * ent = readdir(d);
    while(ent != NULL){
        printf("%s\n", ent->d_name);
        ent = readdir(d);
    }
    closedir(d);
    exit(0);
}
