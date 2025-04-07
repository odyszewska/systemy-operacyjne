#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int global = 0;

int main(int argc, char *argv[]) {
    printf("Nazwa programu: %s\n", argv[0]);
    int local = 0;

    pid_t pid = fork();
    if (pid == 0) {
      printf("child process\n");

      global ++;
      local ++;

      printf("child pid = %d, parent pid = %d\n", getpid(), getppid());
      printf("child's local = %d, child's global = %d\n", local, global);

      execl("/bin/ls", "ls", argv[1], NULL);
      exit(1);
    } else {
      int status;
      waitpid(pid, &status, 0);

      printf("parent process\n");
      printf("parent pid = %d, child pid = %d\n", getpid(), pid);
      printf("Child exit code: %d\n", WEXITSTATUS(status));
      printf("Parent's local = %d, parent's global = %d\n", local, global);
    }


    exit(0);
}