#include "barrier.h"

/**
 * Initializes the generic barrier b. Using the id in barrier_id, it calls the
 *   appropriate initializer and sets the id in b for calls to barrier_wait.
 *   If barrier_id is invalid the program aborts.
 * Threads is used by the semaphore heap barrier so is necessary to include.
 * Returns 0 if successful, -1 on error and errno is set appropriately.
 */
int barrier_init(barrier_t *b, barrier_e barrier_id, unsigned thread_num, \
        pthread_t** threads) {
    b->barrier_id = barrier_id;
    int ret = 0;

    switch(b->barrier_id) {
	case SEM_HEAP_BARRIER:
        ret = sem_heap_barrier_init(&(b->barrier.sem_heap), thread_num, \
            threads);
		break;
    case COND_BARRIER:
        ret = cond_barrier_init(&(b->barrier.cond), thread_num);
		break;
    case PTHREAD_BARRIER:
        ret = pthread_barrier_init(&(b->barrier.pthread), NULL, thread_num);
        if (ret > 0) {
            errno = ret;
            ret = -1;
        }
		break;
    case BARRIER_TOTAL:
        abort();
        break;
	default:
		abort();
	}
    return ret;
}

/**
 * Waits for the barrier_t b. Uses the barrier_id field to determine the
 *   appropriate barrier wait function to call. If barrier_id is invalid the
 *   program aborts.
 * The thread id of the calling thread is needed by sem_heap_barrier_wait
 */
void barrier_wait(barrier_t *b, pthread_t t) {
    switch(b->barrier_id) {
	case SEM_HEAP_BARRIER:
        sem_heap_barrier_wait(&(b->barrier.sem_heap), t);
		break;
    case COND_BARRIER:
        cond_barrier_wait(&(b->barrier.cond));
		break;
    case PTHREAD_BARRIER:
        pthread_barrier_wait(&(b->barrier.pthread));
		break;
    case BARRIER_TOTAL:
        abort();
        break;
	default:
		abort();
    }
}

/**
 * Deletion of different barrier types.
 */
void barrier_delete(barrier_t *b) {
    switch(b->barrier_id) {
	case SEM_HEAP_BARRIER:
        free(b->barrier.sem_heap.sem_arrived);
		break;
    case COND_BARRIER:
        pthread_mutex_destroy(&(b->barrier.cond.mtx));
        pthread_cond_destroy(&(b->barrier.cond.barrier_ready));
		break;
    case PTHREAD_BARRIER:
        pthread_barrier_destroy(&(b->barrier.pthread));
		break;
    case BARRIER_TOTAL:
        abort();
        break;
	default:
		abort();
    }
}

/**
 * Initializes a sem_heap_barrier to block thread_num threads. Along with
 *   typical array variables, this initializes two arrays of semaphores of
 *   length thread_num and also initializes all the semaphores they contain.
 * Returns 0 if successful, -1 if memory allocation fails.
 */
int sem_heap_barrier_init(sem_heap_barrier_t *b, unsigned thread_num, pthread_t **threads) {
    int ret = 0;

    assert(b != NULL);
    ret = pthread_mutex_init(&(b->mtx), NULL);
    if (ret > 0) {
        errno = ret;
    }
    else {
        errno = 0;
        b->sem_arrived = malloc(sizeof(sem_t) * thread_num * 2);
        if (b->sem_arrived == NULL) {
            ret = -1;
        }
        else {
            b->sem_barrier_ready = b->sem_arrived + thread_num;
            b->thread_num = thread_num;
            b->threads = threads;

            int i = 0;
            while (i < thread_num && ret == 0) {
                errno = 0;
                if (sem_init(&(b->sem_arrived[i]), 0, 0) == -1) {
                    ret = -1;
                }
                else {
                    errno = 0;
                    if (sem_init(&(b->sem_barrier_ready[i]), 0, 0) == -1) {
                        ret = -1;
                    }
                }
                i++;
            }
        }
    }
    return ret;
}

/**
 * Initializes a cond_barrier to block thread_num threads.
 * Returns 0 if successful, -1 if memory allocation fails.
 */
int cond_barrier_init(cond_barrier_t *b, unsigned thread_num) {
    int ret = 0;

    assert(b != NULL);
    ret = pthread_mutex_init(&(b->mtx), NULL);
    if (ret > 0) {
        errno = ret;
    }
    else {
        b->count = 0;
        b->thread_num = thread_num;

        ret = pthread_cond_init(&(b->barrier_ready), NULL);
        if (ret > 0) {
            errno = ret;
        }
    }
    return ret;
}

/**
 * Waits for a sem_heap_barrier. Each thread is assigned a unique position in
 *   the heap by passing the list of thread_id's in with the initialization.
 * Algorithm:
 *   Every node waits for its children, signals to its parent that it has 
 *   arrived, and then waits for the signal to go. If the node is a leaf,
 *   it skips waiting for its children. If the node is a root, it skips
 *   signaling its parent. Once the root is done waiting for its left and right
 *   child, this means that all the nodes have arrived. It signals its children
 *   to go and exits. When a node is signaled to go, it signals its children
 *   to go and exits. So every node exits, and the entire heap is reset.
 */
void sem_heap_barrier_wait(sem_heap_barrier_t *b, pthread_t t) {
    unsigned curr_node = 0;
    while(curr_node < b->thread_num && pthread_equal((*(b->threads))[curr_node], t) == 0) {
        curr_node++;
    }
    if (curr_node == b->thread_num) {
        abort();
    }
    else {
        unsigned lchild = get_heap_lchild(curr_node, b->thread_num-1);
        unsigned rchild = get_heap_rchild(curr_node, b->thread_num-1);

        if (lchild > 0) {
            sem_wait(&(b->sem_arrived[lchild]));
        }
        if (rchild > 0) {
            sem_wait(&(b->sem_arrived[rchild]));
        }
        if (curr_node > 0) {
            sem_post(&(b->sem_arrived[curr_node]));
            sem_wait(&(b->sem_barrier_ready[curr_node]));
        }
        if (lchild > 0) {
            sem_post(&(b->sem_barrier_ready[lchild]));
        }
        if (rchild > 0) {
            sem_post(&(b->sem_barrier_ready[rchild]));
        }
    }
}

/**
 * Waits for a cond_barrier. If not all the threads have arrived, it waits
 *   on the condition variable. If the thread is the final thread, it signals
 *   all waiting threads and exits.
 */
void cond_barrier_wait(cond_barrier_t *b) {
    pthread_mutex_lock(&(b->mtx));
    b->count++;
    if (b->count < b->thread_num) {
        pthread_cond_wait(&(b->barrier_ready), &(b->mtx));
    }
    else {
        b->count = 0;
        for (int i = 0; i < b->thread_num-1; i++) {
            pthread_cond_signal(&(b->barrier_ready));
        }
    }
    pthread_mutex_unlock(&(b->mtx));
}

/**
 * Finds the index of a nodes left child in a heap. If a left child does not
 *   exist it returns 0.
 */
unsigned get_heap_lchild(unsigned n, unsigned heap_max) {
    unsigned lchild = 2 * n + 1;
    if (lchild > heap_max) {
        return 0;
    }
    else {
        return lchild;
    }
}

/**
 * Finds the index of a nodes right child in a heap. If a right child does not
 *   exist it returns 0.
 */
unsigned get_heap_rchild(unsigned n, unsigned heap_max) {
    unsigned rchild = 2 * n + 2;
    if (rchild > heap_max) {
        return 0;
    }
    else {
        return rchild;
    }
}
