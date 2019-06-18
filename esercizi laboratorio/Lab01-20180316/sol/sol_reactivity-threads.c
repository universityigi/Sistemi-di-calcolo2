#include "performance.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <pthread.h>

#define ITEMS   (1 << 24)
#define STEP    1024
int* global_buff = NULL;

void do_work() {
    int j;
    for (j = 0; j < ITEMS; j += STEP) {
        global_buff[j] = j;
    }
}

void* thread_fun(void *arg) {
	do_work();
	pthread_exit(NULL);
}


int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "Syntax: %s <N>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// parse N from the command line
	int n = atoi(argv[1]);
	
	// allocate a large buffer of zeroed memory 
	global_buff = (int*)calloc(ITEMS, sizeof(int));
    if (global_buff == NULL) {
        fprintf(stderr, "Cannot allocate memory!\n");
        exit(EXIT_FAILURE);
    }

	// thread reactivity
	printf("Thread reactivity, %d tests...\n", n);
	unsigned long sum = 0;
	timer t;
	int i, ret;

	begin(&t);
	for (i = 0; i < n; i++) {
		pthread_t thread;
		ret = pthread_create(&thread, NULL, thread_fun, NULL);
		if (ret != 0) {
			fprintf(stderr, "Can't create a new thread, error %d\n", ret);
			exit(EXIT_FAILURE);
		}
		ret = pthread_join(thread, NULL);
		if (ret != 0) {
			fprintf(stderr, "Cannot join on thread, error %d\n", ret);
			exit(EXIT_FAILURE);
		}
	}
	end(&t);
	sum += get_microseconds(&t);
	
	// compute statistics
	unsigned long thread_avg = sum / n;
	printf("Average: %lu microseconds\n", thread_avg);

	return EXIT_SUCCESS;
}
