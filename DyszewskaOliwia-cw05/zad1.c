#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void handler(int signum) {
  printf("Odebrano sygnał\n");
}

int main(int argc, char *argv[]) {
  if (strcmp(argv[1], "none") != 0 && strcmp(argv[1], "ignore") != 0 &&
      strcmp(argv[1], "handler") != 0 && strcmp(argv[1], "mask") != 0) {
    fprintf(stderr, "Błąd: Nieprawidłowa opcja. Możliwe opcje to: none, ignore, handler, mask\n");
    exit(1);
  }

  sigset_t set;

  if (strcmp(argv[1], "ignore") == 0) {
    signal(SIGUSR1, SIG_IGN);
  } else if (strcmp(argv[1], "handler") == 0) {
    signal(SIGUSR1, handler);
  } else if (strcmp(argv[1], "mask") == 0) {
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_BLOCK, &set, NULL);
  }

  raise(SIGUSR1);

  if (strcmp(argv[1], "mask") == 0) {
    sigpending(&set);
    if (sigismember(&set, SIGUSR1)) {
      printf("SIGUSR1 jest oczekującym sygnałem\n");
    } else {
      printf("SIGUSR1 nie jest oczekującym sygnałem\n");
    }
  }

  return 0;
}