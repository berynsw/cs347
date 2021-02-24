#include "options.h"

// Sample output
// ./jacobi_process --barrier 0 --input data_ref/input.mtx --output output --subtasks 4
// ^ can be in any order
const char * const options[] = {"--barrier", "--input", "--output", \
    "--subtasks"};

/**
 * Parses the option string and fill option_values. There is NO DEFAULT OPTIONS.
 *   It fails if any options are duplicates or any aren't there. Otherwise
 *   option_values is filled accordingly.
 */
int get_option_values(char **argv, option_values_t *option_values) {
    bool option_found[] = {false, false, false, false};
    int ret = 0;

    unsigned arg = 1;
    bool inval = false;
    while (argv[arg] != NULL && !inval) {
        if (argv[arg+1] == NULL) {
            inval = true;
            ret = -1;
        }
        else {
            unsigned opt = 0;
            while (opt < OPT_TOTAL && \
                    strncmp(argv[arg], options[opt], strlen(argv[arg])) != 0) {
                opt++;
            }
            if (opt == OPT_TOTAL || option_found[opt]) {
                inval = true;
                ret = -1;
            }
            else if (parse_opt(opt, argv[arg+1], option_values) < 0) {
                inval = true;
                ret = -1;
            }
            else {
                option_found[opt] = true;
            }
            arg += 2;
        }
    }

    if (ret == 0) {
        int opt = 0;
        while (opt < OPT_TOTAL && option_found[opt]) {
            opt++;
        }
        if (opt < OPT_TOTAL) {
            ret = -1;
        }
    }
    return ret;
}

/**
 * Parses an option according to the value of opt. Returns -1 on error.
 */
int parse_opt(opt_e opt, char *arg, option_values_t *option_values) {
    unsigned long temp;
    int ret = 0;

    switch (opt) {
    case OPT_BARRIER:
        temp = strtoul(arg, NULL, 10);
        if (temp >= BARRIER_TOTAL) {
            ret = -1;
        }
        else {
            option_values->barrier_id = (barrier_e)temp;
        }
        break;
    case OPT_INPUT:
        option_values->input_fname = arg;
        break;
    case OPT_OUTPUT:
        option_values->output_fname = arg;
        break;
    case OPT_SUBTASKS:
        temp = strtoul(arg, NULL, 10);
        option_values->subtask_num = (unsigned)temp;
        break;
    case OPT_TOTAL:
        ret = -1;
        break;
    default:
        ret = -1;
    }
    return ret;
}
