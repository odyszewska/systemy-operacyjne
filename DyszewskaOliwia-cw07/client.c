#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_TEXT 256
#define SERVER_KEY_PATHNAME "/tmp"
#define SERVER_PROJECT_ID 'S'

typedef enum { INIT = 1, MESSAGE } msg_type;

struct msgbuf {
    long mtype;
    int client_id;
    key_t client_queue_key;
    char mtext[MAX_TEXT];
};

int client_qid;

void receive_messages() {
    struct msgbuf msg;
    while (1) {
        if (msgrcv(client_qid, &msg, sizeof(msg) - sizeof(long), 0, 0) > 0) {
            printf("%s\n", msg.mtext);
        }
    }
}

int main() {
    key_t server_key = ftok(SERVER_KEY_PATHNAME, SERVER_PROJECT_ID);
    int server_qid = msgget(server_key, 0);

    key_t client_key = ftok(".", getpid() % 255 + 1);
    client_qid = msgget(client_key, IPC_CREAT | 0666);

    struct msgbuf init_msg = { .mtype = INIT, .client_queue_key = client_key };
    msgsnd(server_qid, &init_msg, sizeof(init_msg) - sizeof(long), 0);

    struct msgbuf response;
    msgrcv(client_qid, &response, sizeof(response) - sizeof(long), 0, 0);
    int client_id = response.client_id;
    printf("Connected. Assigned ID: %d\n", client_id);

    if (fork() == 0) {
        receive_messages();
        exit(0);
    }

    char text[MAX_TEXT];
    struct msgbuf msg;
    msg.mtype = MESSAGE;
    msg.client_id = client_id;

    while (fgets(text, MAX_TEXT, stdin)) {
        text[strcspn(text, "\n")] = 0;
        strncpy(msg.mtext, text, MAX_TEXT);
        msgsnd(server_qid, &msg, sizeof(msg) - sizeof(long), 0);
    }

    msgctl(client_qid, IPC_RMID, NULL);
    return 0;
}
