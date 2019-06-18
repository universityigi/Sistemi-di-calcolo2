#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

// macros for error handling
#include "common.h"

#define N 100   // child process count
#define M 10    // thread per child process count
#define T 3     // time to sleep for main process

#define END_CHILDREN_ACTIVITIES_SEMAPHORE_NAME "/end_children_activities"
#define MAIN_WAITS_FOR_CHILDREN_SEMAPHORE_NAME	"/main_waits_for_children"
#define CHILDREN_WAIT_FOR_MAIN_SEMAPHORE_NAME	"/children_wait_for_main"
#define CRITICAL_SECTION						"/critical_section"
#define FILENAME	"accesses.log"

/*
 * data structure required by threads
 */
typedef struct thread_args_s {
    unsigned int child_id;
    unsigned int thread_id;
} thread_args_t;

/*
 * parameters can be set also via command-line arguments
 */
int n = N, m = M, t = T;

/* TODO: declare as many semaphores as needed to implement
 * the intended semantics, and choose unique identifiers for
 * them (e.g., "/mysem_critical_section") */
sem_t* end_children_activities = NULL;
sem_t* main_waits_for_children = NULL;
sem_t* children_wait_for_main = NULL;
sem_t* critical_section = NULL;


/*
 * Ensures that an empty file with given name exists.
 */
void init_file(const char *filename) {
    printf("[Main] Initializing file %s...", FILENAME);
    fflush(stdout);
    int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd<0) handle_error("error while initializing file");
    close(fd);
    printf("closed...file correctly initialized!!!\n");
}

/*
 * Create a named semaphore with a given name, mode and initial value.
 * Also, tries to remove any pre-existing semaphore with the same name.
 */
sem_t *create_named_semaphore(const char *name, mode_t mode, unsigned int value) {
    printf("[Main] Creating named semaphore %s...", name);
    fflush(stdout);
    sem_unlink(name);
    sem_t* sem = sem_open(name, O_CREATE | O_EXCL, mode, value);
    if(sem = SEM_FAILED){
		handle_error("Error creating semaphore");
	}
    
    // TODO

    printf("done!!!\n");
    return sem;
}

void parseOutput() {
    // identify the child that accessed the file most times
    int* access_stats = calloc(n, sizeof(int)); // initialized with zeros
    printf("[Main] Opening file %s in read-only mode...", FILENAME);
	fflush(stdout);
    int fd = open(FILENAME, O_RDONLY);
    if (fd < 0) handle_error("error while opening output file");
    printf("ok, reading it and updating access stats...");
	fflush(stdout);

    size_t read_bytes;
    int index;
    do {
        read_bytes = read(fd, &index, sizeof(int));
        if (read_bytes > 0)
            access_stats[index]++;
    } while(read_bytes > 0);
    printf("ok, closing it...");
	fflush(stdout);

    close(fd);
    printf("closed!!!\n");

    int max_child_id = -1, max_accesses = -1;
    for (i = 0; i < n; i++) {
        printf("[Main] Child %d accessed file %s %d times\n", i, FILENAME, access_stats[i]);
        if (access_stats[i] > max_accesses) {
            max_accesses = access_stats[i];
            max_child_id = i;
        }
    }
    printf("[Main] ===> The process that accessed the file most often is %d (%d accesses)\n", max_child_id, max_accesses);
    free(access_stats);
}

void* thread_function(void* x) {
    thread_args_t *args = (thread_args_t*)x;
    int ret = sem_wait(critical_section);
    if(ret) handle_error("sem_wait failed");
    printf("[Child#%d-Thread#%d] Entered into critical section!!!\n", args->child_id, args->thread_id);

    /* TODO: protect the critical section using semaphore(s) as needed */

    // open file, write child identity and close file
    int fd = open(FILENAME, O_WRONLY | O_APPEND);
    if (fd < 0) handle_error("error while opening file");
    printf("[Child#%d-Thread#%d] File %s opened in append mode!!!\n", args->child_id, args->thread_id, FILENAME);	

    write(fd, &(args->child_id), sizeof(int));
    printf("[Child#%d-Thread#%d] %d appended to file %s opened in append mode!!!\n", args->child_id, args->thread_id, args->child_id, FILENAME);	

    close(fd);
    printf("[Child#%d-Thread#%d] File %s closed!!!\n", args->child_id, args->thread_id, FILENAME);
	
	ret = sem_post(critical_section);
	if(ret) handle_error( "sem_post failed");
	printf("[Child#%d-Thread#%d] Exited from critical section!!!\n", args->child_id, args->thread_id);
    free(args);
    pthread_exit(NULL);
}

void mainProcess() {
    /* TODO: the main process waits for all the children to start,
     * it notifies them to start their activities, and sleeps
     * for some time t. Once it wakes up, it notifies the children
     * to end their activities, and waits for their termination.
     * Finally, it calls the parseOutput() method and releases
     * any shared resources. */
     printf("[Main] %d children created, wait for all children to be ready...\n", n);
     int i,ret;
     for(i=0; i < n; i++){
		 ret = sem_wait(main_waits_for_children);
		 if(ret) handle_error(" sem_wait failed");
	 }
	 printf("[Main] All the children are now ready!!!\n");	
	 for(i=0; i < n; i++){
		 ret = sem_post(children_wait_for_main);
		 if(ret) handle_error(" sem_post failed");
	 }
	 
	 printf("[Main] Children have been notified to start their activities!!!\n");	

    // main process
    printf("[Main] Sleeping for %d seconds...\n", t);	
    sleep(t);
    printf("[Main] Woke up after having slept for %d seconds!!!\n", t);	

    // notify children to end their activities
    printf("[Main] Notifying children to end their activities...\n");	
    
	ret = sem_post(end_children_activities);
	if(ret) {
	    handle_error("sem_post failed");
    }
	printf("[Main] Children have been notified to end their activities!!!\n");
	
}

void childProcess(int child_id) {
    /* TODO: each child process notifies the main process that it
     * is ready, then waits to be notified from the main in order
     * to start. As long as the main process does not notify a
     * termination event [hint: use sem_getvalue() here], the child
     * process repeatedly creates m threads that execute function
     * thread_function() and waits for their completion. When a
     * notification has arrived, the child process notifies the main
     * process that it is about to terminate, and releases any
     * shared resources before exiting. */
}

int main(int argc, char **argv) {
    // arguments
    if (argc > 1) n = atoi(argv[1]);
    if (argc > 2) m = atoi(argv[2]);
    if (argc > 3) t = atoi(argv[3]);

    // initialize the file
    init_file(FILENAME);

    /* TODO: initialize any semaphore needed in the implementation, and
     * create N children where the i-th child calls childProcess(i); then
     * the main process executes function mainProcess() once all the
     * children have been created */

    exit(EXIT_SUCCESS);
}
