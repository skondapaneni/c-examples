#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

const pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
const pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
const int MAX_WORKERS = 5;

typedef void * job_t;

/**
 * A channel struct For each worker to
 * signal when job is available.
 */
typedef struct worker_channel_st {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    job_t *job;
} worker_channel_t;

/**
 * A queue for placing the worker channel when the worker is 
 * ready to serve.
 */
typedef struct worker_pool_queue_st {
    worker_channel_t *channel;
    struct worker_pool_queue_st *next;
} worker_pool_queue_t, worker_pool_queue_element_t;

/**
 * A queue for storing the job until a worker channel becomes available.
 */
typedef struct job_queue_element_st {
    job_t *job;
    struct job_queue_element_st *next;
} job_queue_t, job_queue_element_t;

/**
 * A worker structure for saving the worker
 * related info. The worker_channel is added to
 * dispatcher's channel queue when worker is ready
 * to pick up a job
 */
typedef struct worker_st {
    pthread_t thread_id;
    int worker_num;
    worker_channel_t channel;
    short stop;
    struct dispatcher_st *dispatcher;
} worker_t;

/**
 * A dispatcher structure for saving the worker info,
 * worker channel queue and the job queue.
 * A dispatcher queues up the job in the job queue when
 * all workers are busy. When a worker becomes available,
 * job is dequeued from the job queue and given to the worker.
 */
typedef struct dispatcher_st {
    pthread_t thread_id;
    pthread_mutex_t lock;
    pthread_cond_t cond;

    struct worker_st **workers;
    worker_pool_queue_t *ready_channel_root;
    job_queue_t *job_root;
    int num_workers;
    int job_pending_cnt;
    short stop;
    short dispatcher_thread_waiting;

} dispatcher_t;

void* dispatch_thread_start(void *arg);

/**
 * func: new_dispatcher 
 *    Creates a new dispatcher instance initialized with the num workers
 * under service for this dispatcher. Also creates a thread to dispatch job to
 * workers asynchronously.
 * 
 * arg1: num_workers
 */
dispatcher_t *
new_dispatcher(int num_workers) {
    pthread_attr_t attr;
    int s;

    s = pthread_attr_init(&attr);

    if (s != 0) {
        perror("pthread_attr_init");
    }

    dispatcher_t *d = (dispatcher_t *) malloc(sizeof(dispatcher_t));
    memset(d, 0, sizeof(dispatcher_t));

    d->num_workers = num_workers;
    d->lock = lock; // initialize
    d->cond = cond; // initialize

    // A thread for checking if a worker becomes available.
    s = pthread_create(&d->thread_id, &attr, &dispatch_thread_start, d);
    return d;
}

/**
 * func: new_worker_pool_queue_element
 *
 * wraps the worker channel pointer into a worker_channel_queue element which
 * can then be added into a worker_channel_queue list.
 *
 * arg1: worker_channel_t *
 */
worker_pool_queue_t *
new_worker_pool_queue_element(worker_channel_t *channel) {
    worker_pool_queue_element_t *node =
            (worker_pool_queue_element_t *) malloc(
                    sizeof(worker_pool_queue_element_t));
    memset(node, 0, sizeof(worker_pool_queue_element_t));
    node->channel = channel;
    return node;
}

/**
 * func: add_channel
 *
 * Add's the worker channel into a worker channel queue
 * 
 * arg1: worker_pool_queue_t **root
 * arg2: worker_channel_t *channel
 */
void add_channel(worker_pool_queue_t **root, worker_channel_t *channel) {
    worker_pool_queue_element_t *prev;

    if (root == NULL)
        return;
    worker_pool_queue_element_t *node = new_worker_pool_queue_element(channel);

    if (node != NULL) {
        if (*root == NULL) {
            *root = node;
            return;
        } else {
            for (prev = *root; prev->next != NULL; prev = prev->next) {
                ;
            }
            prev->next = node;
            return;
        }
    }
}

/**
 * func: remove_channel
 *
 * Remove's the matching worker channel from the worker channel queue
 * 
 * arg1: worker_pool_queue_t **root
 * arg2: worker_channel_t *channel
 */
void remove_channel(worker_pool_queue_t **root, worker_channel_t *channel) {
    worker_pool_queue_element_t *prev = NULL, *cur = NULL;

    if (root == NULL || *root == NULL)
        return;
    for (cur = *root; cur != NULL; cur = cur->next) {
        if (cur->channel == channel) {
            if (cur == *root) {
                *root = cur->next;
            } else {
                prev->next = cur->next;
            }
            return;
        }
        prev = cur;
    }
    return;
}

/**
 * func: new_job_element
 *
 * wraps the job pointer into a job_queue element which
 * can then be added into a job queue list.
 *
 * arg1: worker_channel_t *
 */
