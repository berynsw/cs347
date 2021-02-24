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
#include "text_io.h"



void printdata(char* data){
    printf("%s\n", data);
}



char filetype(struct stat filestat){
    static const int modemask[] = {S_IFBLK,S_IFCHR,S_IFDIR,
                                   S_IFLNK,S_IFIFO,S_IFREG,
                                   S_IFSOCK};
    static const char modechar[] = {'b', 'c', 'd',
                                    'l', 'p', 'f',
                                    's', '?'};
    static const int maskcount = sizeof(modemask)/sizeof(modemask[0]);
    mode_t filemode = filestat.st_mode;
    int i = 0;
    while(i < maskcount && modemask[i] != (filemode & S_IFMT))
        i++;
    return modechar[i];
}



char* format_mode(struct stat filestat){
    char* mode = (char*)malloc(sizeof(char) * 11);
    if(filetype(filestat) == 'f')
        mode[0] = '-';
    else
        mode[0] = filetype(filestat);
    static const int permmask[] = {S_IRUSR,S_IWUSR,S_IXUSR,
                                   S_IRGRP,S_IWGRP,S_IXGRP,
                                   S_IROTH,S_IWOTH,S_IXOTH};
    static const char permchar[] = {'r', 'w', 'x',
                                    'r', 'w', 'x',
                                    'r', 'w', 'x'};
    static const int permcount = sizeof(permchar)/sizeof(permchar[0]);
    mode_t filemode = filestat.st_mode;
    for(int i = 0; i < permcount; i++){
        if(filemode & permmask[i])
            mode[i+1] = permchar[i];
        else
            mode[i+1] = '-';
    }
    mode[10] = '\0';
    return mode;
}






struct stat getstats(char* filename){
    struct stat file1;
    if(stat(filename, &file1) != 0)
        perror("stat error\n");
    return file1;
}
