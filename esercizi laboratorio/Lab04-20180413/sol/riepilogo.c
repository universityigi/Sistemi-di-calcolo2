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

#define END_CHILDREN_ACTIVITIES_SEMAPHORE_NAME	"/end_children_activities"
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

// parameters can be set also via command-line arguments
int n = N, m = M, t = T;

/*
 * named semaphore for letting main process wait for all the children to
 * start
 */
sem_t *main_waits_for_children = NULL;

/*
 * named semaphore for letting children wait for the main to notify to
 * begin activities
 */
sem_t *children_wait_for_main = NULL;

// semaphore to protect the critical section
sem_t *critical_section = NULL;

// named semaphore for notifying child processes to end their activities
sem_t *end_children_activities = NULL;

/*
 * Create a named semaphore with a given name, mode and initial value.
 * Also, tries to remove any pre-existing semaphore with the same name.
 */
sem_t *create_named_semaphore(const char *name, mode_t mode, unsigned int value) {
    printf("[Main] Creating named semaphore %s...", name);
	fflush(stdout);
    /* if exist we first unlink it */
    sem_unlink(name);
    sem_t *sem = sem_open(name, O_CREAT|O_EXCL, mode, value);
    if (sem == SEM_FAILED) {
        handle_error("Error creating named semaphore");
    }
    printf("done!!!\n");
    return sem;
}

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

