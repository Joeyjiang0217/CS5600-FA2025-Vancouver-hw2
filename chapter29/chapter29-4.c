// hand_over_hand_list_bench.c
// Build: gcc -O2 -pthread hand_over_hand_list_bench.c -o llbench
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#define CACHELINE 64
static inline double now_sec(void){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts); return ts.tv_sec + ts.tv_nsec*1e-9; }
static inline uint32_t xorshift32(uint32_t *s){ uint32_t x=*s; x^=x<<13; x^=x>>17; x^=x<<5; return *s=x; }

// ---------------- Standard single-lock list (unsorted) ----------------
typedef struct node_glob {
    int key;
    struct node_glob* next;
} node_glob_t;

typedef struct {
    node_glob_t* head;
    pthread_mutex_t lock;
} list_global_t;

static void list_global_init(list_global_t* L){
    L->head = NULL;
    pthread_mutex_init(&L->lock, NULL);
}

static bool list_global_lookup(list_global_t* L, int k){
    bool found = false;
    pthread_mutex_lock(&L->lock);
    for(node_glob_t* c=L->head; c; c=c->next){ if(c->key==k){ found=true; break; } }
    pthread_mutex_unlock(&L->lock);
    return found;
}

static bool list_global_insert(list_global_t* L, int k){
    pthread_mutex_lock(&L->lock);
    // avoid duplicates for fair comparison
    node_glob_t* c = L->head;
    while(c){ if(c->key==k){ pthread_mutex_unlock(&L->lock); return false; } c=c->next; }
    node_glob_t* n = (node_glob_t*)malloc(sizeof(*n));
    if(!n){ pthread_mutex_unlock(&L->lock); return false; }
    n->key=k; n->next=L->head; L->head=n;
    pthread_mutex_unlock(&L->lock);
    return true;
}

static bool list_global_delete(list_global_t* L, int k){
    pthread_mutex_lock(&L->lock);
    node_glob_t** pp = &L->head;
    node_glob_t*  c  = L->head;
    while(c){
        if(c->key==k){ *pp = c->next; free(c); pthread_mutex_unlock(&L->lock); return true; }
        pp=&c->next; c=c->next;
    }
    pthread_mutex_unlock(&L->lock);
    return false;
}

// ---------------- Hand-over-hand (lock-coupled) sorted list ----------------
// Sorted ascending with sentinel head(INT_MIN) and tail(INT_MAX).
typedef struct node_hoh {
    int key;
    struct node_hoh* next;
    pthread_mutex_t  m;
    char pad[CACHELINE - sizeof(int) - sizeof(void*) - sizeof(pthread_mutex_t) % CACHELINE];
} node_hoh_t;

typedef struct {
    node_hoh_t* head; // sentinel head
    node_hoh_t* tail; // sentinel tail
} list_hoh_t;

static node_hoh_t* mk_node(int k, node_hoh_t* next){
    node_hoh_t* n = (node_hoh_t*)aligned_alloc(CACHELINE, sizeof(node_hoh_t));
    if(!n) return NULL;
    n->key = k; n->next = next; pthread_mutex_init(&n->m, NULL);
    return n;
}

static void list_hoh_init(list_hoh_t* L){
    L->tail = mk_node(INT_MAX, NULL);
    L->head = mk_node(INT_MIN, L->tail);
}

static void list_hoh_lock(node_hoh_t* n){ pthread_mutex_lock(&n->m); }
static void list_hoh_unlock(node_hoh_t* n){ pthread_mutex_unlock(&n->m); }

// internal search: returns locked pair (pred,curr) such that pred->key < k <= curr->key
static void list_hoh_find(list_hoh_t* L, int k, node_hoh_t** out_pred, node_hoh_t** out_curr){
    node_hoh_t* pred = L->head;
    list_hoh_lock(pred);
    node_hoh_t* curr = pred->next;
    list_hoh_lock(curr);
    while(curr->key < k){
        list_hoh_unlock(pred);
        pred = curr;
        curr = curr->next;
        list_hoh_lock(curr);
    }
    *out_pred = pred; *out_curr = curr;
}

static bool list_hoh_lookup(list_hoh_t* L, int k){
    node_hoh_t* pred; node_hoh_t* curr;
    list_hoh_find(L, k, &pred, &curr);
    bool found = (curr->key == k);
    list_hoh_unlock(curr); list_hoh_unlock(pred);
    return found;
}

static bool list_hoh_insert(list_hoh_t* L, int k){
    node_hoh_t* pred; node_hoh_t* curr;
    list_hoh_find(L, k, &pred, &curr);
    if(curr->key == k){ list_hoh_unlock(curr); list_hoh_unlock(pred); return false; }
    node_hoh_t* n = mk_node(k, curr);
    if(!n){ list_hoh_unlock(curr); list_hoh_unlock(pred); return false; }
    // pred and curr are both locked (lock coupling guarantees adjacency)
    pred->next = n;
    list_hoh_unlock(curr); list_hoh_unlock(pred);
    return true;
}

static bool list_hoh_delete(list_hoh_t* L, int k){
    node_hoh_t* pred; node_hoh_t* curr;
    list_hoh_find(L, k, &pred, &curr);
    if(curr->key != k){ list_hoh_unlock(curr); list_hoh_unlock(pred); return false; }
    // remove curr
    pred->next = curr->next;
    list_hoh_unlock(curr); list_hoh_unlock(pred);
    pthread_mutex_destroy(&curr->m);
    free(curr);
    return true;
}

