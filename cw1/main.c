#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>

#define BUFFOR_SIZE 80

double* vector;
int* range;
double* result;
int id;

void handle_sigusr1(int sig) {
    int id = sig - SIGRTMIN;
    double sum = 0.0f;
    for(int i=range[id]; i<range[id+1]; i++) {
        sum += vector[i];
        usleep(1000);

    }
    result[id] = sum;
    exit(0);
}


int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Nie podano nazwy pliku\n");
        return 1;
    }

    FILE* f = fopen(argv[1], "r");
    if(f == NULL) {
        perror("Bład przy otwieraniu pliku\n");
        return 1;
    }
    char buffor[BUFFOR_SIZE+1];
    fgets(buffor, BUFFOR_SIZE, f);
    const int vector_elements_num = atoi(buffor);
    vector = malloc(sizeof(double) * vector_elements_num);
    printf("Liczba elementów w wektorze: %i \n", vector_elements_num);
    for(int i=0; i<vector_elements_num; i++) {
        fgets(buffor, BUFFOR_SIZE, f);
        vector[i] = atof(buffor);
    }
    fclose(f);

    sigset_t set;
    int sig;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);

    int values[] = {1, 2, 4, 6, 8, 16};
    int N;
    for(int k=0; k<6; k++) {
        N = values[k];
        range = mmap(NULL, sizeof(int) * (N + 1), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        result = mmap(NULL, sizeof(double) * N, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        pid_t pids[N];
        for(int i=0; i<N; i++) {
            signal(SIGRTMIN + i, handle_sigusr1);
            if((pids[i] = fork()) == 0) {
                sigwait(&set, &sig);
            }
        }

        for(int i=0; i<N; i++) {
            range[i] = (vector_elements_num / N) * i;
        }
        range[N] = vector_elements_num;

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        for(int i=0; i<N; i++) {
            kill(pids[i], SIGRTMIN + i);
        }

        for(int i=0; i<N; i++) {
            wait(NULL);
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        long elapsed = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
        printf("Czas wykonania dla N = %d: %ld ns\n", N, elapsed);

        double sum = 0.0f;
        for(int i=0; i<N; i++) {
            sum += result[i];
        }

        printf("Suma = %f\n", sum);

        munmap(range, sizeof(int) * (N + 1));
        munmap(result, sizeof(double) * N);
    }

    free(vector);

    return 0;
}

