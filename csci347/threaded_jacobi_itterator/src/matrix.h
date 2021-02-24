#ifndef __MATRIX_H
#define __MATRIX_H
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

// Full matrix size
#define MATRIX_ROWS_FULL 1024
#define MATRIX_COLS_FULL 1024

// Size of the actual matrix
// The barrier test has a smaller matrix size, otherwise the full barrier is
//   run against.
#ifdef BARRIER_TEST
#define MATRIX_ROWS 66
#define MATRIX_COLS 66
#else
#define MATRIX_ROWS 1024
#define MATRIX_COLS 1024
#endif

// Defines for variable-size matrix input
#define COL_CHARS 13
#define ROW_CHARS ((COL_CHARS * MATRIX_COLS_FULL) + 1)

// Error defines
typedef enum mat_err mat_err;
enum mat_err {
    MAT_ERR_NONE,
    MAT_ERR_MALLOC,
    MAT_ERR_FOPEN,
    MAT_ERR_FSCANF,
    MAT_ERR_FPRINTF
};

// The partition type
typedef struct matrix_partition matrix_partition_t;
struct matrix_partition {
    unsigned row_start;
    unsigned row_end;
    unsigned col_start;
    unsigned col_end;
};

// Matrix creation/deletion
mat_err matrix_init(double (**matrix)[MATRIX_COLS]);
mat_err matrix_init_value(double (**matrix)[MATRIX_COLS], \
    double (*matrix_src)[MATRIX_COLS]);
void matrix_delete(double (**matrix)[MATRIX_COLS]);

// Matrix partitioning
void matrix_partitions(matrix_partition_t *partitions, \
    unsigned partitions_c);
void matrix_square_partitions(matrix_partition_t *partitions, \
    unsigned partitions_c);
void matrix_row_partitions(matrix_partition_t *partitions, \
        unsigned partitions_c);
        
bool ispow2(int n);
int getpow2(int n);

// Matrix file operations
mat_err matrix_file_in(double (*matrix)[MATRIX_COLS], char *input_fname);
mat_err matrix_file_out(double (*matrix)[MATRIX_COLS], char *output_fname);
mat_err skip_rows(FILE *f, unsigned skip);
mat_err skip_cols(FILE *f, unsigned skip);

#endif /* __MATRIX_H */