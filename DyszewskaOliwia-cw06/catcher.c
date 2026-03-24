#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

double f(double x) {
    return 4.0 / (x * x + 1.0);
}

double rectangle_integration(double a, double b, double width) {
    double result = 0.0;
    for (double x = a; x < b; x += width) {
        result += f(x) * width;
    }
    return result;
}

int main() {
    const char *fifo_send = "/tmp/fifo_to_receiver";
    const char *fifo_receive = "/tmp/fifo_to_sender";

    mkfifo(fifo_send, 0666);
    mkfifo(fifo_receive, 0666);


    int fd_send = open(fifo_send, O_RDONLY);

    double bounds[2];
    read(fd_send, bounds, sizeof(bounds));
    close(fd_send);

    double a = bounds[0];
    double b = bounds[1];
    double width = 0.000001;
    double result = rectangle_integration(a, b, width);

    int fd_receive = open(fifo_receive, O_WRONLY);
    write(fd_receive, &result, sizeof(result));
    close(fd_receive);

    return 0;
}
