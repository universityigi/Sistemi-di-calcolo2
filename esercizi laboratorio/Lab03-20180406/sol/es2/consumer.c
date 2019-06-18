#include "common.h"

#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

sem_t *sem_filled, *sem_empty, *sem_cs;

void openSemaphores() {
    sem_filled = sem_open(SEMNAME_FILLED, 0);
    if (sem_filled == SEM_FAILED) handle_error("sem_open filled");
    
    sem_empty = sem_open(SEMNAME_EMPTY, 0);
    if (sem_empty == SEM_FAILED) handle_error("sem_open empty");

    sem_cs = sem_open(SEMNAME_CS, 0);
    if (sem_cs == SEM_FAILED) handle_error("sem_open cs");
}

void closeAndDestroySemaphores() {
    int ret;
    
    // first close
    ret = sem_close(sem_filled);
    if (ret) handle_error("sem_close filled");

    ret = sem_close(sem_empty);
    if (ret) handle_error("sem_close empty");

    ret = sem_close(sem_cs);
    if (ret) handle_error("sem_close cs");

    // then unlink
    ret = sem_unlink(SEMNAME_FILLED);
    if (ret) handle_error("sem_unlink filled");

    ret = sem_unlink(SEMNAME_EMPTY);
    if (ret) handle_error("sem_unlink empty");

    ret = sem_unlink(SEMNAME_CS);
    if (ret) handle_error("sem_unlink cs");

}

void consume(int id, int numOps) {
    int localSum = 0;
    while (numOps > 0) {
        int ret = sem_wait(sem_filled);
        if (ret) handle_error("sem_wait filled");

        sem_wait(sem_cs);
        if (ret) handle_error("sem_wait cs");

        // producer, just do your thing!
        int value = readFromBufferFile(BUFFER_SIZE, BUFFER_FILENAME);
        localSum += value;

        sem_post(sem_cs);
        if (ret) handle_error("sem_post cs");

        sem_post(sem_empty);
        if (ret) handle_error("sem_post empty");

        numOps--;
    }
    printf("Consumer %d ended. Local sum is %d\n", id, localSum);
}

int main(int argc, char** argv) {
    if (access(BUFFER_FILENAME, F_OK) == -1) {
        printf("ERROR: no buffer file. Start the producer(s) first!\n");
        exit(EXIT_FAILURE);
    }

    openSemaphores();

    int i;
    for (i=0; i<NUM_CONSUMERS; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            handle_error("fork");
        } else if (pid == 0) {
            consume(i, OPS_PER_CONSUMER);
            _exit(EXIT_SUCCESS);
        }
    }

    for (i=0; i<NUM_CONSUMERS; ++i) {
        int status;
        wait(&status);
        if (WEXITSTATUS(status)) handle_error("child crashed");
    }

    printf("Consumers have terminated. Exiting...\n");

    closeAndDestroySemaphores();

    exit(EXIT_SUCCESS);
}