CC = gcc -Wall -O0 -g
LDFLAGS = -lpthread
ARCH = $(shell uname -m)

all: client-$(ARCH)

client-$(ARCH): client.c common.h
	$(CC) client.c -o client-$(ARCH) $(LDFLAGS)

:phony
clean:
	rm -f client-$(ARCH)
