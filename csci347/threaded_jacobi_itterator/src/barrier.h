#ifndef __BARRIER_H
#define __BARRIER_H
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

// Barrier definitions
typedef struct sem_heap_barrier sem_heap_barrier_t;
struct sem_heap_barrier {
    pthread_mutex_t mtx;
    unsigned thread_num;
    sem_t *sem_arrived;
    sem_t *sem_barrier_ready;
    pthread_t **threads;
};

typedef struct cond_barrier cond_barrier_t;
struct cond_barrier {
    pthread_mutex_t mtx;
    unsigned count;
    unsigned thread_num;
    pthread_cond_t barrier_ready;
};

// enum to uniquely id each type of barrier
typedef enum barrier_e barrier_e;
enum barrier_e {
    SEM_HEAP_BARRIER = 0,
    COND_BARRIER     = 1,
    PTHREAD_BARRIER  = 2,
    BARRIER_TOTAL    = 3
};

// Generic barrier type. Union of all possible barriers and an enum to
//   differentiate the different barrier types.
typedef struct barrier_t barrier_t;
struct barrier_t {
    union barrier_u {
        sem_heap_barrier_t sem_heap;
        cond_barrier_t     cond;
        pthread_barrier_t  pthread;
    } barrier;
    barrier_e barrier_id;
};
// Initializes the generic barrier b with a given id in barrier_id. b must point
//   to an already allocated barrier_t.
int barrier_init(barrier_t *b, barrier_e barrier_id, unsigned thread_num, pthread_t **threads);
// Waits for the generic barrier b. b must point to an already allocated
//   barrier_t.
void barrier_wait(barrier_t *b, pthread_t t);
// Deletes a barrier. Simple stuff.
void barrier_delete(barrier_t *b);

// Initialization for a sem_heap_barrier. b must point to an allready allocated
//   sem_heap_barrier_t.
int sem_heap_barrier_init(sem_heap_barrier_t *b, unsigned thread_num, pthread_t **threads);
// Initialization for a cond_barrier. b must point to an allready allocated
//   cond_barrier_t.
int cond_barrier_init(cond_barrier_t *b, unsigned thread_num);

// Wait for a sem_heap_barrier. The b must already be initialized.
void sem_heap_barrier_wait(sem_heap_barrier_t *b, pthread_t t);
// Wait for a sem_heap_barrier. The b must already be initialized.
void cond_barrier_wait(cond_barrier_t *b);

// Helpers for sem_heap_barrier_wait.
unsigned get_heap_lchild(unsigned n, unsigned heap_max);
unsigned get_heap_rchild(unsigned n, unsigned heap_max);

#endif /* __BARRIER_H */