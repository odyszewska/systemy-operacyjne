#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

double f(double x) {
    return 4.0 / (x * x + 1);
}

double rectangle_integration(double a, double b, double width) {
    double result = 0.0;
    for (double x = a; x < b; x += width) {
        result += f(x) * width;
    }
    return result;
}

int main(int argc, char *argv[]) {
    double width = atof(argv[1]);
    int max_k = atoi(argv[2]);
    double a = 0.0, b = 1.0;

    for (int k = 1; k <= max_k; ++k) {
        int pipes[k][2];
        pid_t pids[k];

        struct timeval start, end;
        gettimeofday(&start, NULL);

        double interval = (b - a) / k;

        for (int i = 0; i < k; ++i) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            pids[i] = fork();

            if (pids[i] == 0) {
                close(pipes[i][0]);

                double local_a = a + i * interval;
                double local_b = local_a + interval;
                double local_result = rectangle_integration(local_a, local_b, width);

                write(pipes[i][1], &local_result, sizeof(local_result));
                close(pipes[i][1]);

                exit(EXIT_SUCCESS);
            }
            close(pipes[i][1]);
        }

        double total_result = 0.0;
        for (int i = 0; i < k; ++i) {
            double partial_result;
            read(pipes[i][0], &partial_result, sizeof(partial_result));
            total_result += partial_result;
            close(pipes[i][0]);
            waitpid(pids[i], NULL, 0);
        }

        gettimeofday(&end, NULL);

        double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

        printf("k = %d, wynik = %.10f, czas = %.6f sekund\n", k, total_result, time_spent);
    }
    return 0;
}
