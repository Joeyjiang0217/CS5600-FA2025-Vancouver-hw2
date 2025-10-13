#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc < 2)
    {
        printf("Error usage.\n");
        return -1;
    }

    printf("PID: %d\n", getpid());

    // Parse command-line arguments
    int megabytes = atoi(argv[1]);
    int duration = 0;  // 0 means run indefinitely
    
    if (argc >= 3) {
        duration = atoi(argv[2]);
    }

    // Allocate the array
    long long size = (long long)megabytes * 1024 * 1024;  // Convert MB to bytes
    char *array = (char *)malloc(size);
    if (array == NULL)
    {
        printf("malloc failed.\n");
        return -2;
    }
    printf("Successfully allocated %d MB of memory\n", megabytes);
    
    if (duration > 0) {
        printf("Will run for %d seconds\n", duration);
    } else {
        printf("Running indefinitely (press Ctrl+C to stop)\n");
    }

    // Record start time
    time_t start_time = time(NULL);
    long long iterations = 0;

    // Stream through the array continuously
    while (1) {
        // Touch each byte in the array
        for (long long i = 0; i < size; i++) {
            array[i] = (char)(i % 256);  // Write a value
        }
        
        iterations++;
        
        // Check if we should stop (if duration was specified)
        if (duration > 0) {
            time_t current_time = time(NULL);
            if (difftime(current_time, start_time) >= duration) {
                printf("Completed %lld iterations in %d seconds\n", 
                       iterations, duration);
                break;
            }
        }
        
        // Print status every 10 iterations
        if (iterations % 10 == 0) {
            printf("Iteration %lld: Touched %d MB of memory\n", 
                   iterations, megabytes);
        }
    }

    // Clean up
    free(array);
    printf("Memory freed. Exiting.\n");
    
    return 0;
}