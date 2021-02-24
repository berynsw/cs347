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
#include "list.h"
#include "text_io.h"

typedef struct func_node func_node;
typedef func_node* func_list;
struct func_node{
    func_list next;
    int (*funct)(void* arg, FTSENT* file, time_t t);
    void* arg;
};



// frees function func_list
// doesn't free funct pointers since
// they are't dynamically allocated
void func_list_free(func_list* l){
    while(*l != NULL){
        func_list next = (*l)->next;
        free((*l)->arg);
        free(*l);
        *l = next;
    }
}

// adds a function node to a list of functions
void add_func_node(func_list* l, int (*funct1)(void* arg, FTSENT* file, time_t t), void* arg){
    func_list newnode = (func_list)malloc(sizeof(func_node));
    newnode->funct = funct1;
    newnode->arg = arg;
    newnode->next = NULL;
    while(*l != NULL)
        l = &(*l)->next;
    *l = newnode;
}

// prints each file path
// is always last function added to
// function list
// note: does not use arg
int printfunc(void* arg, FTSENT* file, time_t t){
    printf("%s\n", file->fts_path);
    return 0;
}


int cmin(void* arg, FTSENT* file, time_t t){
    int diff = ((t - file->fts_statp->st_ctime) +60 ) / 60;
    return(diff == atoi(arg));
}


int ctime1(void* arg, FTSENT* file, time_t t){
    int diff = (t - file->fts_statp->st_ctime) / 86400;
    return(diff == atoi(arg));
}

int mmin(void* arg, FTSENT* file, time_t t){
    int diff = ((t - file->fts_statp->st_mtime) +60 ) / 60;
    return(diff == atoi(arg));
}
int mtime(void* arg, FTSENT* file, time_t t){
    int diff = (t - file->fts_statp->st_mtime) / 86400;
    return(diff == atoi(arg));
}


int type(void* arg, FTSENT* file, time_t t){
    char cur_file_type = filetype(*file->fts_statp);
    return(cur_file_type == *(char*)arg);
}


int cnewer(void* arg, FTSENT* file, time_t t){
    struct stat file2;
    stat(arg, &file2);
    return(file->fts_statp->st_ctime > file2.st_ctime);
}


// checks if path is a valid file
int is_file(char* filename){
    struct stat file1;
    return (!stat(filename, &file1));
}



// creates a new process to execute a specified program
// and returns the new programs return value
// converts args from a linkedlist to an array
// if the list contains the string {} it is
// replaced with the current filename
int exec(void* arg, FTSENT* file, time_t t){
    list a = (list)arg;
    int length = listlen(a);

    char* args[length+1];
    int i = 0;
    while(a != NULL){
        if(strcoll(a->data,"{}") == 0)
            args[i] = file->fts_name;
        else
            args[i] = a->data;
        a = a->next;
        i++;
    }
    args[i] = NULL;
    pid_t waitpid;
    int status;
    pid_t cpid = fork();
    if(cpid == 0){
        execvp(args[0], args);
        exit(status);
    }
    else{
        waitpid = wait(&status);
        if(WIFEXITED(status))
            return (!WEXITSTATUS(status));
        else{
            printf("exec error");
            exit(1);
        }
    }
    free(args);
    listfree(&a);
}

// parses arguments into primaries and their
// corresponding args
void checkflags(int argc, char* argv[], func_list* flaglist){
        for(int i = 0; i < argc; i+=2){
            if(strcoll(argv[i],"-cmin") == 0)
                add_func_node(flaglist, cmin, strdup(argv[i+1]));
            else if(strcoll(argv[i],"-cnewer") == 0){
                struct stat file2;
                char* argdup = strdup(argv[i+1]);
                if(stat(argdup, &file2) == 0)
                    add_func_node(flaglist, cnewer, argdup);
                else{
                    printf("stat error, invalid arg to cnewer, must provide valid filename\n");
                    exit(1);
                }
            }
            else if(strcoll(argv[i],"-ctime") == 0)
                add_func_node(flaglist, ctime1, strdup(argv[i+1]));
            else if(strcoll(argv[i],"-mmin") == 0)
                add_func_node(flaglist, mmin, strdup(argv[i+1]));
            else if(strcoll(argv[i],"-mtime") == 0)
                add_func_node(flaglist, mtime, strdup(argv[i+1]));
            else if(strcoll(argv[i],"-type") == 0)
                add_func_node(flaglist, type, strdup(argv[i+1]));
            else if(strcoll(argv[i],"-exec") == 0){
                i++;
                list args = NULL;
                while(strcoll(argv[i],";") != 0){
                    add(strdup(argv[i]), &args);
                    i++;
                }
                add_func_node(flaglist, exec, (list*)args);
                i--;
            }
            else{
                printf("invalid primary\n");
                exit(1);
            }
        }
        add_func_node(flaglist, printfunc, strdup("hullo"));
}

// used in find function
// applies every primary check to a given file
void cycle_functions(func_list flaglist, FTSENT* file, time_t time){
    while(flaglist != NULL && flaglist->funct(flaglist->arg, file, time) != 0)
        flaglist = flaglist->next;
}


// uses the second arg as the
// directory passed into the fts_open function
// then removes the first two args before
// parsing the rest with checkflags
// uses fts_read to explore all files and subfolders
void find(int argc, char* argv[]){
    struct timespec t;
    clockid_t clk;
    if(clock_gettime(clk, &t) == -1)
        perror("clock_gettime");

    char* direc_to_open[2] = {argv[1]};
    FTS* filestream1 = fts_open(direc_to_open, FTS_LOGICAL, NULL);
    FTSENT* entry = fts_read(filestream1);

    argv += 2;
    argc -= 2;
    func_list flaglist = NULL;
    checkflags(argc, argv, &flaglist);

    while(entry != NULL){
        if(entry->fts_info != FTS_DP){
            cycle_functions(flaglist, entry, t.tv_sec);
        }
        entry = fts_read(filestream1);
    }
    func_list_free(&flaglist);
}

int main(int argc, char* argv[]){
    if(argc == 1){
        printf("args, must give find a filename ");
        exit(1);
    }
    else
        find(argc, argv);
}
