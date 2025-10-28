#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define ITERATIONS 1000000000
#define PAGE_SIZE 4096  // Typical page size

struct timeval start, end;

int main()
{
    // Allocate memory for testing (multiple pages)
    size_t total_size = PAGE_SIZE * 100;  // Allocate 100 pages
    char *memory = malloc(total_size);
    if (memory == NULL) {
        perror("malloc failed");
        return 1;
    }
    
    // Set up page_pointer to point to a specific page
    char *page_pointer = memory + (PAGE_SIZE * 10);  // Point to page 10
    
    // Optional: Touch the page first to ensure it's in memory
    *page_pointer = 1;
    
    gettimeofday(&start, NULL);
    
    for (int i = 0; i < ITERATIONS; i++)
    {
        // Access the page
        volatile char temp = *page_pointer;
    }
    
    gettimeofday(&end, NULL);
    
    long elapsed_us = (end.tv_sec - start.tv_sec) * 1000000 + 
                      (end.tv_usec - start.tv_usec);
    double avg_time_us = (double)elapsed_us / ITERATIONS;
    
    printf("Total time: %ld microseconds\n", elapsed_us);
    printf("Average time per access: %.3f microseconds\n", avg_time_us);
    printf("Average time per access: %.3f nanoseconds\n", avg_time_us * 1000);
    
    free(memory);
    return 0;
}