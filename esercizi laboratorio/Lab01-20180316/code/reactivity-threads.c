#include "performance.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <pthread.h>

void do_work() {
	return;
}

void* thread_fun(void *arg) {
	do_work();
	pthread_exit(NULL);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Syntax: %s <N> [<debug>]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// parse N from the command line
	int n = atoi(argv[1]);
	
	// debug mode: use 0 (off) when only N is given as argument
	int debug = (argc > 2) ? atoi(argv[2]) : 0;

	// thread reactivity
	printf("Thread reactivity, %d tests...\n", n);
	unsigned long sum = 0;
	timer t;
	int i;

	for (i = 0; i < n; i++) {
		pthread_t thread;
		begin(&t);
		if (pthread_create(&thread, NULL, thread_fun, NULL) != 0) {
			fprintf(stderr, "Can't create a new thread, error %d\n", errno);
			exit(EXIT_FAILURE);
		}
		pthread_join(thread, NULL);
		end(&t);
		sum += get_microseconds(&t);
		if (debug)
			printf("[%d] %lu us\n", i, get_microseconds(&t));
	}
	
	// compute statistics
	unsigned long thread_avg = sum / n;
	printf("Average: %lu microseconds\n", thread_avg);

	return EXIT_SUCCESS;
}
