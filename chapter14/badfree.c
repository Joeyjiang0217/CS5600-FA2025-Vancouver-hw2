#include <stdio.h>
#include <stdlib.h>

int main() {
    // Allocate array of 100 integers
    int *data = (int *)malloc(100 * sizeof(int));
    
    printf("Memory allocated successfully at address: %p\n", (void *)data);
    
    // Initialize some values
    data[0] = 1;
    data[50] = 50;
    data[99] = 99;
    
    printf("data[0] = %d\n", data[0]);
    printf("data[50] = %d (at address %p)\n", data[50], (void *)&data[50]);
    
    // BUG: Try to free a pointer to the MIDDLE of the array
    // This is NOT the pointer that malloc returned!
    printf("Attempting to free pointer to middle of array...\n");
    free(&data[50]);
    
    printf("Program completed!\n");
    
    return 0;
}