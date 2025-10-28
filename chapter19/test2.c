#include <stdio.h>
#include <sys/time.h>

int main() {
    struct timeval start, end;
    int iterations = 1000000;
    
    gettimeofday(&start, NULL);
    for (int i = 0; i < iterations; i++) {
        gettimeofday(&end, NULL);
    }
    
    long elapsed = (end.tv_sec - start.tv_sec) * 1000000 + 
                   (end.tv_usec - start.tv_usec);
    printf("Timer overhead: %.3f microseconds\n", 
           (double)elapsed / iterations);
    return 0;
}