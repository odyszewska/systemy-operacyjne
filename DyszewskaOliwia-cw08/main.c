#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#define MAX_QUEUE 10
#define TASK_SIZE 10

#define SEM_MUTEX 0
#define SEM_FULL  1
#define SEM_EMPTY 2

typedef struct {
    char task_queue[MAX_QUEUE][TASK_SIZE + 1];
    int front, rear;
} SharedQueue;

int running = 1;

void handle_sigint(int sig) {
    running = 0;
}

void sem_op(int semid, int sem_num, int op) {
    struct sembuf sop = { .sem_num = sem_num, .sem_op = op, .sem_flg = 0 };
    if (semop(semid, &sop, 1) == -1) {
        perror("semop");
        exit(1);
    }
}

void setup() {
    int fd = open("shmfile", O_CREAT|O_RDONLY, 0666);
    if (fd == -1) { perror("open shmfile"); exit(1); }
    close(fd);
    fd = open("semfile", O_CREAT|O_RDONLY, 0666);
    if (fd == -1) { perror("open semfile"); exit(1); }
    close(fd);

    key_t shm_key = ftok("shmfile", 65);
    key_t sem_key = ftok("semfile", 75);

    int shmid = shmget(shm_key, sizeof(SharedQueue), 0666 | IPC_CREAT);
    if (shmid == -1) { perror("shmget"); exit(1); }
    SharedQueue *queue = shmat(shmid, NULL, 0);
    if (queue == (void*)-1) { perror("shmat"); exit(1); }
    queue->front = 0;
    queue->rear  = 0;
    shmdt(queue);

    int semid = semget(sem_key, 3, 0666 | IPC_CREAT);
    if (semid == -1) { perror("semget"); exit(1); }
    union semun { int val; } arg;
    arg.val = 1;            semctl(semid, SEM_MUTEX, SETVAL, arg);
    arg.val = 0;            semctl(semid, SEM_FULL,  SETVAL, arg);
    arg.val = MAX_QUEUE;    semctl(semid, SEM_EMPTY, SETVAL, arg);

    printf("Setup completed.\n");
}

void cleanup() {
    key_t shm_key = ftok("shmfile", 65);
    key_t sem_key = ftok("semfile", 75);

    int shmid = shmget(shm_key, sizeof(SharedQueue), 0666);
    if (shmid != -1) shmctl(shmid, IPC_RMID, NULL);

    int semid = semget(sem_key, 3, 0666);
    if (semid != -1) semctl(semid, 0, IPC_RMID);

    printf("Cleaned up.\n");
}

char *generate_task() {
    static char task[TASK_SIZE + 1];
    for (int i = 0; i < TASK_SIZE; i++)
        task[i] = 'a' + rand() % 26;
    task[TASK_SIZE] = '\0';
    return task;
}

void user_process() {
    srand(time(NULL) ^ getpid());
    signal(SIGINT, handle_sigint);

    key_t shm_key = ftok("shmfile", 65);
    key_t sem_key = ftok("semfile", 75);

    int shmid = shmget(shm_key, sizeof(SharedQueue), 0666);
    SharedQueue *queue = shmat(shmid, NULL, 0);
    int semid = semget(sem_key, 3, 0666);

    while (running) {
        char *task = generate_task();

        sem_op(semid, SEM_EMPTY, -1);
        sem_op(semid, SEM_MUTEX, -1);

        strncpy(queue->task_queue[queue->rear], task, TASK_SIZE+1);
        queue->rear = (queue->rear + 1) % MAX_QUEUE;

        sem_op(semid, SEM_MUTEX, 1);
        sem_op(semid, SEM_FULL, 1);

        printf("Użytkownik %d wysłał: %s\n", getpid(), task);
        sleep(rand() % 3 + 1);
    }

    shmdt(queue);
    printf("Użytkownik %d kończy.\n", getpid());
    exit(0);
}

void printer_process() {
    signal(SIGINT, handle_sigint);

    key_t shm_key = ftok("shmfile", 65);
    key_t sem_key = ftok("semfile", 75);

    int shmid = shmget(shm_key, sizeof(SharedQueue), 0666);
    SharedQueue *queue = shmat(shmid, NULL, 0);
    int semid = semget(sem_key, 3, 0666);

    while (running) {
        sem_op(semid, SEM_FULL,  -1);
        sem_op(semid, SEM_MUTEX, -1);

        char task[TASK_SIZE+1];
        strncpy(task, queue->task_queue[queue->front], TASK_SIZE+1);
        queue->front = (queue->front + 1) % MAX_QUEUE;

        sem_op(semid, SEM_MUTEX, 1);
        sem_op(semid, SEM_EMPTY, 1);

        printf("Drukarka %d drukuje: ", getpid());
        fflush(stdout);
        for (int i = 0; i < TASK_SIZE; i++) {
            putchar(task[i]);
            fflush(stdout);
            sleep(1);
        }
        putchar('\n');
    }

    shmdt(queue);
    printf("Drukarka %d kończy.\n", getpid());
    exit(0);
}

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);

    if (argc < 2) {
        fprintf(stderr, "Użycie: %s [setup|cleanup|run N M]\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "setup") == 0) {
        setup();

    } else if (strcmp(argv[1], "cleanup") == 0) {
        cleanup();

    } else if (strcmp(argv[1], "run") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Użycie: %s run <N_użytkowników> <M_drukarek>\n", argv[0]);
            exit(1);
        }
        int N = atoi(argv[2]);
        int M = atoi(argv[3]);

        setup();
        signal(SIGINT, handle_sigint);

        pid_t *kids = malloc(sizeof(pid_t) * (N + M));
        int idx = 0;
        for (int i = 0; i < N; i++) {
            pid_t pid = fork();
            if (pid == 0) user_process();
            kids[idx++] = pid;
        }
        for (int i = 0; i < M; i++) {
            pid_t pid = fork();
            if (pid == 0) printer_process();
            kids[idx++] = pid;
        }

        while (running) sleep(1);

        for (int i = 0; i < idx; i++) kill(kids[i], SIGINT);
        for (int i = 0; i < idx; i++) wait(NULL);

        cleanup();
        free(kids);

    } else {
        fprintf(stderr, "Nieznany tryb: %s\n", argv[1]);
        exit(1);
    }

    return 0;
}
