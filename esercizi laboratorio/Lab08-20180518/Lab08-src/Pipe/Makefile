CC = gcc -Wall -g
LDFLAGS = -lpthread

all: main

main: main.c common.h
	$(CC) -o main main.c $(LDFLAGS)

.PHONY: clean
clean:
	rm -f main

