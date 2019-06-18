CFLAGS=-m32 -g -Wall
all: producer consumer

producer: producer.c common.h common.c
	gcc $(CFLAGS) -o producer producer.c common.c -lpthread

consumer: consumer.c common.h common.c
	gcc $(CFLAGS) -o consumer consumer.c common.c -lpthread

.PHONY: clean
clean:
	rm -f producer consumer bufferfile.bin