job_queue_t *new_job_element(job_t *job) {
    job_queue_t *node = (job_queue_t *) malloc(sizeof(job_queue_t));
    memset(node, 0, sizeof(job_queue_t));
    node->job = job;
    return node;
}

/**
 * func: add_job
 *
 * Add's the job into a job queue
 * 
 * arg1: job_queue_t **root
 * arg2: job_t *job
 */
void add_job(job_queue_t **root, job_t *job) {
    job_queue_t *prev;

    if (root == NULL) return;
    job_queue_element_t *node = new_job_element(job);
    if (node != NULL) {
        if (*root == NULL) {
            *root = node;
            return;
        } else {
            for (prev = *root; prev->next != NULL; prev = prev->next) {
                ;
            }
            prev->next = node;
            return;
        }
    }
}

/**
 * func: remove_job
 *
 * Remove's the matching job from the job queue
 * 
 * arg1: job_queue_t **root
 * arg2: job_t *job
 */
void remove_job(job_queue_t **root, job_t *job) {
    job_queue_element_t *prev = NULL, *cur = NULL;

    if (root == NULL || *root == NULL)
        return;
    for (cur = *root; cur != NULL; cur = cur->next) {
        if (cur->job == job) {
            if (cur == *root) {
                *root = cur->next;
            } else {
                prev->next = cur->next;
            }
            return;
        }
        prev = cur;
    }
    return;
}

/**
 * func: new_worker
 *
 * Creates a new worker object and links it into the dispatcher workers list.
 *
 * arg1: dispatcher_t *
 * arg2: int (worker id)
 */
worker_t *new_worker(dispatcher_t *d, int num) {

    worker_t *worker = (worker_t *) malloc(sizeof(worker_t));
    memset(worker, 0, sizeof(worker_t));
    worker->worker_num = num;
    worker->channel.lock = lock;
    worker->channel.cond = cond;
    worker->dispatcher = d;
    return worker;
}

/**
 * worker_signal_channel_ready
 *
 * Adds the worker channel into the dispatchers queue
 * of available workers
 *
 */
void worker_signal_channel_ready(dispatcher_t *d, 
                    worker_channel_t *worker_channel) {
    pthread_mutex_lock(&d->lock);
    if (d->stop) {
        pthread_mutex_unlock(&d->lock);
        return;
    }
    add_channel(&d->ready_channel_root, worker_channel);
    if (d->dispatcher_thread_waiting) {
        pthread_cond_signal(&d->cond);
    }
    pthread_mutex_unlock(&d->lock);
}

/**
 * func: worker_thread_start
 *
 * thread start method, which starts the worker thread. 
 * Add's the worker channel to the dispatchers channel list, 
 * and waits for the job to be dispatched by the dispatcher. 
 * Once done with the task it will again add it's worker channel back 
 * into the dispatcher channel list.
 *
 * arg1: void * (worker_t *)
 */
void* worker_thread_start(void *arg) {
    worker_t *worker = (worker_t *) arg;
    for (;;) {
        pthread_mutex_lock(&worker->channel.lock);
        worker_signal_channel_ready(worker->dispatcher, &worker->channel);
        if (worker->stop) {
            pthread_mutex_unlock(&worker->channel.lock);
            break;
        }
        printf("worker %d is ready\n", worker->worker_num);
        pthread_cond_wait(&worker->channel.cond, &worker->channel.lock);
        if (worker->stop) {
            printf("worker %d stopping...\n", worker->worker_num);
            pthread_mutex_unlock(&worker->channel.lock);
            break;
        }
        printf("worker %d got the job %s\n", worker->worker_num,
                (char *) worker->channel.job);
        pthread_mutex_unlock(&worker->channel.lock);
        sleep(5);
    }
    return NULL;
}

/**
 * func: create_workers
 *
 * create the workers for the dispatcher to use.
 * 
 * arg1: dispatcher_t *
 */
void create_workers(dispatcher_t *d) {
    d->workers = (worker_t **) malloc(sizeof(worker_t *) * d->num_workers);
    for (int i = 0; i < d->num_workers; i++) {
        d->workers[i] = new_worker(d, i + 1);
    }
}

/**
 * worker_pool_start
 *
 * create the workers and start the worker thread pool.
 *
 * arg1: dispatcher_t *
 */
void worker_pool_start(dispatcher_t *d) {
    int s;
    pthread_attr_t attr;
    s = pthread_attr_init(&attr);

    if (s != 0) {
        perror("pthread_attr_init");
    }

    create_workers(d);

    for (int i = 0; i < d->num_workers; i++) {
        s = pthread_create(&d->workers[i]->thread_id, &attr,
                &worker_thread_start, d->workers[i]);
    }
}

/**
 * worker_pool_stop
 *
 * Called to clear all jobs and stop all
 * the workers
 */
