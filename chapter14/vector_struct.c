#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// ============================================================================
// DATA STRUCTURE - 40 bytes
// ============================================================================

typedef struct {
    int id;
    double value1;
    double value2;
    double value3;
    char name[12];  // Total: 4 + 8 + 8 + 8 + 12 = 40 bytes
} DataItem;

// ============================================================================
// VECTOR IMPLEMENTATION (using realloc)
// ============================================================================

typedef struct {
    DataItem *data;
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
int vector_push_naive(Vector *v, DataItem item) {
    if (v == NULL) return -1;
    
    // Realloc for exactly one more element
    DataItem *new_data = (DataItem *)realloc(v->data, (v->size + 1) * sizeof(DataItem));
    if (new_data == NULL) return -1;
    
    v->data = new_data;
    v->data[v->size] = item;
    v->size++;
    v->capacity = v->size;
    return 0;
}

// Add element to vector (optimized - double capacity when full)
int vector_push_optimized(Vector *v, DataItem item) {
    if (v == NULL) return -1;
    
    // If we're at capacity, expand
    if (v->size >= v->capacity) {
        size_t new_capacity = (v->capacity == 0) ? 1 : v->capacity * 2;
        DataItem *new_data = (DataItem *)realloc(v->data, new_capacity * sizeof(DataItem));
        if (new_data == NULL) return -1;
        
        v->data = new_data;
        v->capacity = new_capacity;
    }
    
    v->data[v->size] = item;
    v->size++;
    return 0;
}

DataItem* vector_get(Vector *v, size_t index) {
    if (v == NULL || index >= v->size) return NULL;
    return &v->data[index];
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
    DataItem item;
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

int list_push(LinkedList *list, DataItem item) {
    if (list == NULL) return -1;
    
    Node *node = (Node *)malloc(sizeof(Node));
    if (node == NULL) return -1;
    
    node->item = item;
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

DataItem* list_get(LinkedList *list, size_t index) {
    if (list == NULL || index >= list->size) return NULL;
    
    Node *current = list->head;
    for (size_t i = 0; i < index; i++) {
        current = current->next;
    }
    return &current->item;
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
// HELPER FUNCTIONS
// ============================================================================

DataItem create_item(int id) {
    DataItem item;
    item.id = id;
    item.value1 = id * 1.5;
    item.value2 = id * 2.7;
    item.value3 = id * 3.14;
    snprintf(item.name, sizeof(item.name), "item_%d", id);
    return item;
}

// ============================================================================
// PERFORMANCE TESTING
// ============================================================================

double time_vector_naive(int n) {
    clock_t start = clock();
    
    Vector *v = vector_create();
    for (int i = 0; i < n; i++) {
        DataItem item = create_item(i);
        vector_push_naive(v, item);
    }
    vector_free(v);
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

double time_vector_optimized(int n) {
    clock_t start = clock();
    
    Vector *v = vector_create();
    for (int i = 0; i < n; i++) {
        DataItem item = create_item(i);
        vector_push_optimized(v, item);
    }
    vector_free(v);
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

double time_linked_list(int n) {
    clock_t start = clock();
    
    LinkedList *list = list_create();
    for (int i = 0; i < n; i++) {
        DataItem item = create_item(i);
        list_push(list, item);
    }
    list_free(list);
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

void time_random_access(int n) {
    // Test random access performance
    Vector *v = vector_create();
    LinkedList *list = list_create();
    
    // Build both structures
    for (int i = 0; i < n; i++) {
        DataItem item = create_item(i);
        vector_push_optimized(v, item);
        list_push(list, item);
    }
    
    // Time vector random access
    clock_t start = clock();
    double sum = 0;
    for (int i = 0; i < 1000; i++) {
        DataItem *item = vector_get(v, rand() % n);
        if (item) sum += item->value1;
    }
    clock_t end = clock();
    double vector_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    // Time list random access
    start = clock();
    sum = 0;
    for (int i = 0; i < 1000; i++) {
        DataItem *item = list_get(list, rand() % n);
        if (item) sum += item->value1;
    }
    end = clock();
    double list_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    vector_free(v);
    list_free(list);
    
    printf("  Random access (1000 ops on %d elements):\n", n);
    printf("    Vector: %.6f seconds\n", vector_time);
    printf("    List:   %.6f seconds", list_time);
    if (vector_time > 0) {
        printf(" (%.1fx slower)\n", list_time / vector_time);
    } else {
        printf("\n");
    }
}

void count_reallocs(int n) {
    // Count how many times realloc is called
    int naive_reallocs = n;  // Every insertion
    
    int optimized_reallocs = 0;
    size_t capacity = 0;
    for (int i = 0; i < n; i++) {
        if (i >= capacity) {
            capacity = (capacity == 0) ? 1 : capacity * 2;
            optimized_reallocs++;
        }
    }
    
    printf("  Realloc calls:\n");
    printf("    Naive:     %d calls\n", naive_reallocs);
    printf("    Optimized: %d calls (%.1fx fewer)\n", 
           optimized_reallocs, (double)naive_reallocs / optimized_reallocs);
    
    // Calculate total bytes copied
    size_t naive_copies = 0;
    for (int i = 1; i <= n; i++) {
        naive_copies += i * sizeof(DataItem);  // Copy all existing elements
    }
    
    size_t optimized_copies = 0;
    capacity = 0;
    size_t size = 0;
    for (int i = 0; i < n; i++) {
        if (i >= capacity) {
            optimized_copies += size * sizeof(DataItem);  // Copy existing elements
            capacity = (capacity == 0) ? 1 : capacity * 2;
        }
        size++;
    }
    
    printf("  Total bytes copied:\n");
    printf("    Naive:     %zu bytes (%.2f MB)\n", 
           naive_copies, naive_copies / (1024.0 * 1024.0));
    printf("    Optimized: %zu bytes (%.2f MB, %.1fx less)\n", 
           optimized_copies, optimized_copies / (1024.0 * 1024.0),
           (double)naive_copies / optimized_copies);
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    printf("=== Dynamic Vector vs Linked List Performance ===\n");
    printf("DataItem size: %zu bytes\n\n", sizeof(DataItem));
    
    int test_sizes[] = {1000, 10000, 50000};
    int num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (int i = 0; i < num_tests; i++) {
        int n = test_sizes[i];
        printf("Test with %d elements (%zu KB of data):\n", 
               n, (n * sizeof(DataItem)) / 1024);
        
        double naive_time = time_vector_naive(n);
        double optimized_time = time_vector_optimized(n);
        double list_time = time_linked_list(n);
        
        printf("  Insertion performance:\n");
        printf("    Vector (naive):     %.6f seconds\n", naive_time);
        printf("    Vector (optimized): %.6f seconds (%.1fx faster than naive)\n", 
               optimized_time, naive_time / optimized_time);
        printf("    Linked List:        %.6f seconds\n", list_time);
        
        if (optimized_time < list_time) {
            printf("    Winner: Vector (optimized) - %.1fx faster than list\n", 
                   list_time / optimized_time);
        } else {
            printf("    Winner: Linked List - %.1fx faster than vector\n", 
                   optimized_time / list_time);
        }
        
        time_random_access(n);
        
        if (i == 0) {  // Only show details for first test
            count_reallocs(n);
        }
        printf("\n");
    }
    
    // Memory usage comparison
    printf("=== Memory Overhead Analysis ===\n");
    printf("For 1000 DataItems (40 bytes each):\n");
    printf("  Vector:\n");
    printf("    Data:     %zu bytes (40,000 bytes)\n", 1000 * sizeof(DataItem));
    printf("    Struct:   %zu bytes\n", sizeof(Vector));
    printf("    Total:    %zu bytes\n", 
           1000 * sizeof(DataItem) + sizeof(Vector));
    printf("    Wasted:   ~%zu bytes (when capacity doubled)\n", 
           1000 * sizeof(DataItem) / 2);
    printf("\n");
    printf("  Linked List:\n");
    printf("    Data:     %zu bytes (40,000 bytes)\n", 1000 * sizeof(DataItem));
    printf("    Pointers: %zu bytes (%zu per node)\n", 
           1000 * sizeof(void*), sizeof(void*));
    printf("    Struct:   %zu bytes\n", sizeof(LinkedList));
    printf("    Total:    %zu bytes\n", 
           1000 * sizeof(Node) + sizeof(LinkedList));
    printf("\n");
    printf("  Comparison:\n");
    printf("    Linked list uses %.2fx more memory than vector\n",
           (1000.0 * sizeof(Node) + sizeof(LinkedList)) / 
           (1000.0 * sizeof(DataItem) + sizeof(Vector)));
    
    return 0;
}