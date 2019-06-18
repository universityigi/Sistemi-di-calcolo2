#include "util.h"

#include <errno.h>
#include <fcntl.h>  // O_CREAT and O_EXCL flags
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define LOG_INTERVAL        1
#define NUM_RESOURCES       3
#define SEMAPHORE_NAME      "/simple_scheduler"

// we use a global variable to store the pointer to the named semaphore
sem_t* named_semaphore;

void cleanup() {
    printf("\rShutting down the server...\n");
	if(sem_close(named_semaphore)) handle_error("sem_close error");
    if(sem_unlink(SEMAPHORE_NAME)) handle_error("sem_unlink error");
    /** Remove the named semaphore.
     *
     * After closing the semaphore, the server process is responsible
     * for unlinking it: in fact, since it's the server that grants
     * access to a shared resource, the named semaphore cannot remain
     * in the system after the server terminates!
     * **/

    /**
     * TODO: EDIT AND IMPLEMENT THE OPERATION DESCRIBED ABOVE
     **/

    exit(0);
}

int main(int argc, char* argv[]) {
    int ret;
    sem_unlink(SEMAPHORE_NAME);
    named_semaphore = sem_open(SEMAPHORE_NAME, O_CREAT | O_EXCL,0600, NUM_RESOURCES);
    
    if(named_semaphore == SEM_FAILED){
		handle_error(" Could not open the named semaphore");
	}
    /** Create a named semaphore to be shared with different processes.
     *
     * O_CREAT tells the system to create the semaphore named SEMAPHORE_NAMED
     * if it does not already exist. When used in an OR combination with
     * O_EXCL, the call fails if the semaphore already exists.
     *
     * Mode 0600 means that only processes owned by the user that creates
     * the semaphore can access it, with read & write (4+2) permissions.
     *
     * We initialize the semaphore with a value equal to NUM_RESOURCES.
     **/

    
    
    /**
     * TODO: EDIT AND IMPLEMENT THE OPERATION DESCRIBED ABOVE
     **/
    
    // creation might fail if the named semaphore hasn't been deleted since its last creation
    // first we have to unlink it

    /* Since a CTRL+C would kill the program, we rely on an auxiliary
     * method to catch the interrupt and execute a cleanup code before
     * terminating the program. */
    setQuitHandler(&cleanup);

    printf("Welcome! This is the server module of our simple resource scheduler.\n\n");
    printf("%d resources are initially available in the system. Use CTRL+C to exit!\n\n", NUM_RESOURCES);

    /* Main loop */
    while(1) {
        /** We want to periodically print statistics on our server's load to screen **/
        char timestamp[9];
        time_t now = time(0);

        // get a timestamp of the form "HH:MM:SS" and store it into a buffer
        strftime((char*)timestamp, 9, "%H:%M:%S", localtime(&now));

        /** Get the current value for the semaphore and store it into a
         * custom local variable (we pass its address using &). The
         * value for the semaphore is the number of resources not in use. **/
        int current_value;
        ret = sem_getvalue(named_semaphore, &current_value);

        if (ret) {
            handle_error("Could not access the named semaphore");
			exit(1);
        }

        printf("[%s] %d resources are available and %d are in use\n", timestamp, current_value, NUM_RESOURCES - current_value);

        sleep(LOG_INTERVAL);
    }

    /*** We will never reach this point since we want to exit the program through CTRL+C only! ***/

    return 0;
}
