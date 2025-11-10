#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "common_threads.h"

//
// Here, you have to write (almost) ALL the code. Oh no!
// How can you show that a thread does not starve
// when attempting to acquire this mutex you build?
//

typedef struct __ns_mutex_t {
    sem_t state;   // protects internal fields
    sem_t gate;    // queue semaphore: sleepers wait here
    int   waiters; // number of threads queued
    int   locked;  // 0 = free, 1 = held (baton pattern keeps this set while passing)
} ns_mutex_t;

void ns_mutex_init(ns_mutex_t *m) {
    Sem_init(&m->state, 1);  // binary semaphore (acts like a tiny spinless mutex)
    Sem_init(&m->gate,  0);  // sleepers block here
    m->waiters = 0;
    m->locked  = 0;
}

void ns_mutex_acquire(ns_mutex_t *m) {
    Sem_wait(&m->state);
    if (!m->locked && m->waiters == 0) {
        m->locked = 1;               // fast path: take the lock
        Sem_post(&m->state);
        return;
    }
    // must queue (either locked or someone already waiting)
    m->waiters++;
    Sem_post(&m->state);

    // sleep until baton is passed to us
    Sem_wait(&m->gate);

    // we are the chosen one; finalize ownership
    Sem_wait(&m->state);
    m->locked--;      // previous owner kept locked==1 while passing the baton
    // locked is now 0; we take it and account for ourselves
    m->locked = 1;
    m->waiters--;
    Sem_post(&m->state);
}

void ns_mutex_release(ns_mutex_t *m) {
    Sem_wait(&m->state);
    if (m->waiters > 0) {
        // keep m->locked = 1 and hand the baton directly to the next waiter
        Sem_post(&m->gate);
    } else {
        m->locked = 0;  // nobody waiting; just free it
    }
    Sem_post(&m->state);
}

typedef struct {
    ns_mutex_t *mtx;
    int id;
    int iters;
    volatile int *shared;
} arg_t;

void *worker(void *arg_) {
    arg_t *a = (arg_t*)arg_;
    for (int i = 0; i < a->iters; i++) {
        ns_mutex_acquire(a->mtx);
        // critical section
        int tmp = *a->shared;
        // make starvation visible by adding a small delay
        // (comment out to measure raw throughput)
        // usleep(1000);
        *a->shared = tmp + 1;
        // end CS
        ns_mutex_release(a->mtx);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int nthreads = 8, iters = 1000;
    if (argc >= 2) nthreads = atoi(argv[1]);
    if (argc >= 3) iters    = atoi(argv[2]);

    ns_mutex_t m; ns_mutex_init(&m);

    volatile int shared = 0;
    pthread_t *ths = malloc(sizeof(*ths)*nthreads);
    arg_t *args    = malloc(sizeof(*args)*nthreads);

    for (int i = 0; i < nthreads; i++) {
        args[i].mtx = &m; args[i].id = i; args[i].iters = iters; args[i].shared = &shared;
        assert(pthread_create(&ths[i], NULL, worker, &args[i]) == 0);
    }
    for (int i = 0; i < nthreads; i++) pthread_join(ths[i], NULL);

    printf("Done. shared=%d (expected %d)\n", shared, nthreads*iters);
    free(ths); free(args);
    return 0;
}
