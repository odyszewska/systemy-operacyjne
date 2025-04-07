#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    int n = atoi(argv[1]);

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            printf("%d %d\n", getppid(), getpid());
            return 0;
        }
    }

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    printf("%d\n", n);
    return 0;
}
