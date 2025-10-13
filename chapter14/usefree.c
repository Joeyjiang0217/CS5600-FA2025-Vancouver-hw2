#include <stdio.h>
#include <stdlib.h>

int main() {
    // Allocate array of 100 integers
    int *data = (int *)malloc(100 * sizeof(int));
    
    printf("Memory allocated successfully\n");
    
    // Initialize some values
    data[0] = 42;
    data[50] = 100;
    data[99] = 999;
    
    printf("Before free: data[50] = %d\n", data[50]);
    
    // Free the memory
    free(data);
    printf("Memory has been freed\n");
    
    // BUG: Trying to access memory after it's been freed!
    printf("After free: data[50] = %d\n", data[50]);
    
    printf("Program completed!\n");
    
    return 0;
}