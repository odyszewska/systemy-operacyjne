#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int main() {
    const char *fifo_send = "/tmp/fifo_to_receiver";
    const char *fifo_receive = "/tmp/fifo_to_sender";

    mkfifo(fifo_send, 0666);
    mkfifo(fifo_receive, 0666);

    double a, b;
    printf("Podaj przedział całkowania (a b): ");
    scanf("%lf %lf", &a, &b);

    int fd_send = open(fifo_send, O_WRONLY);
    double bounds[2] = {a, b};
    write(fd_send, bounds, sizeof(bounds));
    close(fd_send);

    int fd_receive = open(fifo_receive, O_RDONLY);
    double result;
    read(fd_receive, &result, sizeof(result));
    close(fd_receive);

    printf("Wynik całkowania w przedziale [%.6f, %.6f] = %.10f\n", a, b, result);

    unlink(fifo_send);
    unlink(fifo_receive);

    return 0;
}
