#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>

#define MAX_CLIENTS    16
#define MAX_NICK       32
#define MAX_MSG        1024
#define PING_INTERVAL  10
#define PING_TIMEOUT   30
#define MAX_FRAME      (MAX_MSG + 64)

struct client {
    struct sockaddr_in addr;
    socklen_t addrlen;
    char nick[MAX_NICK];
    time_t last_alive;
    bool active;
};

static struct client clients[MAX_CLIENTS];
static volatile sig_atomic_t running = 1;

static void on_sigint(int sig) {
    (void)sig;
    running = 0;
}

static void perror_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static int add_client(const char *nick,
                      struct sockaddr_in *addr, socklen_t addrlen) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i].active) {
            clients[i].active     = true;
            clients[i].addr       = *addr;
            clients[i].addrlen    = addrlen;
            strncpy(clients[i].nick, nick, MAX_NICK-1);
            clients[i].nick[MAX_NICK-1] = '\0';
            clients[i].last_alive = time(NULL);
            return i;
        }
    }
    return -1;
}

static int find_client_by_nick(const char *nick) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active && strcmp(clients[i].nick, nick) == 0)
            return i;
    }
    return -1;
}

static int find_client_by_addr(struct sockaddr_in *addr, socklen_t addrlen) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active
         && clients[i].addrlen == addrlen
         && clients[i].addr.sin_port == addr->sin_port
         && clients[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr)
            return i;
    }
    return -1;
}

static void remove_client(int idx) {
    clients[idx].active = false;
}

static void sendto_addr(int sock,
                        struct sockaddr_in *addr, socklen_t len,
                        const char *fmt, ...) {
    char buf[MAX_FRAME];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    sendto(sock, buf, strlen(buf), 0,
           (struct sockaddr *)addr, len);
}

static void broadcast_all(int sock,
                          const char *sender, const char *body) {
    char frame[MAX_FRAME];
    char timestr[32];
    time_t now = time(NULL);
    strftime(timestr, sizeof(timestr), "%F %T", localtime(&now));

    int n = snprintf(frame, sizeof(frame),
                     "FROM %s %s %s\n",
                     sender, timestr, body);
    if (n < 0) return;

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].active
         && strcmp(clients[i].nick, sender) != 0) {
            sendto(sock, frame, n, 0,
                   (struct sockaddr *)&clients[i].addr,
                   clients[i].addrlen);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int port = atoi(argv[1]);
    memset(clients, 0, sizeof(clients));

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) perror_exit("socket");

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        perror_exit("bind");

    signal(SIGINT, on_sigint);
    printf("[server] UDP listening on port %d …\n", port);

    time_t last_ping = time(NULL);
    while (running) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        struct timeval tv = {1,0};
        int sel = select(sock+1, &rfds, NULL, NULL, &tv);
        if (sel < 0) {
            if (errno == EINTR) continue;
            perror_exit("select");
        }

        if (FD_ISSET(sock, &rfds)) {
            char buf[MAX_MSG+1] = {0};
            struct sockaddr_in src;
            socklen_t srclen = sizeof(src);
            ssize_t n = recvfrom(sock, buf, MAX_MSG, 0,
                                 (struct sockaddr *)&src, &srclen);
            if (n <= 0) continue;
            buf[n] = '\0';

            int idx = find_client_by_addr(&src, srclen);
            if (strncmp(buf, "HELLO ", 6) == 0) {
                char nick[MAX_NICK];
                sscanf(buf + 6, "%31s", nick);
                if (find_client_by_nick(nick) != -1
                 || add_client(nick, &src, srclen) == -1) {
                    sendto_addr(sock, &src, srclen,
                                "ERROR nickname_taken_or_server_full\n");
                } else {
                    printf("[server] %s joined\n", nick);
                }
                continue;
            }
            if (idx < 0) continue;

            if (strncmp(buf, "ALIVE", 5) == 0) {
                clients[idx].last_alive = time(NULL);
                continue;
            }
            if (strncmp(buf, "LIST", 4) == 0) {
                sendto_addr(sock, &src, srclen, "USERS ");
                for (int k = 0; k < MAX_CLIENTS; ++k)
                    if (clients[k].active)
                        sendto_addr(sock, &src, srclen,
                                    "%s ", clients[k].nick);
                sendto_addr(sock, &src, srclen, "\n");
                continue;
            }
            if (strncmp(buf, "2ALL ", 5) == 0) {
                broadcast_all(sock, clients[idx].nick, buf + 5);
                continue;
            }
            if (strncmp(buf, "2ONE ", 5) == 0) {
                char target[MAX_NICK];
                char *msg = strchr(buf + 5, ' ');
                if (!msg) continue;
                *msg = '\0';
                strcpy(target, buf + 5);
                msg++;
                int t = find_client_by_nick(target);
                if (t != -1) {
                    char frame[MAX_FRAME];
                    char timestr[32];
                    time_t now = time(NULL);
                    strftime(timestr, sizeof(timestr),
                             "%F %T", localtime(&now));
                    int len = snprintf(frame, sizeof(frame),
                                       "FROM %s %s %s\n",
                                       clients[idx].nick,
                                       timestr, msg);
                    if (len > 0) {
                        sendto(sock, frame, len, 0,
                               (struct sockaddr *)&clients[t].addr,
                               clients[t].addrlen);
                    }
                } else {
                    sendto_addr(sock, &src, srclen,
                                "ERROR no_such_user\n");
                }
                continue;
            }
            if (strncmp(buf, "STOP", 4) == 0) {
                printf("[server] %s requested STOP\n",
                       clients[idx].nick);
                remove_client(idx);
                continue;
            }
        }

        time_t now = time(NULL);
        if (now - last_ping >= PING_INTERVAL) {
            last_ping = now;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (!clients[i].active) continue;
                if (now - clients[i].last_alive > PING_TIMEOUT) {
                    printf("[server] %s timed out\n",
                           clients[i].nick);
                    remove_client(i);
                } else {
                    sendto(sock, "PING\n", 5, 0,
                           (struct sockaddr *)&clients[i].addr,
                           clients[i].addrlen);
                }
            }
        }
    }

    printf("[server] shutting down…\n");
    close(sock);
    return 0;
}
