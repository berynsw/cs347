#include <stdio.h>
#include "matrix.h"
#include <math.h>
#include <assert.h>

/**
 * Extremely simple utility to check if a solved matrix is valid. Prints in a
 *   form compatible with the test output.
 * Returns 0 on success, and another value if one of the (many) assertions
 *   fails.
 */
int main(int argc, char **argv) {
    double (*m1)[MATRIX_COLS];
    double (*m2)[MATRIX_COLS];
    assert(matrix_init(&m1) == MAT_ERR_NONE);
    assert(matrix_init(&m2) == MAT_ERR_NONE);
    assert(matrix_file_in(m1, argv[1]) == MAT_ERR_NONE);
    assert(matrix_file_in(m2, argv[2]) == MAT_ERR_NONE);

    double delta, delta_max = 0.0, delta_min = 100.0;
    for (int r = 0; r < MATRIX_ROWS; r++) {
        for (int c = 0; c < MATRIX_COLS; c++) {
            delta = fabs(m1[r][c] - m2[r][c]);
            if (delta > delta_max) {
                delta_max = delta;
            }
            if (delta < delta_min) {
                delta_min = delta;
            }
        }
    }
    printf("%.10e,%.10e,", delta_min, delta_max);
    return 0;
}