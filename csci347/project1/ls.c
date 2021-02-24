#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "list.c"

/* printdata
*  simple print function used by list forall
*/
void printdata(char* data);

/* a_flag_check
*  cycles through arguments to check for -a
*/
int a_flag_check(int argc, char* argv[]);

/* populatelist
*  reads directory names into linked lists
*  checks for -a to display hidden
*/
void populatelist(list* l, int a_flag);




void printdata(char* data){
    printf("%s\n", data);
}

int a_flag_check(int argc, char* argv[]){
    for(int i = 1; i < argc; i++){
        if(strcoll(argv[i],"-a") == 0)
            return 1;
    }
    return 0;
}

void populatelist(list* l, int a_flag){
    DIR* d = opendir(".");
    struct dirent* ent = readdir(d);
    //traverse directories
    while(ent != NULL){
        //-a flag is not set and we encounter a "hidden" file that starts with '.'
        if((a_flag == 0) && (ent->d_name[0] == '.'));
        else
            add(ent->d_name, l);
        ent = readdir(d);
    }
    closedir(d);
}





int main(int argc, char* argv[]){

    DIR* d = opendir(".");
    if(d == NULL){
        perror("opendir");
        exit(1);
    }

    list list = NULL;
    populatelist(&list, a_flag_check(argc, argv));
    mergesort(&list);
    forall(list, &printdata);
    listfree(&list);
    forall(list, &printdata);
    closedir(d);

    exit(0);
}
