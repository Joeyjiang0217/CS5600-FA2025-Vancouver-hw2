#include <stdio.h>

int main() {
    // Create a pointer to an integer
    int *ptr;
    
    // Set it to NULL
    ptr = NULL;
    
    printf("About to dereference NULL pointer...\n");
    
    // Try to dereference it
    int value = *ptr;
    
    printf("Value: %d\n", value);
    
    return 0;
}