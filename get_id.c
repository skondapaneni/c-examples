#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <assert.h>


unsigned int counter = 0;
int next_id;
unsigned int *ids = NULL;
unsigned int *ids_double = NULL;
unsigned int *ids_cur = NULL;

sem_t s1;
sem_t s2;


#define ID_CACHE_SIZE  250

char *str;

const char *
now () {
    time_t now = time(0);

    // Convert now to tm struct for local timezone
    struct tm* localtm = localtime(&now);

   str = asctime(localtm);
   str[strlen(str)-2] = '\0'; // strip the new line
   return str;
}

int
get_id() {
   // add some delay to simulate an actual id generation delay
   nanosleep((const struct timespec[]){{0, 1000000000L/50}}, NULL);
   return rand();
}

void *
id_loop (void *data) {
    int i;
    while (1) {
        sem_wait(&s1); // signalling semaphore
        sem_wait(&s2); // binary semaphore
        for (i = 0; i < ID_CACHE_SIZE; i++) {
            ids_double[i] = get_id();
        }
        sem_post(&s2);
    }
}

void
id_init() {
    int i;
    ids = malloc(sizeof(unsigned int) * ID_CACHE_SIZE);
    assert(ids != NULL);

    for (i = 0; i < ID_CACHE_SIZE; i++) {
        ids[i] = get_id();
    }

    ids_double = malloc(sizeof(unsigned int) * ID_CACHE_SIZE);
    assert(ids_double != NULL);

    for (i = 0; i < ID_CACHE_SIZE; i++) {
        ids_double[i] = get_id();
    }

    ids_cur = ids;
}

void
id_refresh() {

    if (!ids || !ids_double) {
        id_init();
        return;
    }

    sem_wait(&s2);
    if (ids_cur == ids_double) {
        ids_cur = ids;
        sem_post(&s1);
    } else {
        ids_cur = ids_double;
        ids_double = ids;
        sem_post(&s1);
    }
    sem_post(&s2);
}


int
_next_id() {
    if ((next_id = counter % ID_CACHE_SIZE) == 0) {
        id_refresh();
    }
    counter++;
    return ids_cur[next_id];
}


int main(int argc, char **argv) {
    srand(time(0));

    pthread_t thread_in_bg;
    pthread_attr_t attr;
    short use_cache = 0;

    if (argc > 1) {
        if (strcmp(argv[1], "-c") == 0) {
            use_cache = 1;
        }
    }

    typedef int (*id_func)();
    id_func id_f = get_id;
    if (use_cache) {
        id_f = _next_id;
    }

    if (sem_init(&s1, 0, 0) == -1) {
        perror("Failed to initialize semaphore s1 !!");
    }

    if (sem_init(&s2, 0, 1) == -1) {
        perror("Failed to initialize semaphore s2 !!");
    }

    pthread_attr_init(&attr);
    pthread_create(&thread_in_bg, &attr, id_loop, NULL );
    pthread_attr_destroy(&attr);

    printf("now %s\n", now());

    int i, id;



    for (i = 0; i < 10000; i++) {
        
         id = (*id_f)();
        //id = _next_id();
        //id = get_id();
        //printf("next_id[%d] = %d\n", i, id);
    }

    printf("now %s\n", now());

    pthread_cancel(thread_in_bg);

    pthread_join(thread_in_bg, NULL);

    return 0;

}

