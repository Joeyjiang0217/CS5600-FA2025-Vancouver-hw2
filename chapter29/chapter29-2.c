// counter_bench_per_thread.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>  // for sysconf()

typedef struct __counter_t {
    int value;
    pthread_mutex_t lock;
} counter_t;

void init(counter_t *c) {
    c->value = 0;
    pthread_mutex_init(&c->lock, NULL);
}
void increment(counter_t *c) {
    pthread_mutex_lock(&c->lock);
    c->value++;
    pthread_mutex_unlock(&c->lock);
}
int get(counter_t *c) {
    pthread_mutex_lock(&c->lock);
    int rc = c->value;
    pthread_mutex_unlock(&c->lock);
    return rc;
}

static inline double timespec_sec_diff(struct timespec a, struct timespec b) {
    long sec  = b.tv_sec  - a.tv_sec;
    long nsec = b.tv_nsec - a.tv_nsec;
    if (nsec < 0) { --sec; nsec += 1000000000L; }
    return (double)sec + (double)nsec / 1e9;
}

typedef struct {
    counter_t *ctr;
    uint64_t iters;                 // per-thread iterations (same for all threads)
    pthread_barrier_t *start_barrier;
} worker_arg_t;

void* worker(void *arg_) {
    worker_arg_t *arg = (worker_arg_t*)arg_;
    pthread_barrier_wait(arg->start_barrier); // start together
    for (uint64_t i = 0; i < arg->iters; ++i) {
        increment(arg->ctr);
    }
    return NULL;
}

double run_trial(int n_threads, uint64_t per_thread_increments) {
    counter_t c;
    init(&c);

    pthread_t *ths = (pthread_t*)malloc(sizeof(pthread_t) * n_threads);
    worker_arg_t *args = (worker_arg_t*)malloc(sizeof(worker_arg_t) * n_threads);

    pthread_barrier_t start_barrier;
    pthread_barrier_init(&start_barrier, NULL, n_threads + 1);

    for (int i = 0; i < n_threads; ++i) {
        args[i].ctr = &c;
        args[i].iters = per_thread_increments;   // *** key change: fixed per-thread work
        args[i].start_barrier = &start_barrier;
        if (pthread_create(&ths[i], NULL, worker, &args[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    pthread_barrier_wait(&start_barrier); // release workers simultaneously

    for (int i = 0; i < n_threads; ++i) {
        pthread_join(ths[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    // Sanity: total should equal per-thread * threads
    uint64_t expected = per_thread_increments * (uint64_t)n_threads;
    int final = get(&c);
    if ((uint64_t)final != expected) {
        fprintf(stderr, "ERROR: expected %llu, got %d\n",
                (unsigned long long)expected, final);
        exit(2);
    }

    pthread_barrier_destroy(&start_barrier);
    pthread_mutex_destroy(&c.lock);
    free(ths);
    free(args);

    return timespec_sec_diff(t0, t1);
}

static int dblcmp(const void* a, const void* b) {
    double da = *(const double*)a, db = *(const double*)b;
    return (da > db) - (da < db);
}

int main(void) {
    const uint64_t PER_THREAD = 1000000ULL;  // 1M per thread
    const int TRIALS = 5;                    // run several times, take median

    long ncpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpus < 1) ncpus = 1;

    printf("System has %ld available CPU cores\n", ncpus);

    printf("Benchmark: mutex-protected counter, %llu increments PER THREAD\n\n",
           (unsigned long long)PER_THREAD);
    printf("%-8s %-12s %-12s\n", "Threads", "Time(s)", "Total/s");
    printf("--------------------------------------\n");

    for (int t = 1; t <= 4; ++t) {
        double secs[TRIALS];
        for (int k = 0; k < TRIALS; ++k) {
            secs[k] = run_trial(t, PER_THREAD);
        }
        qsort(secs, TRIALS, sizeof(double), dblcmp);
        double med = secs[TRIALS/2];
        double total_ops = (double)PER_THREAD * (double)t;
        printf("%-8d %-12.6f %-12.0f\n", t, med, total_ops / med);
    }
    return 0;
}
