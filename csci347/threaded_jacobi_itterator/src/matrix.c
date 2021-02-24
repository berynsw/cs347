#include "matrix.h"

/**
 * Initializes a matrix. Simple stuff.
 */
mat_err matrix_init(double (**matrix)[MATRIX_COLS]) {
    mat_err ret = MAT_ERR_NONE;

    assert(matrix != NULL);
    errno = 0;
    *matrix = malloc(sizeof(double) * MATRIX_ROWS * MATRIX_COLS);
    if (*matrix == NULL) {
        ret = MAT_ERR_MALLOC;
    }
    return ret;
}

/**
 * Initializes a matrix and sets its initial value.
 */
mat_err matrix_init_value(double (**matrix)[MATRIX_COLS], \
        double (*matrix_src)[MATRIX_COLS]) {
    mat_err ret = MAT_ERR_NONE;
    ret = matrix_init(matrix);
    if (ret == MAT_ERR_NONE) {
        memcpy(*matrix, matrix_src, sizeof(double) * MATRIX_COLS * MATRIX_ROWS);
    }
    return ret;
}

/**
 * Deletes a matrix and points it to NULL.
 */
void matrix_delete(double (**matrix)[MATRIX_COLS]) {
    free(*matrix);
    *matrix = NULL;
}

/**
 * Partitions the matrix, comparing against the test being run.
 */
void matrix_partitions(matrix_partition_t *partitions, \
        unsigned partitions_c) {
#ifdef BARRIER_TEST
    matrix_square_partitions(partitions, partitions_c);
#else
    matrix_row_partitions(partitions, partitions_c);
#endif
}

/**
 * Partitions the matrix by rows. Distributes the remainder of an uneven
 *   division between the first rows called.
 */
void matrix_row_partitions(matrix_partition_t *partitions, \
        unsigned partitions_c) {
    int col_start = 1;
    int col_end = MATRIX_COLS-1;
    
    int rows = (MATRIX_ROWS-2)/partitions_c;
    int rem = (MATRIX_ROWS-2)%partitions_c;
    assert(rows > 0);

    int row_start = 1;
    int partition_i = 0;
    while(partition_i < partitions_c){
        partitions[partition_i].col_start = col_start;
        partitions[partition_i].col_end   = col_end;
        
        partitions[partition_i].row_start = row_start;
        row_start += rows;
        if(rem > 0){
            row_start++;
            rem--;
        }
        partitions[partition_i].row_end = row_start;
        partition_i++;
    }
    assert(partition_i == partitions_c);
}

/**
 * Partitions the matrix into equal squares. The simplification that the number
 *   of squares must be a power of two is admissible since...
 *     1. This algorithm is used to test the effects of logarithmically
 *          increasing threads.
 *     2. It is very easy to find the two largest numbers that multiply together
 *          to get the number of squares.
 * The algorithm works even if the area being partitioned is not a power of 2,
 *   but to make sure we can get the largest number of partitions possible, the
 *   associated matrix has a modifiable area that is a power of 2.
 */
void matrix_square_partitions(matrix_partition_t *partitions, \
        unsigned partitions_c) {
    int pow = getpow2(partitions_c);
    assert(pow > -1);
        
    int row_part = 1U << (pow / 2);
    int col_part = 1U << ((pow / 2) + pow % 2);

    int row_div = (MATRIX_ROWS-2) / row_part;
    int row_rem = (MATRIX_ROWS-2) % row_part;
    int col_div = (MATRIX_COLS-2) / col_part;
    int col_rem = (MATRIX_COLS-2) % col_part;

    assert(row_div > 0);
    assert(col_div > 0);

    int partition_i = 0;
    int row_start, row_end, row_curr = 1;
    for (int row = 0; row < row_part; row++) {
        row_start = row_curr;
        row_curr += row_div;
        if (row_rem > 0) {
            row_curr++;
            row_rem--;
        }
        row_end = row_curr;

        int col_start, col_end, col_curr = 1;
        int col_rem_temp = col_rem;
        for (int col = 0; col < col_part; col++) {
            col_start = col_curr;
            col_curr += col_div;
            if (col_rem_temp > 0) {
                col_curr++;
                col_rem_temp--;
            }
            col_end = col_curr;
            
            partitions[partition_i].row_start = row_start;
            partitions[partition_i].row_end   = row_end;
            partitions[partition_i].col_start = col_start;
            partitions[partition_i].col_end   = col_end;
            partition_i++;
        }
    }
    assert(partition_i == partitions_c);
}

