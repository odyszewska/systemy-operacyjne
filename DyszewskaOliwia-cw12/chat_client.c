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
#include <unistd.h>

#define MAX_NICK  32
#define MAX_MSG   512

static int sockfd = -1;
static volatile sig_atomic_t running = 1;
static char nick[MAX_NICK];
static struct sockaddr_in server_addr;

static void perror_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void *rx_thread(void *arg) {
    (void)arg;
    char buf[MAX_MSG + 1];
    struct sockaddr_in src;
    socklen_t srclen = sizeof(src);

    while (running) {
        ssize_t n = recvfrom(sockfd, buf, MAX_MSG, 0,
                             (struct sockaddr *)&src, &srclen);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recvfrom");
            break;
        }
        buf[n] = '\0';
        if (strncmp(buf, "PING", 4) == 0) {
            const char *alive = "ALIVE\n";
            sendto(sockfd, alive, strlen(alive), 0,
                   (struct sockaddr *)&server_addr, sizeof(server_addr));
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
    const char *stop = "STOP\n";
    sendto(sockfd, stop, strlen(stop), 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <nick> <server_ip> <server_port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    strncpy(nick, argv[1], MAX_NICK-1);
    nick[MAX_NICK-1] = '\0';
    const char *ip = argv[2];
    int port = atoi(argv[3]);

    signal(SIGINT, on_sigint);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) perror_exit("socket");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) != 1)
        perror_exit("inet_pton");

    char hello[MAX_MSG];
    snprintf(hello, sizeof(hello), "HELLO %s\n", nick);
    sendto(sockfd, hello, strlen(hello), 0,
           (struct sockaddr *)&server_addr, sizeof(server_addr));

    pthread_t tid;
    if (pthread_create(&tid, NULL, rx_thread, NULL) != 0)
        perror_exit("pthread_create");

    char line[MAX_MSG];
    while (running && fgets(line, sizeof(line), stdin)) {
        sendto(sockfd, line, strlen(line), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (strncmp(line, "STOP", 4) == 0) {
            running = 0;
            break;
        }
    }

    pthread_join(tid, NULL);
    close(sockfd);
    return 0;
}
