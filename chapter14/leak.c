#include <stdio.h>
#include <stdlib.h>

int main() {
    // Allocate memory for 100 integers
    int *data = (int *)malloc(100 * sizeof(int));
    
    printf("Memory allocated successfully at address: %p\n", (void *)data);
    
    // Use the memory
    data[0] = 42;
    printf("Set data[0] = %d\n", data[0]);
    
    printf("Exiting program...\n");
    
    // Oops! Forgot to free(data)
    return 0;
}