CC=gcc
SPEED_TEST_OPT=-Wall -pthread -O2
BARR_TEST_OPT=${SPEED_TEST_OPT} -DBARRIER_TEST
DIFF_CHECK_OPT=-Wall -O2

SPEED_TEST_OUT=jacobi_speedup_test
BARR_TEST_OUT=jacobi_barrier_test
DIFF_CHECK_OUT=diff_check

SRC_DIR=./src
SPEED_TEST_SRC=${SRC_DIR}/jacobi_iterator.c ${SRC_DIR}/options.c \
			  ${SRC_DIR}/matrix.c ${SRC_DIR}/barrier.c
BARR_TEST_SRC=${SPEED_TEST_SRC}
DIFF_CHECK_SRC=${SRC_DIR}/diff_check.c ${SRC_DIR}/matrix.c

all: jacobi_barrier_test jacobi_speedup_test diff_check

jacobi_speedup_test: ${SPEED_TEST_SRC}
	${CC} -o ${SPEED_TEST_OUT} ${SPEED_TEST_OPT} ${SPEED_TEST_SRC}

jacobi_barrier_test: ${BARR_TEST_SRC}
	${CC} -o ${BARR_TEST_OUT} ${BARR_TEST_OPT} ${BARR_TEST_SRC}

diff_check: ${DIFF_CHECK_SRC}
	${CC} -o ${DIFF_CHECK_OUT} ${DIFF_CHECK_OPT} ${DIFF_CHECK_SRC}

clean:
	rm ${SPEED_TEST_OUT}
	rm ${BARR_TEST_OUT}
	rm ${DIFF_CHECK_OUT}
