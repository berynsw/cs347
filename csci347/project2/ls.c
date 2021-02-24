#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "list.h"
#include "text_io.h"


#define OPTSTRING "adil"
int show_hidden;
int show_file_stats;
int show_inode;
int recurse_dirs = 1;
int winsize;
list dir_list = NULL;
list file_list = NULL;

// checks if path is a valid file
int is_file(char* filename){
    struct stat file1;
    return (!stat(filename, &file1));
}

// uses filetype to check if file is
// of type directory
int is_dir(char* filename){
    struct stat file1 = getstats(filename);
    return(filetype(file1) == 'd');
}

// checks if a directory entry starts with a dot
int is_hidden(struct dirent* ent){
    return(ent->d_name[0] == '.');
}

// adds operands to corresponding lists
void parse_operands(int argc, char* argv[]){
    while(optind < argc){
        if(is_file(argv[optind])){
            if(is_dir(argv[optind]) && recurse_dirs){
                add(argv[optind], &dir_list);
            }
            else{
                add(argv[optind], &file_list);
            }
        }
        else{
            printf("cannot access '%s': no such file or directory\n", argv[optind]);
        }
        optind++;
    }
}

// handles flags for ls
void parse_args(int argc, char* argv[]){
    int getopt_check = getopt(argc, argv, OPTSTRING);
    while(getopt_check != -1){
        if(getopt_check == 'a')
            show_hidden = 1;
        else if(getopt_check == 'l')
            show_file_stats = 1;
        else if(getopt_check == 'i')
            show_inode = 1;
        else if(getopt_check == 'd')
            recurse_dirs = 0;
        else{
            printf("invalid arg, only supports: a, d, i, l\n");
        }
        getopt_check = getopt(argc, argv, OPTSTRING);
    }
}

// sets winsize as the number of columns in terminal
// if stdout isn't a terminal sets winsize to -1
void get_winsize(){
    if(isatty(STDOUT_FILENO)){
        struct winsize argp;
        int x = ioctl(STDOUT_FILENO, TIOCGWINSZ, &argp);
        if(x == -1){
            printf("ioctl error\n");
            exit(1);
        }
        else
            winsize = argp.ws_col;
    }
    else
        winsize = -1;
}

// gets string length of inode number
int numlen(char* filename){
    struct stat file1 = getstats(filename);
    long num = (long)file1.st_ino;
    int len = 0;
    while(num != 0){
        len++;
        num /= 10;
    }
    return len;
}

// concatenates a path for stat
char* concat(char* dir, char* name){
    int size = strlen(dir) + strlen(name) + 3;
    char* path = malloc(sizeof(char)*size);
    strcpy(path, dir);
    strcat(path, "/");
    strcat(path, name);
    return path;
}

int colnum(char* dir, list l, int max){
    int ret;
    if(show_inode){
        char* path = concat(dir, l->data);
        int inodesz = numlen(path)+1;
        ret = winsize/(max+inodesz);
        free(path);
    }
    else
        ret = winsize/(max);
    return ret;
}

// prints given list of files in column format
// decides column width based on winsize,
// longest file name + length of
// inode# (if -i option was given)
// if winsize is -1 just prints on newlines
void print_cols(char* dir, list l, int max){
    int cols;
    if(winsize == -1)
        cols = 1;
    else{
        max += 2;
        cols = colnum(dir, l, max);
    }
    int i = 1;
    while(l != NULL){
        if(show_inode){
            char* path = concat(dir, l->data);
            struct stat file1 = getstats(path);
            printf("%ld ", (long)file1.st_ino);
            printf("%-*s", max, l->data);
        }
        else
            printf("%-*s", max, l->data);
        if(i%cols == 0)
            printf("\n");
        l = l->next;
        i++;
    }
    printf("\n");
}

// prints stats about a file
// uses helper functions format_mode, filetype and getstats
// optionally prints inode before rest of stats
void print_stats(char* dir, list l){
    while(l != NULL){
        char* path = concat(dir, l->data);
        struct stat file1 = getstats(path);
        if(show_inode){
            printf("%ld ", (long)file1.st_ino);
        }
        struct passwd* passwordptr;
        passwordptr = getpwuid(file1.st_uid);
        if(passwordptr == NULL){
            printf("getpwuid() error\n");
            exit(1);
        }
        struct group* groupptr;
        groupptr = getgrgid(file1.st_gid);
        if(groupptr == NULL){
            printf("getgrgid() error\n");
            exit(1);
        }
        char datestr[36];
        strftime(datestr, 36, "%b %d %H:%M", localtime(&file1.st_mtime));
        printf("%s %ld %s %s %ld %s ",
                                        format_mode(file1),
                                        (long)file1.st_nlink,
                                        passwordptr->pw_name,
                                        groupptr->gr_name,
                                        (long)file1.st_size,
                                        datestr);
        printf("%s\n", l->data);
        free(path);
        l = l->next;
    }
}

// calls appropriate print func based on flags
void print_format(char* dir, list l, int max){
    if(l != NULL){
        if(show_file_stats)
            print_stats(dir, l);
        else
            print_cols(dir, l, max);
    }
}

// traverses files in the given directory
// if directory given is not the current one,
// uses chdir to move into requested directory and
// move back out after printing files within
// prints files in columns unless -l option is given
void dir_print(char* dir_name){
    list l = NULL;
    DIR* d = opendir(dir_name);
    if(d == NULL){
        printf("opendir error");
        exit(1);
    }
    struct dirent* ent = readdir(d);
    int max = 0;
    while(ent != NULL){
        if(show_hidden || !is_hidden(ent)){
            add(ent->d_name, &l);
            if(strlen(ent->d_name) > max)
                max = strlen(ent->d_name);
        }
        ent = readdir(d);
    }
    merge_sort_list(&l);
    print_format(dir_name, l, max);
    closedir(d);
    listfree(&l);
}

// displays the files in each dir in dir_list
void cycle_dir_list(list l){
    while(l != NULL){
        printf("%s:\n", l->data);
        dir_print(l->data);
        l = l->next;
        printf("\n");
    }
}

int main(int argc, char* argv[]){
    get_winsize();
    parse_args(argc, argv);
    if(optind >= argc){
        if(recurse_dirs)
            dir_print(".");
        else{
            add(".", &file_list);
            print_format(".", file_list, 1);
        }
    }
    else{
        parse_operands(argc, argv);

        int max = find_max(file_list);
        merge_sort_list(&file_list);
        print_format(".", file_list, max);
        printf("\n");

        merge_sort_list(&dir_list);
        cycle_dir_list(dir_list);
    }

    listfree(&dir_list);
    listfree(&file_list);
    exit(0);
}
