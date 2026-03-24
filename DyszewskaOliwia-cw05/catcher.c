#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdatomic.h>

volatile sig_atomic_t current_mode = 0;
volatile sig_atomic_t request_count = 0;

void ctrl_c_handler(int signo) {
    printf("\nWciśnięto CTRL+C\n");
}

void handle_sigusr1(int sig, siginfo_t *info, void *context) {
    current_mode = info->si_value.sival_int;
    request_count++;

    printf("Catcher: odebrano tryb %d od PID %d\n", current_mode, info->si_pid);

    if (current_mode == 1) {
        printf("Liczba żądań zmiany trybu: %d\n", request_count);
    } else if (current_mode == 3) {
        signal(SIGINT, SIG_IGN);
        printf("Ustawiono ignorowanie Ctrl+C\n");
    } else if (current_mode == 4) {
        signal(SIGINT, ctrl_c_handler);
        printf("Ustawiono wypisywanie tekstu po Ctrl+C\n");
    } else if (current_mode == 5) {
        printf("Zakończenie działania catcher\n");
        union sigval val;
        sigqueue(info->si_pid, SIGUSR1, val);
        exit(0);
    }

    union sigval val;
    sigqueue(info->si_pid, SIGUSR1, val);
}

int main() {
    printf("Catcher PID: %d\n", getpid());

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    sigset_t mask;
    sigemptyset(&mask);

    while (1) {
        if (current_mode == 2) {
            for (int i = 1; current_mode == 2; i++) {
                printf("%d\n", i);
                sleep(1);
            }
        } else {
            pause();
        }
    }

    return 0;
}
