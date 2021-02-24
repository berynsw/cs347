#include "matrix.h"
#include "barrier.h"
#include "options.h"
#include <stdbool.h>
#include <math.h>
#include <time.h>

// Accuracy constant
const double epsilon = 0.001;

// Where all subtask threads sync after doing an iteration
barrier_t subtask_done_barrier;
// Where the subtask threads wait for the main thread to decide their fate
barrier_t subtask_wait_barrier;

// Subtask arguments
typedef struct subtask_arg subtask_arg_t;
struct subtask_arg {
    double (*matrix_a)[MATRIX_COLS];
    double (*matrix_b)[MATRIX_COLS];
    matrix_partition_t *subtask_bounds;
    double delta_max;
};

// Global states for communication with subtask threads

// Keeps threads from executing before all threads are created (to make errors)
//   in thread creation easier to handle.
sem_t creation_wait;
// Controls further iterations of jacobi algorithm
bool do_next_iteration;
// Controls which matrix is read from or written to. Shared since this must be
//   identical between threads.
bool read_a_write_b;

// Statistics related to the runtime performance of the jacobi_iteration algorithm
struct runtime_stats {
    unsigned iterations;
    struct timespec runtime_cpu_process;
    struct timespec runtime_real;
};

// Error defines
typedef enum jacobi_err jacobi_err;
enum jacobi_err {
    JACOBI_ERR_NONE,
    JACOBI_ERR_MALLOC,
    JACOBI_ERR_PTHREAD_CREATE,
    JACOBI_ERR_BARRIER_INIT
};

/**
 * Calcualtes difference between start and end times for a given run of jacobi
 */
void timespec_diff(struct timespec *end, struct timespec *start, \
        struct timespec *diff) {
    diff->tv_sec = end->tv_sec - start->tv_sec;
    diff->tv_nsec = end->tv_nsec - start->tv_nsec;
    if (diff->tv_nsec < 0) {
        diff->tv_sec--;
        diff->tv_nsec = 1000000000L + diff->tv_nsec;
    }
}

/**
 * Converts time to microseconds
 */
double conv_timespec_to_ms(struct timespec *tm) {
    double msec = tm->tv_sec * 1000.0;
    msec += tm->tv_nsec / 1000000.0;
    return msec;
}

/**
 * Calculcates an iteration of jacobi's within the specified bounds.
 * Returns the max delta of the iteration.
 */
double do_bounded_iteration(double (*read_matrix)[MATRIX_COLS], \
        double (*write_matrix)[MATRIX_COLS], \
        matrix_partition_t *subtask_bounds) {
    
    double prev_estimate, delta, delta_max = 0.0;

    for (unsigned row = subtask_bounds->row_start; \
            row < subtask_bounds->row_end; row++) {
        for (unsigned col = subtask_bounds->col_start; \
                col < subtask_bounds->col_end; col++){

            prev_estimate = read_matrix[row][col];
            write_matrix[row][col] = (read_matrix[row][col+1]+
                                      read_matrix[row][col-1]+
                                      read_matrix[row+1][col]+
                                      read_matrix[row-1][col]) / 4.0;

            delta = fabs(prev_estimate - write_matrix[row][col]);
            if (delta > delta_max) {
                delta_max = delta;
            }
        }
    }
    return delta_max;
}

/**
 * Does iterations of jacobi to completion over some bounds given in arg.
 * creation_wait provides a way to kill threads if one of them fails to create.
 *   Once passed, it calculates iterations of its partition until told to stop.
 * Exit condition is do_next_iteration being set false. 
 */
void* jacobi_iteration_subtask(void* arg) {
    subtask_arg_t *subtask_args = (subtask_arg_t*)arg;
    double delta_max = 0.0;

    sem_wait(&creation_wait);

    while (do_next_iteration) {
        if (read_a_write_b) {
            delta_max = do_bounded_iteration(subtask_args->matrix_a, \
                subtask_args->matrix_b, subtask_args->subtask_bounds);
        }
        else {
            delta_max = do_bounded_iteration(subtask_args->matrix_b, \
                subtask_args->matrix_a, subtask_args->subtask_bounds);
        }
        subtask_args->delta_max = delta_max;
        barrier_wait(&subtask_done_barrier, pthread_self());
        barrier_wait(&subtask_wait_barrier, pthread_self());
    }
    pthread_exit(NULL);
}



/**
 * Creates the subtask threads.
 * creation_wait exists as a safeguard against a pthread failing to 
 *   create. If one fails, do_next_iteration is set to false and creation_wait
 *   is released for all the threads that succeeded, killing them. Otherwise at
 *   the end it is released for all threads.
 * At the end, the main thread id is put into threads since it is used by the
 *   semaphore heap barrier.
 */
