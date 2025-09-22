#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

static inline uint64_t nsec_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

int main(int argc, char **argv) {
    long iters = (argc > 1) ? atol(argv[1]) : 1000000; 
    if (iters <= 0) { fprintf(stderr, "iters must be > 0\n"); return 1; }

    uint64_t t0 = nsec_now(), t1 = nsec_now(), t2 = nsec_now();
    printf("Timer deltas (ns): %llu, %llu\n",
           (unsigned long long)(t1 - t0),
           (unsigned long long)(t2 - t1));

    for (int i = 0; i < 10000; ++i) (void)syscall(SYS_getpid);

    uint64_t start = nsec_now();
    for (long i = 0; i < iters; ++i) {
        (void)syscall(SYS_getpid);
    }
    uint64_t end = nsec_now();

    double total_ns = (double)(end - start);
    double per_syscall_ns = total_ns / iters;

    printf("Iterations: %ld\n", iters);
    printf("Total time: %.0f ns\n", total_ns);
    printf("Avg syscall cost: %.2f ns\n", per_syscall_ns);
    return 0;
}
