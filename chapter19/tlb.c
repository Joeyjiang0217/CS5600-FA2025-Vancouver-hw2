#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <stdint.h>

#define PAGESIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <number_of_pages> <number_of_trials>\n", argv[0]);
        return 1;
    }
    
    int num_pages = atoi(argv[1]);
    long num_trials = atol(argv[2]);
    
    // Allocate array of pointers
    size_t array_size = num_pages * PAGESIZE;
    char *memory = (char *)malloc(array_size);
    if (!memory) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    // Create a circular linked list through pages
    // Each page points to the next page
    uintptr_t **ptrs = (uintptr_t **)memory;
    for (int i = 0; i < num_pages; i++) {
        ptrs[i * (PAGESIZE / sizeof(uintptr_t *))] = 
            (uintptr_t *)&ptrs[((i + 1) % num_pages) * (PAGESIZE / sizeof(uintptr_t *))];
    }
    
    // Pin to CPU 0
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    sched_setaffinity(0, sizeof(set), &set);
    
    // Start pointer
    uintptr_t **p = &ptrs[0];
    
    // Warm up
    for (int i = 0; i < num_pages * 1000; i++) {
        p = (uintptr_t **)*p;
    }
    
    // Measure time
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Pointer chasing - cannot be optimized away!
    for (long trial = 0; trial < num_trials; trial++) {
        for (int i = 0; i < num_pages; i++) {
            p = (uintptr_t **)*p;  // Follow pointer to next page
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    // Prevent dead code elimination
    if (p == (uintptr_t **)0x1) {
        printf("Prevent optimization\n");
    }
    
    // Calculate time per access
    long long start_ns = (long long)start.tv_sec * 1000000000LL + start.tv_nsec;
    long long end_ns = (long long)end.tv_sec * 1000000000LL + end.tv_nsec;
    long long total_ns = end_ns - start_ns;
    
    double total_accesses = (double)num_trials * (double)num_pages;
    double time_per_access = (double)total_ns / total_accesses;
    
    // Output results
    printf("%d pages\n", num_pages);
    printf("%ld trials\n", num_trials);
    printf("%.2f nanoseconds\n", time_per_access);
    
    free(memory);
    return 0;
}