jacobi_err jacobi_iteration_start_subtasks(pthread_t *threads, \
        subtask_arg_t *subtask_args, unsigned subtask_num) {
    int err = 0;
    jacobi_err ret = JACOBI_ERR_NONE;
    
    sem_init(&creation_wait, 0, 0);
    unsigned t = 0;
    while (t < subtask_num && err == 0) {
        err = pthread_create(&(threads[t]), NULL, \
            jacobi_iteration_subtask, (void*)&(subtask_args[t]));
        if (err > 0) {
            do_next_iteration = false;
            for (int j = 0; j < t; j++) {
                sem_post(&creation_wait);
            }
            for (int j = 0; j < t; j++) {
                pthread_join(threads[j], NULL);
            }

            errno = err;
            ret = JACOBI_ERR_PTHREAD_CREATE;
        }
        else {
            t++;
        }
    }

    threads[t] = pthread_self();

    if (t == subtask_num) {
        for (int j = 0; j < subtask_num; j++) {
            sem_post(&creation_wait);
        }
    }
    
    return ret;
}

/**
 * Initializes subtasks, and after each iteration calculates the maximum delta
 *   and compares against epsilon to decide whether to continue running or not.
 * Once done, waits for all subtask threads to exit, and stores the number of
 *   iterations in iterations.
 */
jacobi_err jacobi_iteration_main_thread(pthread_t *threads, subtask_arg_t *subtask_args, \
        unsigned subtask_num, unsigned *iterations) {

    *iterations = 0;
    jacobi_err ret = JACOBI_ERR_NONE;

    do_next_iteration = true;
    read_a_write_b = true;

    ret = jacobi_iteration_start_subtasks(threads, subtask_args, subtask_num);
    if (ret == JACOBI_ERR_NONE) {
        while (do_next_iteration) {
            barrier_wait(&subtask_done_barrier, pthread_self());

            double delta_max = 0.0;
            for (int i = 0; i < subtask_num; i++) {
                if (subtask_args[i].delta_max > delta_max) {
                    delta_max = subtask_args[i].delta_max;
                }
            }

            do_next_iteration = (delta_max > epsilon);
            if (do_next_iteration) {
                read_a_write_b = !read_a_write_b;
            }

            (*iterations)++;
            barrier_wait(&subtask_wait_barrier, pthread_self());
        }
        for (int i = 0; i < subtask_num; i++) {
            pthread_join(threads[i], NULL);
        }
    }
    return ret;
}

/**
 * Wrapper function to record realtime and CPU time of the jacobi algorithm.
 * Runs the jacobi algorithm and stores the return data in rs.
 */
jacobi_err time_jacobi_iteration(pthread_t *threads, \
        subtask_arg_t *subtask_args, unsigned subtask_num, \
        struct runtime_stats *rs) {

    struct timespec starttime_cpu_process;
    struct timespec starttime_real;
    struct timespec endtime_cpu_process;
    struct timespec endtime_real;
    jacobi_err ret = JACOBI_ERR_NONE;
    
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &starttime_cpu_process);
    clock_gettime(CLOCK_MONOTONIC_RAW, &starttime_real);

    ret = jacobi_iteration_main_thread(threads, subtask_args, subtask_num, \
        &(rs->iterations));

    if (ret == JACOBI_ERR_NONE) {
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endtime_cpu_process);
        clock_gettime(CLOCK_MONOTONIC_RAW, &endtime_real);

        timespec_diff(&endtime_cpu_process, &starttime_cpu_process, \
            &(rs->runtime_cpu_process));
        timespec_diff(&endtime_real, &starttime_real, &(rs->runtime_real));
    }
    return ret;
}

/**
 * All the alocation for running the algorithm. Contained in a folder to keep
 *   the mess hidden from view.
 */
jacobi_err jacobi_iteration_mem_init(double (*input_matrix)[MATRIX_COLS], \
        double (**matrix_a)[MATRIX_COLS], double (**matrix_b)[MATRIX_COLS], \
        pthread_t **threads, subtask_arg_t **subtask_args, \
        matrix_partition_t **subtask_bounds, unsigned subtask_num) {

    jacobi_err ret = JACOBI_ERR_NONE;

    ret = matrix_init_value(matrix_a, input_matrix);
    if (ret == JACOBI_ERR_NONE) {
        ret = matrix_init_value(matrix_b, input_matrix);
        if (ret != JACOBI_ERR_NONE) {
            matrix_delete(matrix_a);
        }
        else {
            errno = 0;
            *threads = malloc(sizeof(pthread_t) * (subtask_num+1));
            if (*threads == NULL) {
                matrix_delete(matrix_a);
                matrix_delete(matrix_b);

                ret = JACOBI_ERR_MALLOC;
            }
            else {
                errno = 0;
                *subtask_args = malloc(sizeof(subtask_arg_t) * subtask_num);
                if (*subtask_args == NULL) {
                    matrix_delete(matrix_a);
                    matrix_delete(matrix_b);
                    free(*threads);

                    ret = JACOBI_ERR_MALLOC;
                }
                else {
                    *subtask_bounds = malloc(sizeof(matrix_partition_t) * \
                        subtask_num);
                    if (*subtask_bounds == NULL) {
                        matrix_delete(matrix_a);
                        matrix_delete(matrix_b);
                        free(*threads);
                        free(*subtask_args);

                        ret = JACOBI_ERR_MALLOC;
                    }
                }
            }
        }
    }
    return ret;
}

