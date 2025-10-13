#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// ============================================================================
// VECTOR IMPLEMENTATION (using realloc)
// ============================================================================

typedef struct {
    int *data;
    size_t size;      // Current number of elements
    size_t capacity;  // Total allocated capacity
} Vector;

Vector* vector_create() {
    Vector *v = (Vector *)malloc(sizeof(Vector));
    if (v == NULL) return NULL;
    
    v->data = NULL;
    v->size = 0;
    v->capacity = 0;
    return v;
}

// Add element to vector (naive approach - realloc on every add)
int vector_push_naive(Vector *v, int value) {
    if (v == NULL) return -1;
    
    // Realloc for exactly one more element
    int *new_data = (int *)realloc(v->data, (v->size + 1) * sizeof(int));
    if (new_data == NULL) return -1;
    
    v->data = new_data;
    v->data[v->size] = value;
    v->size++;
    v->capacity = v->size;
    return 0;
}

// Add element to vector (optimized - double capacity when full)
int vector_push_optimized(Vector *v, int value) {
    if (v == NULL) return -1;
    
    // If we're at capacity, expand
    if (v->size >= v->capacity) {
        size_t new_capacity = (v->capacity == 0) ? 1 : v->capacity * 2;
        int *new_data = (int *)realloc(v->data, new_capacity * sizeof(int));
        if (new_data == NULL) return -1;
        
        v->data = new_data;
        v->capacity = new_capacity;
    }
    
    v->data[v->size] = value;
    v->size++;
    return 0;
}

int vector_get(Vector *v, size_t index) {
    if (v == NULL || index >= v->size) return -1;
    return v->data[index];
}

void vector_free(Vector *v) {
    if (v == NULL) return;
    free(v->data);
    free(v);
}

// ============================================================================
// LINKED LIST IMPLEMENTATION
// ============================================================================

typedef struct Node {
    int value;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    size_t size;
} LinkedList;

LinkedList* list_create() {
    LinkedList *list = (LinkedList *)malloc(sizeof(LinkedList));
    if (list == NULL) return NULL;
    
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

int list_push(LinkedList *list, int value) {
    if (list == NULL) return -1;
    
    Node *node = (Node *)malloc(sizeof(Node));
    if (node == NULL) return -1;
    
    node->value = value;
    node->next = NULL;
    
    if (list->tail == NULL) {
        // First node
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
    
    list->size++;
    return 0;
}

int list_get(LinkedList *list, size_t index) {
    if (list == NULL || index >= list->size) return -1;
    
    Node *current = list->head;
    for (size_t i = 0; i < index; i++) {
        current = current->next;
    }
    return current->value;
}

void list_free(LinkedList *list) {
    if (list == NULL) return;
    
    Node *current = list->head;
    while (current != NULL) {
        Node *next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

// ============================================================================
// PERFORMANCE TESTING
// ============================================================================

double time_vector_naive(int n) {
    clock_t start = clock();
    
    Vector *v = vector_create();
    for (int i = 0; i < n; i++) {
        vector_push_naive(v, i);
    }
    vector_free(v);
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

double time_vector_optimized(int n) {
    clock_t start = clock();
    
    Vector *v = vector_create();
    for (int i = 0; i < n; i++) {
        vector_push_optimized(v, i);
    }
    vector_free(v);
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

double time_linked_list(int n) {
    clock_t start = clock();
    
    LinkedList *list = list_create();
    for (int i = 0; i < n; i++) {
        list_push(list, i);
    }
    list_free(list);
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

double time_random_access(int n) {
    // Test random access performance
    Vector *v = vector_create();
    LinkedList *list = list_create();
    
    // Build both structures
    for (int i = 0; i < n; i++) {
        vector_push_optimized(v, i);
        list_push(list, i);
    }
    
    // Time vector random access
    clock_t start = clock();
    int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += vector_get(v, rand() % n);
    }
    clock_t end = clock();
    double vector_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    // Time list random access
    start = clock();
    sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += list_get(list, rand() % n);
    }
    end = clock();
    double list_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    vector_free(v);
    list_free(list);
    
    printf("  Random access (1000 ops on %d elements):\n", n);
    printf("    Vector: %.6f seconds\n", vector_time);
    printf("    List:   %.6f seconds (%.1fx slower)\n", 
           list_time, list_time / vector_time);
    
    return 0;
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    printf("=== Dynamic Vector vs Linked List Performance ===\n\n");
    
    int test_sizes[] = {1000, 10000, 50000};
    int num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (int i = 0; i < num_tests; i++) {
        int n = test_sizes[i];
        printf("Test with %d elements:\n", n);
        
        double naive_time = time_vector_naive(n);
        double optimized_time = time_vector_optimized(n);
        double list_time = time_linked_list(n);
        
        printf("  Vector (naive realloc):     %.6f seconds\n", naive_time);
        printf("  Vector (optimized):         %.6f seconds (%.1fx faster than naive)\n", 
               optimized_time, naive_time / optimized_time);
        printf("  Linked List:                %.6f seconds\n", list_time);
        printf("  Winner: %s\n", 
               (optimized_time < list_time) ? "Vector (optimized)" : "Linked List");
        
        time_random_access(n);
        printf("\n");
    }
    
    // Memory usage comparison
    printf("=== Memory Overhead Analysis ===\n");
    printf("For 1000 integers:\n");
    printf("  Vector:      %zu bytes (array) + %zu bytes (struct) = %zu bytes\n",
           1000 * sizeof(int), sizeof(Vector), 1000 * sizeof(int) + sizeof(Vector));
    printf("  Linked List: %zu bytes (1000 nodes) + %zu bytes (struct) = %zu bytes\n",
           1000 * sizeof(Node), sizeof(LinkedList), 1000 * sizeof(Node) + sizeof(LinkedList));
    printf("  Overhead:    Linked list uses %.1fx more memory\n",
           (1000.0 * sizeof(Node) + sizeof(LinkedList)) / 
           (1000.0 * sizeof(int) + sizeof(Vector)));
    
    return 0;
}