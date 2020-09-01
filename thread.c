#include <pthread.h>
#include <thread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <synch.h>

sigset_t       signalSet;

void * sigint(void *arg);
static void * threadTwo(void *arg);
static void * threadThree(void *arg);


void * 
main(int argc, char **argv)
{
    pthread_t    t;
    pthread_t    t2;
    pthread_t    t3;

    thr_setconcurrency(3);
    sigfillset ( &signalSet );

    /*
     * Block signals in initial thread. New threads will
     * inherit this signal mask.
     */
    pthread_sigmask ( SIG_BLOCK, &signalSet, NULL );

    printf("Creating threads\n");

    /* 
     * POSIX thread create arguments:
     * thr_id, attr, strt_func, arg
     */

    pthread_create(&t, NULL, sigint, NULL);
    pthread_create(&t2, NULL, threadTwo, NULL);
    pthread_create(&t3, NULL, threadThree, NULL);

    printf("##################\n");
    printf("press CTRL-C to deliver SIGINT to sigint thread\n");
    printf("##################\n");

    thr_exit((void *)0);
}

static void *
threadTwo(void *arg)
{
    printf("hello world, from threadTwo [tid: %d]\n",
                                 pthread_self());
    printf("threadTwo [tid: %d} is now complete and exiting\n",
                                 pthread_self());
    thr_exit((void *)0);
}

static void *
threadThree(void *arg)
{
    printf("hello world, from threadThree [tid: %d]\n",
                                 pthread_self());
    printf("threadThree [tid: %d} is now complete and exiting\n",
                                 pthread_self());
    thr_exit((void *)0);
}

void *
sigint(void *arg)
{
    int    sig;
    int    err;

    printf("thread sigint [tid: %d] awaiting SIGINT\n",
                                 pthread_self());

    /* use POSIX sigwait() -- 2 args
     * signal set, signum
     */
    err = sigwait ( &signalSet, &sig );

    /* test for SIGINT; could catch other signals */
    if (err || sig != SIGINT)
        abort();

    printf("\nSIGINT signal %d caught by sigint thread [tid: %d]\n",
                                 sig, pthread_self());
    thr_exit((void *)0);
}
