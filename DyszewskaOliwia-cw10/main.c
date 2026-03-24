#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>

#define MAX_WAITING_PATIENTS 3
#define MAX_MEDICINE 6

int waiting_patients = 0;
int medicine = MAX_MEDICINE;
int total_patients = 0;
int patients_served = 0;
bool pharmacist_waiting = false;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t doctor_wake = PTHREAD_COND_INITIALIZER;
pthread_cond_t space_in_hospital = PTHREAD_COND_INITIALIZER;
pthread_cond_t medicine_delivered = PTHREAD_COND_INITIALIZER;

int *waiting_ids;
bool **waiting_done_ptrs;

typedef struct {
    int id;
    bool *done;
} patient_info_t;

void print_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* ptm = localtime(&tv.tv_sec);
    char time_string[40];
    strftime(time_string, sizeof(time_string), "%H:%M:%S", ptm);
    printf("[%s.%03ld] ", time_string, tv.tv_usec / 1000);
}

void* doctor(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (!((waiting_patients == 3 && medicine >= 3) || (pharmacist_waiting && medicine < 3))) {
            print_time();
            printf("Lekarz: zasypiam.\n");
            pthread_cond_wait(&doctor_wake, &mutex);
            print_time();
            printf("Lekarz: budzę się.\n");
        }

        if (waiting_patients == 3 && medicine >= 3) {
            int p1 = waiting_ids[0], p2 = waiting_ids[1], p3 = waiting_ids[2];
            print_time();
            printf("Lekarz: konsultuję pacjentów %d, %d, %d\n", p1, p2, p3);
            medicine -= 3;
            patients_served += 3;

            *waiting_done_ptrs[0] = true;
            *waiting_done_ptrs[1] = true;
            *waiting_done_ptrs[2] = true;

            waiting_patients = 0;
            pthread_cond_broadcast(&space_in_hospital);
            pthread_mutex_unlock(&mutex);
            sleep(rand() % 3 + 2);
        }
        else if (pharmacist_waiting && medicine < 3) {
            print_time();
            printf("Lekarz: przyjmuję dostawę leków\n");
            medicine = MAX_MEDICINE;
            pharmacist_waiting = false;
            pthread_cond_signal(&medicine_delivered);
            pthread_mutex_unlock(&mutex);
            sleep(rand() % 3 + 1);
        } else {
            pthread_mutex_unlock(&mutex);
        }

        pthread_mutex_lock(&mutex);
        if (patients_served >= total_patients) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void* patient(void* arg) {
    patient_info_t* info = (patient_info_t*)arg;
    int id = info->id;
    bool* done = info->done;

    while (1) {
        int delay = rand() % 4 + 2;
        print_time();
        printf("Pacjent(%d): Ide do szpitala, bede za %d s\n", id, delay);
        sleep(delay);

        pthread_mutex_lock(&mutex);
        if (waiting_patients >= MAX_WAITING_PATIENTS) {
            pthread_mutex_unlock(&mutex);
            int retry = rand() % 3 + 1;
            print_time();
            printf("Pacjent(%d): za dużo pacjentów, wracam później za %d s\n", id, retry);
            sleep(retry);
            continue;
        }

        waiting_ids[waiting_patients] = id;
        waiting_done_ptrs[waiting_patients] = done;
        waiting_patients++;

        print_time();
        printf("Pacjent(%d): czeka %d pacjentów na lekarza\n", id, waiting_patients);

        if (waiting_patients == 3) {
            print_time();
            printf("Pacjent(%d): budzę lekarza\n", id);
            pthread_cond_signal(&doctor_wake);
        }

        while (!(*done)) {
            pthread_cond_wait(&space_in_hospital, &mutex);
        }

        pthread_mutex_unlock(&mutex);
        print_time();
        printf("Pacjent(%d): kończę wizytę\n", id);
        break;
    }

    return NULL;
}

void* pharmacist(void* arg) {
    int id = *(int*)arg;
    int delay = rand() % 11 + 5;
    print_time();
    printf("Farmaceuta(%d): ide do szpitala, bede za %d s\n", id, delay);
    sleep(delay);

    pthread_mutex_lock(&mutex);
    while (medicine == MAX_MEDICINE) {
        print_time();
        printf("Farmaceuta(%d): czekam na oproznienie apteczki\n", id);
        pthread_cond_wait(&medicine_delivered, &mutex);
    }

    if (medicine < 3) {
        pharmacist_waiting = true;
        print_time();
        printf("Farmaceuta(%d): budzę lekarza\n", id);
        pthread_cond_signal(&doctor_wake);
    }

    pthread_cond_wait(&medicine_delivered, &mutex);
    print_time();
    printf("Farmaceuta(%d): dostarczam leki\n", id);
    pthread_mutex_unlock(&mutex);

    print_time();
    printf("Farmaceuta(%d): zakończyłem dostawę\n", id);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Użycie: %s <liczba_pacjentów> <liczba_farmaceutów>\n", argv[0]);
        return 1;
    }

    int num_patients = atoi(argv[1]);
    int num_pharmacists = atoi(argv[2]);
    total_patients = num_patients;

    pthread_t doctor_thread;
    pthread_t patient_threads[num_patients];
    pthread_t pharmacist_threads[num_pharmacists];

    patient_info_t* patient_info = malloc(sizeof(patient_info_t) * num_patients);
    bool* patient_done = malloc(sizeof(bool) * num_patients);
    int* pharmacist_ids = malloc(sizeof(int) * num_pharmacists);

    waiting_ids = malloc(sizeof(int) * MAX_WAITING_PATIENTS);
    waiting_done_ptrs = malloc(sizeof(bool*) * MAX_WAITING_PATIENTS);

    srand(time(NULL));
    pthread_create(&doctor_thread, NULL, doctor, NULL);

    for (int i = 0; i < num_patients; i++) {
        patient_done[i] = false;
        patient_info[i].id = i + 1;
        patient_info[i].done = &patient_done[i];
        pthread_create(&patient_threads[i], NULL, patient, &patient_info[i]);
    }

    for (int i = 0; i < num_pharmacists; i++) {
        pharmacist_ids[i] = i + 1;
        pthread_create(&pharmacist_threads[i], NULL, pharmacist, &pharmacist_ids[i]);
    }

    for (int i = 0; i < num_patients; i++)
        pthread_join(patient_threads[i], NULL);

    pthread_join(doctor_thread, NULL);

    for (int i = 0; i < num_pharmacists; i++)
        pthread_join(pharmacist_threads[i], NULL);

    free(waiting_ids);
    free(waiting_done_ptrs);
    free(patient_info);
    free(patient_done);
    free(pharmacist_ids);

    return 0;
}
