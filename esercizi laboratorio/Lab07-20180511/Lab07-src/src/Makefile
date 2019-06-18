CC = gcc -Wall -g

all: client serial multiprocess multithread

client: client.c common.h
	$(CC) -o client client.c

serial: server.c common.h
	$(CC) -DSERVER_SERIAL -o serial server.c

multiprocess: server.c common.h
	$(CC) -DSERVER_MPROC -o multiprocess server.c

multithread: server.c common.h
	$(CC) -DSERVER_MTHREAD -o multithread server.c -lpthread

.PHONY: clean

clean:
	rm -f client serial multiprocess multithread
