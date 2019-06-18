CC = gcc -Wall -g

all: echo client

echo: echo.c common.h
	$(CC) -o echo echo.c rw.c

client: client.c common.h
	$(CC) -o client client.c rw.c

.PHONY: clean

clean:
	rm -f echo client fifo_client fifo_echo
