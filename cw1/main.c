#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <signal.h>

#define BUFFOR_SIZE 80
#define CHILD 4

double* vector;
int* range;
double* result;
int id;

void handle_sigusr1(int sig) {
    int id = sig - SIGRTMIN;
    double sum = 0.0f;
    for(int i=range[id]; i<range[id+1]; i++) {
        sum += vector[i];
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
    range = mmap(NULL, sizeof(int) * (CHILD + 1), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    result = mmap(NULL, sizeof(double) * CHILD, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    pid_t pids[CHILD];
    printf("Liczba elementów w wektorze: %i \n", vector_elements_num);
    for(int i=0; i<vector_elements_num; i++) {
        fgets(buffor, BUFFOR_SIZE, f);
        vector[i] = atof(buffor);
    }
    fclose(f);
    printf("v = [ ");
    for(int i=0; i<vector_elements_num; i++) {
        printf("%f ", vector[i]);
    }
    printf("]\n");

    sigset_t set;
    int sig;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);

    for(int i=0; i<CHILD; i++) {
        signal(SIGRTMIN + i, handle_sigusr1);
        if((pids[i] = fork()) == 0) {
            sigwait(&set, &sig);
        }
    }


    for(int i=0; i<CHILD; i++) {
        range[i] = (vector_elements_num / CHILD) * i;
    }
    range[CHILD] = vector_elements_num;

    for(int i=0; i<CHILD; i++) {
        kill(pids[i], SIGRTMIN + i);
    }

    for(int i=0; i<CHILD; i++) {
        wait(NULL);
    }

    double sum = 0.0f;
    for(int i=0; i<CHILD; i++) {
        sum += result[i];
    }

    printf("Suma = %f\n", sum);

    munmap(range, sizeof(int) * (CHILD + 1));
    munmap(result, sizeof(double) * CHILD);
    free(vector);

    return 0;
}