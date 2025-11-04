// approx_counter_bench_tabular.c
// Compile:  gcc -O2 -pthread approx_counter_bench_tabular.c -o bench
// Run:      ./bench
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#ifndef MAX_THREADS
#define MAX_THREADS 64
#endif

typedef struct {
    int              global;                    // global count
    pthread_mutex_t  glock;                     // global lock
    int              local[MAX_THREADS];        // per-thread local counts
    pthread_mutex_t  llock[MAX_THREADS];        // per-thread locks
    int              threshold;                 // flush threshold
    int              nslots;                    // how many locals are in use
} counter_t;

typedef struct {
    counter_t        *ctr;
    int               tid;                      // 0..n_threads-1
    uint64_t          iters;
    pthread_barrier_t *start_barrier;
} worker_arg_t;

// --- counter implementation (approximate) ---
static void counter_init(counter_t *c, int threshold, int nslots) {
    c->threshold = threshold;
    c->nslots    = nslots;
    c->global    = 0;
    pthread_mutex_init(&c->glock, NULL);
    for (int i = 0; i < nslots; ++i) {
        c->local[i] = 0;
        pthread_mutex_init(&c->llock[i], NULL);
    }
}

static inline void counter_update(counter_t *c, int tid, int amt) {
    int slot = tid; // 1:1 mapping
    pthread_mutex_lock(&c->llock[slot]);
    c->local[slot] += amt;
    if (c->local[slot] >= c->threshold) {
        pthread_mutex_lock(&c->glock);
        c->global += c->local[slot];
        pthread_mutex_unlock(&c->glock);
        c->local[slot] = 0;
    }
    pthread_mutex_unlock(&c->llock[slot]);
}

static int counter_get(counter_t *c) { // (approximate)
    pthread_mutex_lock(&c->glock);
    int v = c->global;
    pthread_mutex_unlock(&c->glock);
    return v;
}

static void counter_flush(counter_t *c) { // for verification after timing
    for (int i = 0; i < c->nslots; ++i) {
        pthread_mutex_lock(&c->llock[i]);
        int v = c->local[i];
        c->local[i] = 0;
        pthread_mutex_unlock(&c->llock[i]);
        if (v) {
            pthread_mutex_lock(&c->glock);
            c->global += v;
            pthread_mutex_unlock(&c->glock);
        }
    }
}

static void *worker(void *arg_) {
    worker_arg_t *arg = (worker_arg_t*)arg_;
    pthread_barrier_wait(arg->start_barrier); // one-barrier start
    for (uint64_t i = 0; i < arg->iters; ++i) {
        counter_update(arg->ctr, arg->tid, 1);
    }
    return NULL;
}

static inline double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static double run_once(int n_threads, int threshold, uint64_t per_thread_iters) {
    counter_t c;
    counter_init(&c, threshold, n_threads);

    pthread_t *ths = (pthread_t*)malloc(sizeof(pthread_t) * n_threads);
    worker_arg_t *args = (worker_arg_t*)malloc(sizeof(worker_arg_t) * n_threads);
    pthread_barrier_t start_barrier;
    pthread_barrier_init(&start_barrier, NULL, n_threads + 1);

    for (int i = 0; i < n_threads; ++i) {
        args[i].ctr = &c;
        args[i].tid = i;
        args[i].iters = per_thread_iters;
        args[i].start_barrier = &start_barrier;
        pthread_create(&ths[i], NULL, worker, &args[i]);
    }

    double t0 = now_sec();
    pthread_barrier_wait(&start_barrier);
    for (int i = 0; i < n_threads; ++i) pthread_join(ths[i], NULL);
    double t1 = now_sec();

    counter_flush(&c); // not timed; sanity check only
    uint64_t expected = (uint64_t)n_threads * per_thread_iters;
    if ((uint64_t)c.global != expected) {
        fprintf(stderr, "WARN: final=%d expected=%llu (thr=%d threads=%d)\n",
                c.global, (unsigned long long)expected, threshold, n_threads);
    }

    pthread_barrier_destroy(&start_barrier);
    free(ths);
    free(args);
    return t1 - t0;
}

// pretty line
static void print_sep(void) {
    puts("---------------------------------------");
}

int main(void) {
    // Thresholds requested: 1..1024 (powers of two)
    const int thresholds[] = {1,2,4,8,16,32,64,128,256,512,1024};
    const int n_thr_vals[] = {1,2,3,4};
    const uint64_t iters_per_thread = 1000000ULL;

    puts("Approximate counter benchmark (per-CPU + threshold)");
    printf("Each thread performs %llu increments\n\n",
           (unsigned long long)iters_per_thread);

    for (int ti = 0; ti < (int)(sizeof(n_thr_vals)/sizeof(n_thr_vals[0])); ++ti) {
        int nt = n_thr_vals[ti];
        printf("Threads: %d\n", nt);
        printf("%-10s %-10s %-15s\n", "Threshold", "Time(s)", "Total/s");
        print_sep();

        for (int th = 0; th < (int)(sizeof(thresholds)/sizeof(thresholds[0])); ++th) {
            int threshold = thresholds[th];
            double secs = run_once(nt, threshold, iters_per_thread);
            unsigned long long total = (unsigned long long)nt * iters_per_thread;
            double per_sec = (secs > 0.0) ? (double)total / secs : 0.0;

            // Format like your screenshot
            printf("%-10d %-10.6f %-15.0f\n", threshold, secs, per_sec);
        }
        puts(""); // blank line between thread groups
    }
    return 0;
}
