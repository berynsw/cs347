#ifndef __OPTIONS_H
#define __OPTIONS_H
#include "barrier.h"
#include <stdbool.h>
#include <string.h>

// Enum for each of the values
typedef enum opt opt_e;
enum opt {
    OPT_BARRIER  = 0,
    OPT_INPUT    = 1,
    OPT_OUTPUT   = 2,
    OPT_SUBTASKS = 3,
    OPT_TOTAL    = 4
};
// Corresponding strings for each option.
extern const char * const options[];

// struct containing option values
typedef struct option_values option_values_t;
struct option_values {
    barrier_e barrier_id;
    char *input_fname;
    char *output_fname;
    unsigned subtask_num;
};

int get_option_values(char **argv, option_values_t *option_values);
int parse_opt(opt_e opt, char *arg, option_values_t *option_values);

#endif /* __OPTIONS_H */