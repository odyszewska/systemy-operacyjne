#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>


#define MAX_NICK  32
#define MAX_MSG   512

static int sockfd = -1;
static volatile sig_atomic_t running = 1;
static char nick[MAX_NICK];

static void perror_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void *rx_thread(void *arg) {
    (void)arg;
    char buf[MAX_MSG];
    while (running) {
        ssize_t n = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            printf("[client] server closed connection\n");
            running = 0;
            break;
        }
        buf[n] = '\0';
        if (strncmp(buf, "PING", 4) == 0) {
            send(sockfd, "ALIVE\n", 6, 0);
        } else {
            printf("%s", buf);
            fflush(stdout);
        }
    }
    return NULL;
}

static void on_sigint(int sig) {
    (void)sig;
    running = 0;
    if (sockfd != -1) send(sockfd, "STOP\n", 5, 0);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <nick> <server_ip> <server_port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    strncpy(nick, argv[1], MAX_NICK - 1);
    nick[MAX_NICK - 1] = '\0';
    const char *ip = argv[2];
    int port = atoi(argv[3]);

    signal(SIGINT, on_sigint);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) perror_exit("socket");

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) perror_exit("inet_pton");

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        perror_exit("connect");

    char hello[MAX_MSG];
    snprintf(hello, sizeof(hello), "HELLO %s\n", nick);
    send(sockfd, hello, strlen(hello), 0);

    pthread_t tid;
    pthread_create(&tid, NULL, rx_thread, NULL);

    char line[MAX_MSG];
    while (running && fgets(line, sizeof(line), stdin)) {
        if (!running) break;
        send(sockfd, line, strlen(line), 0);
        if (strncmp(line, "STOP", 4) == 0) {
            running = 0;
            break;
        }
    }

    pthread_join(tid, NULL);
    close(sockfd);
    return 0;
}
