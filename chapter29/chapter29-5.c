// Run 5 tests: each with 4 threads, each thread performs 10k, 20k, 30k, 40k, 50k updates
// Build:  gcc -O2 -pthread hash_4threads_multi_runs.c -o hbench
// Run:    ./hbench
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define THREADS   4
#define RUNS      5
#define BUCKETS   101
#define KEY_SPACE 1000000

// -------------- Timing & RNG --------------
static inline double now_sec(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}
static inline uint32_t xorshift32(uint32_t *s){
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *s = x;
}

// -------------- Simple list (no internal locks) --------------
typedef struct node {
    int key;
    struct node* next;
} node_t;

typedef struct {
    node_t* head;
} list_t;

static void List_Init(list_t* L){ L->head = NULL; }

static bool List_Insert(list_t* L, int k){
    for(node_t* c=L->head;c;c=c->next) if(c->key==k) return false;
    node_t* n=(node_t*)malloc(sizeof(*n));
    if(!n) return false;
    n->key=k; n->next=L->head; L->head=n; return true;
}

static bool List_Delete(list_t* L, int k){
    node_t** pp=&L->head; node_t* c=L->head;
    while(c){
        if(c->key==k){ *pp=c->next; free(c); return true; }
        pp=&c->next; c=c->next;
    }
    return false;
}

// -------------- Hash table (single global lock) --------------
typedef struct {
    list_t* lists;
    int nbk;
    pthread_mutex_t lock;  // one global lock
} hash_t;

static void Hash_Init(hash_t* H, int nbk){
    H->nbk = nbk;
    H->lists = (list_t*)malloc(sizeof(list_t)*nbk);
    for(int i=0;i<nbk;i++) List_Init(&H->lists[i]);
    pthread_mutex_init(&H->lock, NULL);
}
static inline int bkt(hash_t* H, int k){ return ((unsigned)k) % (unsigned)H->nbk; }

static bool Hash_Insert(hash_t* H, int k){
    pthread_mutex_lock(&H->lock);
    bool ok = List_Insert(&H->lists[bkt(H,k)], k);
    pthread_mutex_unlock(&H->lock);
    return ok;
}
static bool Hash_Delete(hash_t* H, int k){
    pthread_mutex_lock(&H->lock);
    bool ok = List_Delete(&H->lists[bkt(H,k)], k);
    pthread_mutex_unlock(&H->lock);
    return ok;
}
static inline void Hash_Update(hash_t* H, int k){
    if(!Hash_Insert(H,k)) (void)Hash_Delete(H,k);
}

// -------------- Thread worker --------------
typedef struct {
    hash_t* H;
    uint32_t seed;
    int key_space;
    uint32_t ops;
    pthread_barrier_t* bar;
} worker_arg_t;

static void* worker(void* a_){
    worker_arg_t* a = (worker_arg_t*)a_;
    uint32_t s = a->seed;
    pthread_barrier_wait(a->bar);
    for(uint32_t i=0;i<a->ops;i++){
        int k = 1 + (int)(xorshift32(&s) % a->key_space);
        Hash_Update(a->H, k);
    }
    return NULL;
}

// -------------- Main benchmark --------------
int main(void){
    const int ops_per_thread[RUNS] = {10000,20000,30000,40000,50000};
    printf("Hash table (single global lock) ¡ª 4 threads ¡Á {10k..50k} updates\n\n");

    for(int r=0; r<RUNS; ++r){
        int ops = ops_per_thread[r];
        hash_t H; Hash_Init(&H, BUCKETS);
        pthread_t th[THREADS];
        worker_arg_t wa[THREADS];
        pthread_barrier_t start_barrier;
        pthread_barrier_init(&start_barrier, NULL, THREADS+1);

        for(int i=0;i<THREADS;i++){
            wa[i].H = &H;
            wa[i].seed = (uint32_t)(0x9e3779b9u ^ (i+1)*12345);
            wa[i].key_space = KEY_SPACE;
            wa[i].ops = ops;
            wa[i].bar = &start_barrier;
            pthread_create(&th[i], NULL, worker, &wa[i]);
        }

        double t0 = now_sec();
        pthread_barrier_wait(&start_barrier);
        for(int i=0;i<THREADS;i++) pthread_join(th[i], NULL);
        double t1 = now_sec();

        double secs = t1 - t0;
        double total_ops = (double)THREADS * ops;
        double throughput = total_ops / secs;
        printf("Run %d: %d updates/thread (total %d) -> %.6f s, %.0f ops/s\n",
               r+1, ops, (int)total_ops, secs, throughput);

        pthread_barrier_destroy(&start_barrier);
    }

    return 0;
}
