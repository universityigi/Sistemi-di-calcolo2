#include "util.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_SLEEP           6
#define THREAD_BURST        5
#define SEMAPHORE_NAME      "/simple_scheduler"

typedef struct thread_args_s {
    int     ID;
} thread_args_t;

void* client(void *arg_ptr) {
    char errorStr[100];
    thread_args_t* args = (thread_args_t*) arg_ptr;

    /** Open an existing named semaphore.
     *
     * For this operation, we can use the short version of sem_open
     * that takes only two parameters, namely the identifier for
     * the semaphore and the desired flags (0 in this scenario).
     **/

    /**
     * TODO: EDIT AND IMPLEMENT THE OPERATION DESCRIBED ABOVE
     **/


    sem_t* my_named_semaphore = sem_open(SEMAPHORE_NAME,0);

    if (my_named_semaphore == SEM_FAILED) {
        snprintf(errorStr, sizeof(errorStr), "Could not open the named semaphore from thread %d", args->ID);
        handle_error_en(SEM_FAILED,errorStr);
    }

    /*** Acquire the resource ***/
    if (sem_wait(my_named_semaphore)) {
        snprintf(errorStr, sizeof(errorStr), "Could not lock the semaphore from thread %d", args->ID);
        handle_error(errorStr);
    }

    printf("[@Thread%d] Resource acquired...\n", args->ID);

    // we simulate some work by sleeping for 0 up to MAX_SLEEP seconds
    sleep(rand() % (MAX_SLEEP+1));

    /*** Free the resource ***/
    if (sem_post(my_named_semaphore)) {
        snprintf(errorStr, sizeof(errorStr), "Could not unlock the semaphore from thread %d", args->ID);
        handle_error(errorStr);
    }

    printf("[@Thread%d] Done. Resource released!\n", args->ID);

    /*** Close the named semaphore ***/
    if (sem_close(my_named_semaphore)) {
        snprintf(errorStr, sizeof(errorStr), "Could not close the semaphore from thread %d", args->ID);
        handle_error(errorStr);
    }

    free(args);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    char errorStr[100];
    int thread_ID = 0;

    printf("Welcome! This is a simple client for our FCFS scheduler.\n\n");
    printf("Please make sure that the server is already running in a separate terminal.\n\n");

    /* Main loop */
    printf("[DRIVER] Press ENTER to spawn %d new threads. Press CTRL+D to quit!\n", THREAD_BURST);

    while(1) {
        int input_char;

        /* We want to skip any character that is not allowed:
         * - when ENTER is pressed, on Linux the character '\n' is read by getchar()
         * - CTRL+D is read as EOF, a special sequence defined in stdio.h */
        while ( (input_char = getchar()) != '\n' && input_char != EOF ) continue;

        if (input_char == EOF) break;

        printf("==> [DRIVER] Spawning %d threads now...\n", THREAD_BURST);

        int i;
        for (i = 0; i < THREAD_BURST; ++i) {
            pthread_t thread_handle;

            thread_args_t* args = malloc(sizeof(thread_args_t));
            args->ID = thread_ID;

			int ret = pthread_create(&thread_handle, NULL, client, args);
            if (ret) {
                snprintf(errorStr, sizeof(errorStr), "Cannot create thread %d", thread_ID);
                handle_error_en(ret, errorStr);
            }

            ++thread_ID;

            // I won't wait for this thread to terminate: let's detach it!
            ret = pthread_detach(thread_handle);
			if (ret) {
			    snprintf(errorStr, sizeof(errorStr), "cannot detach thread %d", thread_ID);
                handle_error_en(ret, errorStr);
            }
        }

        printf("==> [DRIVER] Press ENTER to spawn %d new threads. Press CTRL+D to quit!\n", THREAD_BURST);
    }

    printf("[DRIVER] Waiting for any running thread to complete and then exiting...\n");

    // CAREFUL: as we are in the main thread, using exit here
	// would terminate the process! By using pthread_exit we
	// let the other threads complete when we exit the main one
	pthread_exit(NULL);
}
