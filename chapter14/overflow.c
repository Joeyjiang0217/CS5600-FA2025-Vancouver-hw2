#include <stdio.h>
#include <stdlib.h>

int main() {
    // Allocate array of 100 integers
    int *data = (int *)malloc(100 * sizeof(int));
    
    printf("Memory allocated successfully for 100 integers\n");
    printf("Valid indices: 0 to 99\n");
    
    // Initialize some values
    data[0] = 1;
    data[99] = 99;
    
    printf("data[0] = %d\n", data[0]);
    printf("data[99] = %d\n", data[99]);
    
    // BUG: Writing to index 100, which is out of bounds!
    printf("Setting data[100] = 0...\n");
    data[100] = 0;
    
    printf("Program completed successfully!\n");
    
    free(data);
    return 0;
}