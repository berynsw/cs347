#if !defined(_eb10477d_62d1_4c76_983b_b4a109def205)
#define _eb10477d_62d1_4c76_983b_b4a109def205

// simple print function for
// string linkedlist
void printdata(char* data);

// concise way of checking file type supplied by Aran
// modechar array is one longer than modemask array
// to handle if the filetype is unknown
// this function is also used in find.c
char filetype(struct stat filestat);

// build mode string to return to printdata_long
// has file type fix for reg file being represented
// as '-' in ls vs 'f' in find
char* format_mode(struct stat filestat);

// populates stat struct for a given file
// checks return of stat() call
struct stat getstats(char* filename);

#endif //_eb10477d_62d1_4c76_983b_b4a109def205
