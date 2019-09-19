#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <time.h> 
#include <unistd.h>
#include <string.h>
#include <assert.h>


// simulate mutex locking with atomic operations.

typedef struct lock_resource_st {
    int counter;
    void *owner_ctxt;
} lock_resource_t;

typedef struct thread_ctxt_st {
   lock_resource_t *lock_p;
   int              thread_num;
   pthread_t        thread_id;
} thread_ctxt_t;

sig_atomic_t interrupted;
lock_resource_t resource;

int 
mutex_lock (lock_resource_t *res_p, void *owner_ctxt) {
    int idx, idx_next;
    if (owner_ctxt == res_p->owner_ctxt) {
        return -1;
    }
    do {
       idx = 0; 
       idx_next = 1; 
    } while (__sync_bool_compare_and_swap(&res_p->counter, idx, idx_next) != 1);
    res_p->owner_ctxt = owner_ctxt;
    return 0;
}

int 
mutex_unlock (lock_resource_t *res_p, void *owner_ctxt) {
    int idx, idx_next;
    if (owner_ctxt != res_p->owner_ctxt) {
        return -1;
    }
    res_p->owner_ctxt = NULL;
    do {
       idx = 1; 
       idx_next = 0; 
    } while (__sync_bool_compare_and_swap(&res_p->counter, idx, idx_next) != 1);
    return 0;
}

void *
thread_loop (void *data) {
    thread_ctxt_t *ctxt_p = (thread_ctxt_t *) data;
   
    while (!interrupted) {
       mutex_lock(ctxt_p->lock_p, ctxt_p->thread_id);
       printf("%d got the lock\n", ctxt_p->thread_num);
       //sleep(rand() % 5);
       sleep(1);
       mutex_unlock(ctxt_p->lock_p, ctxt_p->thread_id);
       printf("%d unlocked \n", ctxt_p->thread_num);
    }
    return NULL;
}

void 
init_threads (int n, thread_ctxt_t *ctxt_p) {

    pthread_t       t;
    pthread_attr_t  attr;

    pthread_attr_init(&attr);
    for (int i = 0; i < n; i++) {
        ctxt_p[i].lock_p = &resource;
        ctxt_p[i].thread_num = i;
        pthread_create(&ctxt_p[i].thread_id, &attr, thread_loop, &ctxt_p[i] );
    }
}


static void 
sighandler(int num) {
    interrupted = 1;
}

int main(int argc, char **argv) {
    thread_ctxt_t   *ctxt_p;
    void            *res;
    int             n = 5;

    signal(SIGINT, sighandler);
    memset(&resource, 0, sizeof(resource));
    ctxt_p = (thread_ctxt_t *) calloc(n, sizeof(thread_ctxt_t));
    assert(ctxt_p != NULL);
    srand(time(0)); 
    init_threads(n, ctxt_p);

    for (int i = 0; i < n; i++) {
        pthread_join(ctxt_p[i].thread_id, &res);
    }
}