/**
 * Starts the algorithm. Allocates everything and handles lots of random errors.
 */
jacobi_err jacobi_iterator(double (*input_matrix)[MATRIX_COLS], \
        double (**output_matrix)[MATRIX_COLS], barrier_e barrier_id, \
        unsigned subtask_num, struct runtime_stats *rs) {

    assert(input_matrix != NULL);
    assert(output_matrix != NULL);
    assert(rs != NULL);

    double (*matrix_a)[MATRIX_COLS];
    double (*matrix_b)[MATRIX_COLS];

    pthread_t *threads;
    subtask_arg_t *subtask_args;
    matrix_partition_t *subtask_bounds;

    jacobi_err ret;

    if (barrier_init(&subtask_done_barrier, barrier_id, subtask_num + 1, &threads) < 0
     || barrier_init(&subtask_wait_barrier, barrier_id, subtask_num + 1, &threads) < 0) {
         ret = JACOBI_ERR_BARRIER_INIT;
    }
    else {
        ret = jacobi_iteration_mem_init(input_matrix, &matrix_a, &matrix_b, \
            &threads, &subtask_args, &subtask_bounds, subtask_num);
        if (ret == JACOBI_ERR_NONE) {
            matrix_partitions(subtask_bounds, subtask_num);
            for (unsigned i = 0; i < subtask_num; i++) {
                subtask_args[i].matrix_a = matrix_a;
                subtask_args[i].matrix_b = matrix_b;
                subtask_args[i].subtask_bounds = &(subtask_bounds[i]);
            }
            
            ret = time_jacobi_iteration(threads, subtask_args, subtask_num, rs);

            if (ret == JACOBI_ERR_NONE) {
                if (read_a_write_b) {
                    matrix_delete(&matrix_a);
                    *output_matrix = matrix_b;
                }
                else {
                    matrix_delete(&matrix_b);
                    *output_matrix = matrix_a;
                }
            }
            free(threads);
            free(subtask_args);
            barrier_delete(&subtask_done_barrier);
            barrier_delete(&subtask_wait_barrier);
        }
    }
    return ret;
}

/**
 * Simple error output for any mat_err
 * All of the errors have associated errno values, so perror is called 
 *   directly.
 */
void mat_perror(mat_err err, char *prog_name) {
    printf("%s: mat_err: ", prog_name);
    perror(NULL);
}

/**
 * Simple error output for any jacobi_err
 * All of the errors have associated errno values, so perror is called 
 *   directly.
 */ 
void jacobi_perror(jacobi_err err, char *prog_name) {
    printf("%s: jacobi_err: ", prog_name);
    perror(NULL);
}

/**
 * Parses options, reads input, runs the algorithm, writes output.
 */
int main(int argc, char **argv) {
    option_values_t option_values;

    double (*input_matrix)[MATRIX_COLS];
    double (*output_matrix)[MATRIX_COLS];

    struct runtime_stats rs;

    mat_err m_err = MAT_ERR_NONE;
    jacobi_err j_err = JACOBI_ERR_NONE;
    int ret = 0;

    if (get_option_values(argv, &option_values) < 0) {
        printf("Invalid arguments\n");
        printf("Usage: %s --[barrier][0-2] --[input][\"file name\"] "\
            "--[output][\"file name\"] --[subtasks][n]\n", argv[0]);
        printf("All the above arguments are required\n");
        ret = -1;
    }
    else {
        m_err = matrix_init(&input_matrix);
        if (m_err != MAT_ERR_NONE) {
            mat_perror(m_err, argv[0]);
            ret = -1;
        }
        else {
            m_err = matrix_file_in(input_matrix, option_values.input_fname);
            if (m_err != MAT_ERR_NONE) {
                mat_perror(m_err, argv[0]);
                ret = -1;
            }
            else {
                j_err = jacobi_iterator(input_matrix, &output_matrix, \
                    option_values.barrier_id, option_values.subtask_num, &rs);
                if (j_err != JACOBI_ERR_NONE) {
                    jacobi_perror(j_err, argv[0]);
                    ret = -1;
                }
                else {
                    m_err = matrix_file_out(output_matrix, \
                        option_values.output_fname);
                    if (m_err != MAT_ERR_NONE) {
                        mat_perror(m_err, argv[0]);
                        ret = -1;
                    }
                    else {
                        printf("%d,%.10e,%.10e,", rs.iterations, \
                            conv_timespec_to_ms(&(rs.runtime_real)), \
                            conv_timespec_to_ms(&(rs.runtime_cpu_process)));
                    }
                    free(input_matrix);
                    free(output_matrix);
                }
            }
        }
    }
    return ret;
}
