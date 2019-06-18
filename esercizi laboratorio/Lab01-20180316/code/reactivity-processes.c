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

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Syntax: %s <N> [<debug>]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// parse N from the command line
	int n = atoi(argv[1]);
	
	// debug mode: use 0 (off) when only N is given as argument
	int debug = (argc > 2) ? atoi(argv[2]) : 0;
		
	// process reactivity
	printf("Process reactivity, %d tests...\n", n);
	unsigned long sum = 0;
	timer t;
	int i;
	for (i = 0; i < n; i++) {
		begin(&t);
		pid_t pid = fork();
		if (pid == -1) {
			fprintf(stderr, "Can't fork, error %d\n", errno);
			exit(EXIT_FAILURE);
		} else if (pid == 0) {
			do_work();
			_exit(EXIT_SUCCESS);
		} else {
			wait(0);
		}
		end(&t);
		sum += get_microseconds(&t);
		if (debug) {
			printf("[%d] %lu us\n", i, get_microseconds(&t));
		}
	}
	unsigned long process_avg = sum / n;
	printf("Average: %lu microseconds\n", process_avg);
	
	return EXIT_SUCCESS;
}