// ---------------- Benchmark harness ----------------
typedef struct {
    // shared config
    int impl; // 0=global, 1=hoh
    uint64_t ops;
    int key_space;     // keys drawn uniformly from [1..key_space]
    int pct_lookup;    // e.g., 90
    int pct_insert;    // e.g., 5 (rest delete)
    pthread_barrier_t *bar;

    // lists (one of them is used depending on impl)
    list_global_t *LG;
    list_hoh_t    *LH;

    // thread-local
    uint32_t seed;
} worker_arg_t;

static void* worker(void* a_){
    worker_arg_t* a = (worker_arg_t*)a_;
    uint32_t s = (uint32_t)(0x9e3779b9 ^ (uintptr_t)a); // cheap per-thread seed
    a->seed = s;
    pthread_barrier_wait(a->bar);
    for(uint64_t i=0;i<a->ops;i++){
        int r = (int)(xorshift32(&s) % 100);
        int k = 1 + (int)(xorshift32(&s) % a->key_space);
        if(a->impl==0){
            if(r < a->pct_lookup)          (void)list_global_lookup(a->LG, k);
            else if(r < a->pct_lookup+a->pct_insert) (void)list_global_insert(a->LG, k);
            else                            (void)list_global_delete(a->LG, k);
        }else{
            if(r < a->pct_lookup)          (void)list_hoh_lookup(a->LH, k);
            else if(r < a->pct_lookup+a->pct_insert) (void)list_hoh_insert(a->LH, k);
            else                            (void)list_hoh_delete(a->LH, k);
        }
    }
    return NULL;
}

static void fill_initial_global(list_global_t* L, int n, int key_space){
    for(int i=0;i<n;i++){
        int k = 1 + rand()%key_space;
        list_global_insert(L,k);
    }
}
static void fill_initial_hoh(list_hoh_t* L, int n, int key_space){
    for(int i=0;i<n;i++){
        int k = 1 + rand()%key_space;
        list_hoh_insert(L,k);
    }
}

// small parser
static int parse_list(const char* s, int* out, int maxn){
    int n=0; const char* p=s;
    while(*p && n<maxn){
        out[n++] = strtol(p,(char**)&p,10);
        if(*p==',') p++;
        else break;
    }
    return n;
}

int main(int argc, char** argv){
    // defaults
    uint64_t ops_per_thread = 5000ULL;
    int init_keys = 100000;
    int key_space = 1000000;
    int pct_lookup=90, pct_insert=5; // delete = 5
    int thr_list[16]; int nthr = 0; thr_list[nthr++]=1; thr_list[nthr++]=2; thr_list[nthr++]=4; thr_list[nthr++]=8;

    // args
    for(int i=1;i<argc;i++){
        if(!strncmp(argv[i],"--ops=",6)) ops_per_thread = strtoull(argv[i]+6,NULL,10);
        else if(!strncmp(argv[i],"--init=",7)) init_keys = atoi(argv[i]+7);
        else if(!strncmp(argv[i],"--keys=",7)) key_space = atoi(argv[i]+7);
        else if(!strncmp(argv[i],"--threads=",10)) nthr = parse_list(argv[i]+10, thr_list, 16);
        else if(!strncmp(argv[i],"--mix=",6)){ // "90,5,5"
            int v[3]={0}; parse_list(argv[i]+6, v, 3);
            pct_lookup=v[0]; pct_insert=v[1];
        }
        else {
            fprintf(stderr,"Usage: %s [--ops=N] [--init=N] [--keys=N] [--threads=1,2,4,8] [--mix=L,I,D]\n", argv[0]);
            return 1;
        }
    }

    printf("Linked-list benchmark (global-lock vs hand-over-hand)\n");
    printf("Init=%d keys, KeySpace=%d, Mix L/I/D=%d/%d/%d, Ops/Thr=%llu\n\n",
           init_keys, key_space, pct_lookup, pct_insert, 100-pct_lookup-pct_insert,
           (unsigned long long)ops_per_thread);

    // --- run both implementations
    for(int impl=0; impl<2; ++impl){
        const char* name = (impl==0) ? "Global-lock (single mutex)" : "Hand-over-hand (lock coupling)";
        printf("%s\n", name);
        printf("%-8s %-10s %-12s\n","Threads","Time(s)","Ops/sec");
        puts("---------------------------------------");

        for(int ti=0; ti<nthr; ++ti){
            int T = thr_list[ti];
            pthread_t *th = (pthread_t*)malloc(sizeof(pthread_t)*T);
            worker_arg_t *wa = (worker_arg_t*)malloc(sizeof(worker_arg_t)*T);
            pthread_barrier_t bar; pthread_barrier_init(&bar, NULL, T+1);

            // create the list and prefill
            list_global_t LG; list_hoh_t LH;
            if(impl==0){ list_global_init(&LG); fill_initial_global(&LG, init_keys, key_space); }
            else        { list_hoh_init(&LH);   fill_initial_hoh(&LH,   init_keys, key_space); }

            for(int i=0;i<T;i++){
                wa[i].impl = impl;
                wa[i].ops  = ops_per_thread;
                wa[i].key_space = key_space;
                wa[i].pct_lookup = pct_lookup;
                wa[i].pct_insert = pct_insert;
                wa[i].bar  = &bar;
                wa[i].LG   = &LG;
                wa[i].LH   = &LH;
                pthread_create(&th[i], NULL, worker, &wa[i]);
            }

            double t0=now_sec();
            pthread_barrier_wait(&bar);
            for(int i=0;i<T;i++) pthread_join(th[i], NULL);
            double t1=now_sec();

            double secs = t1-t0;
            double total_ops = (double)T * (double)ops_per_thread;
            double ops_per_sec = total_ops / secs;
            printf("%-8d %-10.6f %-12.0f\n", T, secs, ops_per_sec);

            free(th); free(wa);
            pthread_barrier_destroy(&bar);
        }
        puts("");
    }

    return 0;
}
