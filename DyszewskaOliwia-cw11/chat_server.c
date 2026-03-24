#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>


#define MAX_CLIENTS 16
#define MAX_NICK    32
#define MAX_MSG     1024
#define PING_INTERVAL 10
#define PING_TIMEOUT  30
#define MAX_FRAME (MAX_MSG + 64)

struct client {
    int   fd;
    char  nick[MAX_NICK];
    time_t last_alive;
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

static int add_client(int fd, const char *nick) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].fd == -1) {
            clients[i].fd = fd;
            strncpy(clients[i].nick, nick, MAX_NICK - 1);
            clients[i].nick[MAX_NICK - 1] = '\0';
            clients[i].last_alive = time(NULL);
            return 0;
        }
    }
    return -1;
}

static void remove_client(int idx) {
    close(clients[idx].fd);
    clients[idx].fd = -1;
    clients[idx].nick[0] = '\0';
}

static void send_line(int fd, const char *fmt, ...) {
    char buf[MAX_MSG];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    send(fd, buf, strlen(buf), 0);
}

static void broadcast(const char *sender, const char *body)
{
    char frame[MAX_FRAME];
    char timestr[32];

    time_t now = time(NULL);
    strftime(timestr, sizeof(timestr), "%F %T", localtime(&now));

    int n = snprintf(frame, sizeof(frame), "FROM %s %s ", sender, timestr);
    if (n < 0 || n >= (int)sizeof(frame))
        return;

    snprintf(frame + n, sizeof(frame) - n, "%s\n", body);
\
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].fd != -1 && strcmp(clients[i].nick, sender) != 0) {
            send_line(clients[i].fd, "%s", frame);
        }
    }
}

static int find_client_by_nick(const char *nick) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].fd != -1 && strcmp(clients[i].nick, nick) == 0) return i;
    }
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    for (int i = 0; i < MAX_CLIENTS; ++i) clients[i].fd = -1;

    int lsock = socket(AF_INET, SOCK_STREAM, 0);
    if (lsock == -1) perror_exit("socket");

    int opt = 1;
    setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(lsock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        perror_exit("bind");
    if (listen(lsock, MAX_CLIENTS) == -1) perror_exit("listen");

    signal(SIGINT, on_sigint);
    printf("[server] listening on port %d …\n", port);

    time_t last_ping = time(NULL);

    while (running) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(lsock, &rfds);
        int maxfd = lsock;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].fd != -1) {
                FD_SET(clients[i].fd, &rfds);
                if (clients[i].fd > maxfd) maxfd = clients[i].fd;
            }
        }

        struct timeval tv = {1, 0};
        if (select(maxfd + 1, &rfds, NULL, NULL, &tv) == -1) {
            if (errno == EINTR) continue;
            perror_exit("select");
        }

        if (FD_ISSET(lsock, &rfds)) {
            int cfd = accept(lsock, NULL, NULL);
            if (cfd == -1) { perror("accept"); continue; }
            char buf[MAX_MSG] = {0};
            ssize_t n = recv(cfd, buf, sizeof(buf) - 1, 0);
            if (n <= 0) { close(cfd); continue; }
            if (strncmp(buf, "HELLO ", 6) != 0) { close(cfd); continue; }
            char nick[MAX_NICK];
            sscanf(buf + 6, "%31s", nick);
            if (find_client_by_nick(nick) != -1 || add_client(cfd, nick) == -1) {
                send_line(cfd, "ERROR nickname_taken_or_server_full\n");
                close(cfd);
                continue;
            }
            printf("[server] %s joined\n", nick);
        }

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].fd != -1 && FD_ISSET(clients[i].fd, &rfds)) {
                char buf[MAX_MSG] = {0};
                ssize_t n = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);
                if (n <= 0) {
                    printf("[server] %s disconnected\n", clients[i].nick);
                    remove_client(i);
                    continue;
                }
                if (strncmp(buf, "LIST", 4) == 0) {
                    send_line(clients[i].fd, "USERS ");
                    for (int k = 0; k < MAX_CLIENTS; ++k) if (clients[k].fd != -1) {
                        send_line(clients[i].fd, "%s ", clients[k].nick);
                    }
                    send_line(clients[i].fd, "\n");
                } else if (strncmp(buf, "2ALL ", 5) == 0) {
                    broadcast(clients[i].nick, buf + 5);
                } else if (strncmp(buf, "2ONE ", 5) == 0) {
                    char target[MAX_NICK];
                    char *msg = strchr(buf + 5, ' ');
                    if (!msg) continue;
                    *msg = '\0';
                    strcpy(target, buf + 5);
                    msg++;
                    int t = find_client_by_nick(target);
                    if (t != -1) {
                        time_t now = time(NULL);
                        char timestr[32];
                        strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));
                        send_line(clients[t].fd, "FROM %s %s %s\n", clients[i].nick, timestr, msg);
                    } else {
                        send_line(clients[i].fd, "ERROR no_such_user\n");
                    }
                } else if (strncmp(buf, "STOP", 4) == 0) {
                    printf("[server] %s requested STOP\n", clients[i].nick);
                    remove_client(i);
                } else if (strncmp(buf, "ALIVE", 5) == 0) {
                    clients[i].last_alive = time(NULL);
                }
            }
        }

        time_t now = time(NULL);
        if (now - last_ping >= PING_INTERVAL) {
            last_ping = now;
            for (int i = 0; i < MAX_CLIENTS; ++i) if (clients[i].fd != -1) {
                if (now - clients[i].last_alive > PING_TIMEOUT) {
                    printf("[server] %s timed out\n", clients[i].nick);
                    remove_client(i);
                } else {
                    send_line(clients[i].fd, "PING\n");
                }
            }
        }
    }

    printf("[server] shutting down…\n");
    for (int i = 0; i < MAX_CLIENTS; ++i) if (clients[i].fd != -1) close(clients[i].fd);
    close(lsock);
    return 0;
}
