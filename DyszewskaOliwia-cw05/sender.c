#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

volatile sig_atomic_t received = 0;

void handler(int sig) {
    received = 1;
}

int main(int argc, char *argv[]) {
    pid_t catcher_pid = atoi(argv[1]);
    int mode = atoi(argv[2]);

    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    union sigval val;
    val.sival_int = mode;
    sigqueue(catcher_pid, SIGUSR1, val);

    sigset_t mask;
    sigemptyset(&mask);
    while (!received) {
        sigsuspend(&mask);
    }

    printf("Sender: otrzymano potwierdzenie od catcher.\n");
    return 0;
}