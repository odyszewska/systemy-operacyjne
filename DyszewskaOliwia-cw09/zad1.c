#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>

long double f(long double x) {
    return 4.0L / (x * x + 1.0L);
}

typedef struct {
    int thread_id;
    long long start_index;
    long long end_index;
    long double width;
    long double *results;
    int *ready;
} ThreadData;

void *compute_integral(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    long double sum = 0.0L;
    for (long long i = data->start_index; i < data->end_index; ++i) {
        long double x = i * data->width;
        sum += f(x) * data->width;
    }
    data->results[data->thread_id] = sum;
    data->ready[data->thread_id] = 1;
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    long double width = strtold(argv[1], NULL);
    int num_threads = atoi(argv[2]);
    long long num_rectangles = (long long)(1.0L / width);

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    ThreadData *thread_data = malloc(num_threads * sizeof(ThreadData));
    long double *results = calloc(num_threads, sizeof(long double));
    int *ready = calloc(num_threads, sizeof(int));

    int rectangles_per_thread = num_rectangles / num_threads;
    int remainder = num_rectangles % num_threads;

    long long current_start = 0;
    for (int i = 0; i < num_threads; ++i) {
        long long current_end = current_start + rectangles_per_thread;
        if (i < remainder) current_end++;

        thread_data[i].thread_id = i;
        thread_data[i].start_index = current_start;
        thread_data[i].end_index = current_end;
        thread_data[i].width = width;
        thread_data[i].results = results;
        thread_data[i].ready = ready;

        pthread_create(&threads[i], NULL, compute_integral, &thread_data[i]);

        current_start = current_end;
    }

    int all_ready = 0;
    while (!all_ready) {
        all_ready = 1;
        for (int i = 0; i < num_threads; ++i) {
            if (ready[i] == 0) {
                all_ready = 0;
                break;
            }
        }
    }

    long double final_result = 0.0L;
    for (int i = 0; i < num_threads; ++i) {
        final_result += results[i];
    }

    printf("Przybliżona wartość całki: %.18Lf\n", final_result);

    free(threads);
    free(thread_data);
    free(results);
    free(ready);

    return 0;
}
