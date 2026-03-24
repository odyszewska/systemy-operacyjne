#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define MAX_CLIENTS 10
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

int client_queues[MAX_CLIENTS];

int main() {
    key_t server_key = ftok(SERVER_KEY_PATHNAME, SERVER_PROJECT_ID);
    int server_qid = msgget(server_key, IPC_CREAT | 0666);

    int client_count = 0;
    struct msgbuf msg;

    printf("Server started...\n");

    while (1) {
        if (msgrcv(server_qid, &msg, sizeof(msg) - sizeof(long), 0, 0) == -1) {
            perror("msgrcv");
            continue;
        }

        if (msg.mtype == INIT) {
            if (client_count >= MAX_CLIENTS) {
                fprintf(stderr, "Max clients reached.\n");
                continue;
            }

            int client_id = client_count++;
            client_queues[client_id] = msgget(msg.client_queue_key, 0);

            struct msgbuf reply = { .mtype = 1, .client_id = client_id };
            snprintf(reply.mtext, MAX_TEXT, "Your client ID: %d", client_id);
            msgsnd(client_queues[client_id], &reply, sizeof(reply) - sizeof(long), 0);
            printf("New client connected with ID %d\n", client_id);

        } else if (msg.mtype == MESSAGE) {
            printf("Received from client %d: %s\n", msg.client_id, msg.mtext);

            for (int i = 0; i < client_count; ++i) {
                if (i == msg.client_id) continue;
                struct msgbuf out = { .mtype = 1, .client_id = msg.client_id };
                snprintf(out.mtext, MAX_TEXT, "Client %d: ", msg.client_id);
                strncat(out.mtext, msg.mtext, MAX_TEXT - strlen(out.mtext) - 1);
                msgsnd(client_queues[i], &out, sizeof(out) - sizeof(long), 0);
            }
        }
    }

    return 0;
}