void* thread_function(void* arg_ptr) {

    thread_args_t *args = (thread_args_t*)arg_ptr;

    // enter critical section
    int ret = sem_wait(critical_section);
    if(ret) {
        handle_error("sem_wait failed");
	}
    printf("[Child#%d-Thread#%d] Entered into critical section!!!\n", args->child_id, args->thread_id);

    // open file, write child identity and close file
    int fd = open(FILENAME, O_WRONLY | O_APPEND);
    if (fd < 0) handle_error("error while opening file");
    printf("[Child#%d-Thread#%d] File %s opened in append mode!!!\n", args->child_id, args->thread_id, FILENAME);	

    write(fd, &(args->child_id), sizeof(int));
    printf("[Child#%d-Thread#%d] %d appended to file %s opened in append mode!!!\n", args->child_id, args->thread_id, args->child_id, FILENAME);	

    close(fd);
    printf("[Child#%d-Thread#%d] File %s closed!!!\n", args->child_id, args->thread_id, FILENAME);

    // exit critical section
    ret = sem_post(critical_section);
	if(ret) {
	    handle_error("sem_post failed");
	}
	
    printf("[Child#%d-Thread#%d] Exited from critical section!!!\n", args->child_id, args->thread_id);

    // clean up
    printf("[Child#%d-Thread#%d] Completed!!!\n", args->child_id, args->thread_id);	
    free(args);

    pthread_exit(NULL);
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

    int max_child_id = -1, max_accesses = -1, i;
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

void main_process() {
    // wait for all the children to start
    printf("[Main] %d children created, wait for all children to be ready...\n", n);
    int i, ret;
    for (i = 0; i < n; i++) {
        ret = sem_wait(main_waits_for_children);
		if(ret) {
		    handle_error("sem_wait failed");
		}
	}
    printf("[Main] All the children are now ready!!!\n");	

    // notify children to start their activities
    printf("[Main] Notifying children to start their activities...\n");
    for (i = 0; i < n; i++) {
        ret = sem_post(children_wait_for_main);
		if(ret) {
		    handle_error("sem_post failed");
		}
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

    // wait for all the children to terminate
    printf("[Main] Waiting for all the children to terminate...\n");
    int child_status;
    for (i = 0; i < n; i++) {
        ret = wait(&child_status);
		if(ret) {
		    handle_error("wait failed");
		}
        if (WEXITSTATUS(child_status)) {
            fprintf(stderr, "ERROR: child died with code %d\n", WEXITSTATUS(child_status));
            exit(EXIT_FAILURE);
        }
	}
    printf("[Main] All the children have terminated!!!\n");

    // identify the child that accessed the file most times
    parseOutput();

    // clean up
    printf("[Main] Cleaning up...");
	fflush(stdout);

    ret = sem_close(end_children_activities);
    if(ret) {
	    handle_error("sem_close failed");
	}
    ret = sem_unlink(END_CHILDREN_ACTIVITIES_SEMAPHORE_NAME);
	if(ret) {
	    handle_error("sem_unlink failed");
	}
	

    ret = sem_close(main_waits_for_children);
	if(ret) {
	    handle_error("sem_close failed");
    }
    
    ret = sem_unlink(MAIN_WAITS_FOR_CHILDREN_SEMAPHORE_NAME);
	if(ret) {
	    handle_error("sem_unlink failed");
    }
    
    ret = sem_close(children_wait_for_main);
	if(ret) {
	    handle_error("sem_close failed");
    }
    
    ret = sem_unlink(CHILDREN_WAIT_FOR_MAIN_SEMAPHORE_NAME);
	if(ret) {
	    handle_error("sem_unlink failed");
	}

    ret = sem_close(critical_section);
	if(ret) {
	    handle_error("sem_close failed");
    }
    ret = sem_unlink(CRITICAL_SECTION);
	if(ret) {
	    handle_error("sem_unlink failed");
	}

    printf("done!!!\n");
}

void child_process(int child_id) {
    printf("[Child#%d] Child process initialized\n", child_id);

    // notify main process that I am ready
    int ret = sem_post(main_waits_for_children);
    if(ret) {
        handle_error("sem_post_failed");
    }

    printf("[Child#%d] Main process notified that I am ready!!!\n", child_id);

    // wait for main to notify me to begin my activities
    ret = sem_wait(children_wait_for_main);
    if(ret) {
        handle_error("sem_wait failed");
    }
	printf("[Child#%d] Notification to begin received!!!\n", child_id);

    int main_notification;
    unsigned int thread_id = 0;
    pthread_t* thread_handlers = malloc(m * sizeof(pthread_t));

    do {
        int j;

        // reuse the buffer across iterations
        memset(thread_handlers, 0, m * sizeof(pthread_t));

        // create M threads
        printf("[Child#%d] Creating %d threads...\n", child_id, m);
        for (j = 0; j < m; j++) {
            thread_args_t *t_args = (thread_args_t *)malloc(sizeof(thread_args_t));
            t_args->child_id = child_id;
            t_args->thread_id = thread_id++;
            ret = pthread_create(&thread_handlers[j], NULL, thread_function, t_args);
            if(ret) {
                handle_error_en(ret, "pthread_create failed");
            }
			
        }
        printf("[Child#%d] %d threads created!!!\n", child_id, m);

        // wait for their completion
        printf("[Child#%d] Waiting for the end of the %d threads...\n", child_id, m);
        for (j = 0; j < m; j++) {
            ret = pthread_join(thread_handlers[j], NULL);
			if(ret){
			    handle_error_en(ret, "pthread_join failed");
			}
		}
        printf("[Child#%d] %d threads completed!!!\n", child_id, m);

        printf("[Child#%d] Checking for end activities notification...\n", child_id);
        ret = sem_getvalue(end_children_activities, &main_notification);
		if(ret) {
		    handle_error_en(ret, "sem_getvalue failed");
		}

        if (main_notification) break;

        printf("[Child#%d] Go on with activities!!!\n", child_id);
    } while(1);

    free(thread_handlers);

    printf("[Child#%d] Activities completed!!!\n", child_id);

    // close our local handles to the named semaphores
    ret = sem_close(end_children_activities);
	if(ret) {
	    handle_error("sem_close failed");
	}
    
    ret = sem_close(main_waits_for_children);
	if(ret) {
	    handle_error("sem_close failed");
    }
    
    ret = sem_close(children_wait_for_main);
	if(ret) {
	    handle_error("sem_close failed");
    }
    
    ret = sem_close(critical_section);
	if(ret){
	    handle_error("sem_close failed");
	}
}

int main(int argc, char **argv) {
    // arguments
    if (argc > 1) n = atoi(argv[1]);
    if (argc > 2) m = atoi(argv[2]);
    if (argc > 3) t = atoi(argv[3]);

    int i;

    // end_children_activities named semaphore
    end_children_activities = create_named_semaphore(END_CHILDREN_ACTIVITIES_SEMAPHORE_NAME, 0600, 0);

    // main_waits_for_children named semaphore
    main_waits_for_children = create_named_semaphore(MAIN_WAITS_FOR_CHILDREN_SEMAPHORE_NAME, 0600, 0);

    // children_wait_for_main named semaphore
    children_wait_for_main = create_named_semaphore(CHILDREN_WAIT_FOR_MAIN_SEMAPHORE_NAME, 0600, 0);

    // critical section named semaphore
    critical_section = create_named_semaphore(CRITICAL_SECTION, 0600, 1);

    // initialize the file
    init_file(FILENAME);

    // create the N children
    printf("[Main] Creating %d children...\n", n);
    for (i = 0; i < n; i++) {
        pid_t pid = fork(); // PID of child process
        if (pid == -1) {
            printf("Error creating child process #%d: %s\n", i, strerror(errno));
            exit(EXIT_FAILURE); // note that you have to kill manually any other process forked so far!
        } else if (pid == 0) {
            // child process, its id is i, exit from cycle
            printf("[Child#%d] Child process created, pid %d\n", i, getpid());
            child_process(i);
            _exit(EXIT_SUCCESS);
        } else {
            // main process, go on creating all required child processes
        }
    }

    /* MAIN PROCESS */
    main_process();

    exit(EXIT_SUCCESS);
}