void worker_pool_stop(dispatcher_t *d) {

    pthread_mutex_lock(&d->lock);
    d->stop = 1;
    while (1) {
        job_queue_t *cur = d->job_root;
        if (cur == NULL) {
            break;
        }
        d->job_pending_cnt--;
        remove_job(&d->job_root, cur->job);
        free(cur);
    }
    pthread_mutex_unlock(&d->lock);

    for (int i = 0; i < d->num_workers; i++) {
        pthread_mutex_lock(&d->workers[i]->channel.lock);
        d->workers[i]->stop = 1;
        pthread_cond_signal(&d->workers[i]->channel.cond);
        pthread_mutex_unlock(&d->workers[i]->channel.lock);
        pthread_join(d->workers[i]->thread_id, NULL);
    }
}

/**
 * dispatch_job
 *
 * dispatch the job to the next available worker. 
 * if no worker is available, the job is queued and dispatched
 * asynchronously from a dispatch thread when the worker 
 * becomes available.
 *
 * arg1: dispatcher_t *
 * arg2: job_t *
 */
short dispatch_job(dispatcher_t *d, job_t *job) {
    worker_pool_queue_t *worker_channel_top;
    pthread_mutex_lock(&d->lock);
    worker_channel_top = d->ready_channel_root;
    if (worker_channel_top == NULL) { // no free worker channels
        d->job_pending_cnt++;
        add_job(&d->job_root, job); // queue up the job
        pthread_mutex_unlock(&d->lock);
        return 0;
    }

    remove_channel(&d->ready_channel_root, worker_channel_top->channel);
    pthread_mutex_unlock(&d->lock);

    worker_channel_top->channel->job = job;
    pthread_mutex_lock(&worker_channel_top->channel->lock);
    pthread_cond_signal(&worker_channel_top->channel->cond);
    pthread_mutex_unlock(&worker_channel_top->channel->lock);
    free(worker_channel_top);
    return 1;
}

/**
 * func: dispatch_all_pending_jobs
 *
 * internal function to dispatch all jobs in job queue to available
 * workers. Break out of the loop if a worker is not available,
 * or dispatcher is stopped or when no more pending jobs found.
 *
 */
void dispatch_all_pending_jobs(dispatcher_t *d) {
    while (1) {
        pthread_mutex_lock(&d->lock);
        job_queue_t *cur = d->job_root;
        if (cur == NULL) {
            pthread_mutex_unlock(&d->lock);
            break;
        }
 
        if (d->stop) {
            pthread_mutex_unlock(&d->lock);
            break; 
        }
 
        if (d->ready_channel_root == NULL) {
            pthread_mutex_unlock(&d->lock);
            break;
        }
 
        d->job_pending_cnt--;
        remove_job(&d->job_root, cur->job);
        pthread_mutex_unlock(&d->lock);
 
        if (!dispatch_job(d, cur->job)) {
            free(cur);
            break;
        }
        free(cur);
    }
}
  
/**
 * func: dispatch_thread_start
 *
 * async thread for the dispatcher to dispatch pending jobs to the
 * the available workers.
 */
void* dispatch_thread_start(void *arg) {
    dispatcher_t *d = (dispatcher_t *) arg;
    for (;;) {
        pthread_mutex_lock(&d->lock);
        if (d->stop) {
            pthread_mutex_unlock(&d->lock);
            break; // exit the thread
        }

        if (d->job_pending_cnt == 0 || d->ready_channel_root == NULL) {
            d->dispatcher_thread_waiting = 1;
            pthread_cond_wait(&d->cond, &d->lock);
            d->dispatcher_thread_waiting = 0;

            if (d->stop) {
                pthread_mutex_unlock(&d->lock);
                break; // exit the thread
            }

            if (d->job_pending_cnt == 0 || d->ready_channel_root == NULL) {
                pthread_mutex_unlock(&d->lock);
                continue;
            }
        }
        pthread_mutex_unlock(&d->lock);
        dispatch_all_pending_jobs(d);

    }
    return NULL;
}


int main(int argc, char **argv) {
    char *j1 = "one";
    char *j2 = "two";
    char *j3 = "three";
    char *j4 = "four";
    char *j5 = "five";
    char *j6 = "six";

    dispatcher_t *d = new_dispatcher(MAX_WORKERS);
    dispatch_job(d, (job_t *) j1);
    dispatch_job(d, (job_t *) j2);

    worker_pool_start(d);

    dispatch_job(d, (job_t *) j3);
    dispatch_job(d, (job_t *) j4);
    dispatch_job(d, (job_t *) j5);
    dispatch_job(d, (job_t *) j6);

    printf("sleeping for 100 seconds \n");

    sleep(100);

    printf("dispatcher stopping\n");

    worker_pool_stop(d);

    return 0;
}
