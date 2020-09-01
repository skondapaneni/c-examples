#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/signal.h>


task_child ()
{
    int        i;
    sigset_t   set;
    int        sig;

    sigemptyset(&set);
    sigaddset(&set, SIGALARM);
    sigprocmask(SIG_SETMASK,  &set, NULL);

    for (i = 0; i < 3; i++) {
        alarm (60);
        sigwait (set, &sig);
        printf("alarm went off\n");
    }

    printf("Help, I am dying\n");

}

int
main (int argc, char **argv)
{
    uint   pid;
    int    status, i;

    if ((pid = fork()) == 0) {
       task_child ();
       _exit (1);
    }

    if (pid != 0) {
        i = 1;
        do {
            printf("Parent loop %d\n", i);
            /* status = waitpid (pid, &status, WNOHANG); */
            status = waitpid (pid, &status, 0);
            i++;
        } while (WIFEXITED (status));
        printf("Parent DONE\n");
    }
}
