#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>

static inline void pin_to_cpu0(void) {
    cpu_set_t set; CPU_ZERO(&set); CPU_SET(0, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0) perror("sched_setaffinity");
}

static inline uint64_t nsec_now(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) { perror("clock_gettime"); _exit(1); }
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

int main(int argc, char **argv) {
    const int WARMUP = 1000;                 // <-- same warmup on BOTH sides
    long iters = (argc > 1) ? atol(argv[1]) : 100000;
    if (iters <= 0) { fprintf(stderr, "iters must be > 0\n"); return 1; }

    int p2c[2], c2p[2];
    if (pipe(p2c) < 0 || pipe(c2p) < 0) { perror("pipe"); return 1; }

    pin_to_cpu0();

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    char byte = 1;

    if (pid == 0) {
        pin_to_cpu0();
        close(p2c[1]);              
        close(c2p[0]);              

        for (int i = 0; i < WARMUP; ++i) {
            if (read(p2c[0], &byte, 1) != 1) { perror("child warmup read"); _exit(1); }
            if (write(c2p[1], &byte, 1) != 1) { perror("child warmup write"); _exit(1); }
        }

        for (long i = 0; i < iters; ++i) {
            if (read(p2c[0], &byte, 1) != 1) { perror("child read"); _exit(1); }
            if (write(c2p[1], &byte, 1) != 1) { perror("child write"); _exit(1); }
        }
        _exit(0);
    }

    close(p2c[0]);                  
    close(c2p[1]);                  

    for (int i = 0; i < WARMUP; ++i) {
        if (write(p2c[1], &byte, 1) != 1) { perror("warmup write"); return 1; }
        if (read(c2p[0], &byte, 1) != 1)   { perror("warmup read");  return 1; }
    }

    uint64_t start = nsec_now();
    for (long i = 0; i < iters; ++i) {
        if (write(p2c[1], &byte, 1) != 1) { perror("write"); return 1; }
        if (read(c2p[0], &byte, 1) != 1)  { perror("read");  return 1; }
    }
    uint64_t end = nsec_now();

    double total_ns = (double)(end - start);
    double per_roundtrip_ns = total_ns / iters;
    double per_ctxswitch_ns = per_roundtrip_ns / 2.0;

    printf("Iterations (round-trips): %ld\n", iters);
    printf("Total time: %.0f ns\n", total_ns);
    printf("Avg round-trip (2 switches): %.2f ns\n", per_roundtrip_ns);
    printf("Estimated context-switch cost: %.2f ns\n", per_ctxswitch_ns);

    int status = 0;
    waitpid(pid, &status, 0);
    return 0;
}