/**
 * True if n is a positive power of 2. False otherwise.
 */
bool ispow2(int n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

/**
 * Gets the positive power of 2 of n.
 * If it is not a power of 2, returns -1. Otherwise uses nifty-bit-shifty magic
 *   to find the power.
 */
int getpow2(int n) {
    if (!ispow2(n)) {
        return -1;
    }
    else {
        unsigned place = 1;
        int pow = 0;
        while (!(place & n) && place > 0) {
            place = place << 1;
            pow++;
        }
        return pow;
    }
}

/**
 * Gets a matrix from a file.
 * Finds the relative scale of the matrix size to the full matrix size to
 *   determine how many rows and columns, if any, to skip.
 * To keep the end points, it divides up the space inbetween the first and last
 *   entries in the row and column. Then the skip logic is based in the number
 *   of characters each float takes up and other horrible 
 *   hard-coded-considerations. (sorry)
 */
mat_err matrix_file_in(double (*matrix)[MATRIX_COLS], char *input_fname) {
    FILE *input;
    mat_err ret = MAT_ERR_NONE;

    unsigned row_skip     = (MATRIX_ROWS_FULL-MATRIX_ROWS) / (MATRIX_ROWS-1);
    unsigned row_skip_rem = (MATRIX_ROWS_FULL-MATRIX_ROWS) % (MATRIX_ROWS-1);
    unsigned col_skip     = (MATRIX_COLS_FULL-MATRIX_COLS) / (MATRIX_COLS-1);
    unsigned col_skip_rem = (MATRIX_COLS_FULL-MATRIX_COLS) % (MATRIX_COLS-1);

    errno = 0;
    input = fopen(input_fname, "r");
    if (input == NULL) {
        ret = MAT_ERR_FOPEN;
    }
    else {
        int row = 0;
        while (row < MATRIX_ROWS && ret == MAT_ERR_NONE) {
            int col = 0;
            unsigned col_skip_rem_temp = col_skip_rem;
            while (col < MATRIX_COLS && ret == MAT_ERR_NONE) {
                errno = 0;
                int ret = fscanf(input, "%lf", &matrix[row][col]);
                
                if (ret == 0) {
                    ret = MAT_ERR_FSCANF;
                }
                else {
                    if (col < MATRIX_COLS-1) {
                        if (col_skip_rem_temp > 0) {
                            col_skip_rem_temp--;
                            ret = skip_cols(input, col_skip+1);
                        }
                        else {
                            ret = skip_cols(input, col_skip);
                        }
                    }
                    col++;
                }
            }
            if (ret == MAT_ERR_NONE) {
                if (row < MATRIX_ROWS-1) {
                    if (row_skip_rem > 0) {
                        row_skip_rem--;
                        ret = skip_rows(input, row_skip+1);
                    }
                    else {
                        ret = skip_rows(input, row_skip);
                    }
                }
                row++;
            }
        }
        fclose(input);
    }
    return ret;
}

/**
 * Outputs a matrix to a file. Simple stuff.
 */
mat_err matrix_file_out(double (*matrix)[MATRIX_COLS], char *output_fname) {
    FILE *output;
    mat_err ret = MAT_ERR_NONE;

    errno = 0;
    output = fopen(output_fname, "w+");
    if (output == NULL) {
        ret = MAT_ERR_FOPEN;
    }
    else {
        int row = 0;
        while (row < MATRIX_ROWS && ret == MAT_ERR_NONE) {
            int col = 0;
            while (col < MATRIX_COLS && ret == MAT_ERR_NONE) {
                errno = 0;
                if (fprintf(output, "%.10lf ", matrix[row][col]) < 0) {
                    ret = MAT_ERR_FPRINTF;
                }
                col++;
            }
            if (fprintf(output, "\n") < 0) {
                ret = MAT_ERR_FPRINTF;
            }
            row++;
        }
        fclose(output);
    }
    return ret;
}

/**
 * Skips the number of rows in a matrix file.
 */
mat_err skip_rows(FILE *f, unsigned skip) {
    assert(fgetc(f) == ' ');
    assert(fgetc(f) == '\n');
    fseek(f, ROW_CHARS * skip, SEEK_CUR);
    return MAT_ERR_NONE;
}

/**
 * Skips the number of columns in a matrix file.
 */
mat_err skip_cols(FILE *f, unsigned skip) {
    assert(fgetc(f) == ' ');
    fseek(f, COL_CHARS * skip, SEEK_CUR);
    return MAT_ERR_NONE;
}
