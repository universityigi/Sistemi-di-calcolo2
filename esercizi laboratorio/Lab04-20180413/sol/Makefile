CC = gcc -Wall -g
LDFLAGS = -lpthread

all: riepilogo

riepilogo: riepilogo.c common.h
	$(CC) -o riepilogo riepilogo.c $(LDFLAGS)


.PHONY: clean
clean:
	rm -f riepilogo

